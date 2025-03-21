#include "Block.h"
#include "minisim.h"
#include "Oscillator.h"
#include "analysis.h"
#include "../MiniSim.h"

#include <array>
#include <iostream>
#include <random>

#include <diy/decomposition.hpp>
#include <diy/grid.hpp>
#include <diy/io/bov.hpp>
#include <diy/master.hpp>
#include <diy/point.hpp>
#include <diy/vertices.hpp>

#include <vistle/util/stopwatch.h>

#define ENABLE_VISTLE
using namespace minisim;
using namespace vistle;

using Grid = diy::Grid<float, 3>;
using Vertex = Grid::Vertex;
using SpPoint = diy::Point<float, 3>;
using Bounds = diy::Point<float, 6>;

using Link = diy::RegularGridLink;
using Master = diy::Master;
using Proxy = Master::ProxyWithLink;

using RandomSeedType = std::default_random_engine::result_type;

using Time = std::chrono::high_resolution_clock;
using ms = std::chrono::milliseconds;

void MiniSim::run(MiniSimModule &mod, size_t numTimesteps, const std::string &inputFile,
                  const boost::mpi::communicator &comm, const Parameter &param)
{
    m_terminate = false;
    Vertex shape = {64, 64, 64};
    constexpr int seed = 0x240dc6a9;
    Bounds bounds{0., -1., 0., -1., 0., -1.};
    diy::mpi::communicator world{comm};
    const int nblocks = param.nblocks == 0 ? world.size() : param.nblocks; //number of threads to use
    int particlesPerBlock = param.numberOfParticles / nblocks;
    std::vector<Oscillator> oscillators;
    if (world.rank() == 0) {
        oscillators = read_oscillators(inputFile);
        diy::MemoryBuffer bb;
        diy::save(bb, oscillators);
        diy::mpi::broadcast(world, bb.buffer, 0);
    } else {
        diy::MemoryBuffer bb;
        diy::mpi::broadcast(world, bb.buffer, 0);
        diy::load(bb, oscillators);
    }

    diy::Master master(world, param.threads, -1, &Block::create, &Block::destroy);

    diy::ContiguousAssigner assigner(world.size(), nblocks);

    diy::DiscreteBounds domain{3};
    domain.min[0] = domain.min[1] = domain.min[2] = 0;
    for (unsigned i = 0; i < 3; ++i)
        domain.max[i] = shape[i] - 1;

    SpPoint origin{0., 0., 0.};
    SpPoint spacing{1., 1., 1.};
    if (bounds[1] >= bounds[0]) {
        // valid bounds specififed on the command line, calculate the
        // global origin and spacing.
        for (int i = 0; i < 3; ++i)
            origin[i] = bounds[2 * i];

        for (int i = 0; i < 3; ++i)
            spacing[i] = (bounds[2 * i + 1] - bounds[2 * i]) / shape[i];
    }

    if (param.verbose && (world.rank() == 0)) {
        std::cerr << world.rank() << " domain = " << domain.min << ", " << domain.max << std::endl
                  << world.rank() << "bounds = " << bounds << std::endl
                  << world.rank() << "origin = " << origin << std::endl
                  << world.rank() << "spacing = " << spacing << std::endl;
    }

    // record various parameters to initialize analysis
    std::vector<int> gids, from_x, from_y, from_z, to_x, to_y, to_z;

    diy::RegularDecomposer<diy::DiscreteBounds>::BoolVector share_face;
    diy::RegularDecomposer<diy::DiscreteBounds>::BoolVector wrap(3, true);
    diy::RegularDecomposer<diy::DiscreteBounds>::CoordinateVector ghosts = {param.ghostCells, param.ghostCells,
                                                                            param.ghostCells};

    // decompose the domain
    diy::decompose(
        3, world.rank(), domain, assigner,
        [&](int gid, const diy::DiscreteBounds &, const diy::DiscreteBounds &bounds, const diy::DiscreteBounds &domain,
            const Link &link) {
            Block *b =
                new Block(gid, bounds, domain, origin, spacing, param.ghostCells, oscillators, param.velocity_scale);

            // generate particles
            int start = particlesPerBlock * gid;
            int count = particlesPerBlock;
            if ((start + count) > param.numberOfParticles)
                count = std::max(0, param.numberOfParticles - start);
            std::default_random_engine rng(static_cast<RandomSeedType>(seed));
            for (int i = 0; i < world.rank(); ++i) {
                (void)rng(); // different seed for each rank
            }
            b->particles = GenerateRandomParticles<float>(rng, b->domain, b->bounds, b->origin, b->spacing, b->nghost,
                                                          start, count);

            master.add(gid, b, new Link(link));

            gids.push_back(gid);

            from_x.push_back(bounds.min[0]);
            from_y.push_back(bounds.min[1]);
            from_z.push_back(bounds.min[2]);

            to_x.push_back(bounds.max[0]);
            to_y.push_back(bounds.max[1]);
            to_z.push_back(bounds.max[2]);

            if (param.verbose)
                std::cerr << world.rank() << " Block " << *b << std::endl;
        },
        share_face, wrap, ghosts);

    mod.initialize(nblocks, gids.size(), origin.data(), spacing.data(), domain.max[0] + 1, domain.max[1] + 1,
                   domain.max[2] + 1, &gids[0], &from_x[0], &from_y[0], &from_z[0], &to_x[0], &to_y[0], &to_z[0],
                   &shape[0], param.ghostCells);

    int t_count = 0;
    float t = 0.;
    while (t < param.t_end && !m_terminate) {
        if (param.verbose && (world.rank() == 0))
            std::cerr << "started step = " << t_count << " t = " << t << std::endl;

        std::unique_ptr<StopWatch> profiler;
        profiler = std::make_unique<StopWatch>("oscillators::update_fields");
        master.foreach ([=](Block *b, const Proxy &) { b->update_fields(t); });

        profiler = std::make_unique<StopWatch>("oscillators::move_particles");
        master.foreach ([=](Block *b, const Proxy &p) { b->move_particles(param.dt, p); });

        profiler = std::make_unique<StopWatch>("oscillators::master.exchange");
        master.exchange();

        profiler = std::make_unique<StopWatch>("oscillators::handle_incoming_particles");
        master.foreach ([=](Block *b, const Proxy &p) { b->handle_incoming_particles(p); });

        profiler = std::make_unique<StopWatch>("oscillators::analysis");
        // do the analysis using sensei
        // update data adaptor with new data
        master.foreach ([&mod](Block *b, const Proxy &) {
            std::cerr << "data field size = " << b->grid.size() << std::endl;
            mod.SetBlockData(b->gid, b->grid.data());
            mod.SetParticleData(b->gid, b->particles);
        });
        // push data to sensei
        mod.execute(t_count, t);

        profiler.reset(nullptr);

        if (!param.out_prefix.empty()) {
            auto out_start = Time::now();

            // Save the corr buffer for debugging
            std::ostringstream outfn;
            outfn << param.out_prefix << "-" << t << ".bin";

            diy::mpi::io::file out(world, outfn.str(), diy::mpi::io::file::wronly | diy::mpi::io::file::create);
            diy::io::BOV writer(out, shape);
            master.foreach ([&writer](Block *b, const diy::Master::ProxyWithLink &cp) {
                auto link = static_cast<Link *>(cp.link());
                writer.write(link->bounds(), b->grid.data(), true);
            });
            if (param.verbose && (world.rank() == 0)) {
                auto out_duration = std::chrono::duration_cast<ms>(Time::now() - out_start);
                std::cerr << "Output time for " << outfn.str() << ":" << out_duration.count() / 1000 << "."
                          << out_duration.count() % 1000 << " s" << std::endl;
            }
        }

        if (param.sync)
            world.barrier();

        t += param.dt;
        ++t_count;
    }
    mod.finalize();
}


void MiniSim::terminate()
{
    m_terminate = true;
}
