#include <memory>
#include <vector>

#include <diy/master.hpp>
#include <diy/reduce.hpp>
#include <diy/partners/merge.hpp>
#include <diy/io/numpy.hpp>
#include <diy/grid.hpp>
#include <diy/vertices.hpp>

#include "analysis.h"

using GridRef = diy::GridRef<float, 3>;
using Vertex = GridRef::Vertex;
using Vertex4D = Vertex::UPoint;

struct Autocorrelation {
    using Grid = diy::Grid<float, 4>;

    Autocorrelation(size_t window_, int gid_, Vertex from_, Vertex to_)
    : window(window_)
    , gid(gid_)
    , from(from_)
    , to(to_)
    , shape(to - from + Vertex::one())
    ,
    // init grid with (to - from + 1) in 3D, and window in the 4-th dimension
    values(shape.lift(3, window))
    , corr(shape.lift(3, window))
    {
        corr = 0;
    }

    static void *create() { return new Autocorrelation; }
    static void destroy(void *b) { delete static_cast<Autocorrelation *>(b); }

    void process(float *data)
    {
        GridRef g(data, shape);

        // record the values
        diy::for_each(g.shape(), [&](const Vertex &v) {
            auto gv = g(v);

            for (size_t i = 1; i <= window; ++i) {
                if (i > count)
                    continue; // during the initial fill, we don't get contributions to some shifts

                auto uc = v.lift(3, i - 1);
                auto uv = v.lift(3, (offset + window - i) % window);
                corr(uc) += values(uv) * gv;
            }

            auto u = v.lift(3, offset);
            values(u) = gv;
        });

        offset += 1;
        offset %= window;

        ++count;
    }

    size_t window;
    int gid;
    Vertex from, to, shape;
    Grid values; // circular buffer of last `window` values
    Grid corr; // autocorrelations for different time shifts

    size_t offset = 0;
    size_t count = 0;

private:
    Autocorrelation() {} // here just for create; to let Master manage the blocks (+ if we choose to add OOC later)
};

template<class T>
struct add_vectors {
    using Vector = std::vector<T>;
    Vector operator()(const Vector &lhs, const Vector &rhs) const
    {
        Vector res(lhs.size(), 0);
        for (size_t i = 0; i < lhs.size(); ++i)
            res[i] = lhs[i] + rhs[i];
        return res;
    }
};
namespace diy {
namespace mpi {
namespace detail {
template<class U>
struct mpi_op<add_vectors<U>> {
    static MPI_Op get() { return MPI_SUM; }
};
} // namespace detail
} // namespace mpi
} // namespace diy

std::unique_ptr<diy::Master> master;
Vertex domain_shape;
Vertex4D buffer_shape;

// http://stackoverflow.com/a/12580468
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&...args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

void init_analysis(MPI_Comm world, size_t window, size_t n_local_blocks, int domain_shape_x, int domain_shape_y,
                   int domain_shape_z, int *gid, int *from_x, int *from_y, int *from_z, int *to_x, int *to_y, int *to_z)
{
    master = make_unique<diy::Master>(world, -1, -1, &Autocorrelation::create, &Autocorrelation::destroy);
    domain_shape = Vertex{domain_shape_x, domain_shape_y, domain_shape_z};
    buffer_shape = domain_shape.lift(3, window);

    // add blocks, one for each gid
    for (size_t i = 0; i < n_local_blocks; ++i) {
        Vertex from{from_x[i], from_y[i], from_z[i]};
        Vertex to{to_x[i], to_y[i], to_z[i]};
        Autocorrelation *b = new Autocorrelation(window, gid[i], from, to);
        master->add(gid[i], b, new diy::Link);
    }
}

void analyze(int gid, float *data)
{
    // per-box operations
    int lid = master->lid(gid);
    Autocorrelation *corr = master->block<Autocorrelation>(lid);
    corr->process(data);
}

void analysis_round()
{
    // per-round operations: none in our case
}

void analysis_final(size_t k_max, size_t nblocks)
{
#if 0
    // Save the corr buffer for debugging
    diy::mpi::io::file out(master->communicator(), "buffer.npy", diy::mpi::io::file::wronly | diy::mpi::io::file::create);
    diy::io::NumPy writer(out);
    writer.write_header<float>(buffer_shape);
    master->foreach([&writer](Autocorrelation* b, const diy::Master::ProxyWithLink)
                                     {
                                       diy::DiscreteBounds bounds;
                                       for (size_t i = 0; i < 3; ++i)
                                       {
                                         bounds.min[i] = b->from[i];
                                         bounds.max[i] = b->to[i];
                                       }
                                       bounds.min[3] = 0;
                                       bounds.max[3] = b->window - 1;
                                       writer.write(bounds, b->corr.data(), true);
                                     });
#endif

    // add up the autocorrellations
    master->foreach ([](Autocorrelation *b, const diy::Master::ProxyWithLink &cp) {
        std::vector<float> sums(b->window, 0);
        diy::for_each(b->corr.shape(), [&](const Vertex4D &v) {
            size_t w = v[3];
            sums[w] += b->corr(v);
        });

        cp.all_reduce(sums, add_vectors<float>());
    });
    master->exchange();

    if (master->communicator().rank() == 0) {
        // print out the autocorrelations
        auto result = master->proxy(0).get<std::vector<float>>();
        std::cout << "Autocorrelations:";
        for (size_t i = 0; i < result.size(); ++i)
            std::cout << ' ' << result[i];
        std::cout << std::endl;
    }
    master->foreach ([](Autocorrelation *, const diy::Master::ProxyWithLink &cp) { cp.collectives()->clear(); });

    // select k strongest autocorrelations for each shift
    diy::ContiguousAssigner assigner(master->communicator().size(),
                                     nblocks); // NB: this is coupled to main(...) in oscillator.cpp
    //diy::RegularMergePartners   partners(3, nblocks, 2, true);
    diy::RegularDecomposer<diy::DiscreteBounds> decomposer(1, diy::interval(0, nblocks - 1), nblocks);
    diy::RegularMergePartners partners(decomposer, 2);
    diy::reduce(*master, assigner, partners,
                [k_max](void *b_, const diy::ReduceProxy &rp, const diy::RegularMergePartners &) {
                    Autocorrelation *b = static_cast<Autocorrelation *>(b_);
                    //unsigned round = rp.round(); // current round number

                    using MaxHeapVector = std::vector<std::vector<std::tuple<float, Vertex>>>;
                    using Compare = std::greater<std::tuple<float, Vertex>>;
                    MaxHeapVector maxs(b->window);
                    if (rp.in_link().size() == 0) {
                        diy::for_each(b->corr.shape(), [&](const Vertex4D &v) {
                            size_t offset = v[3];
                            float val = b->corr(v);
                            auto &max = maxs[offset];
                            if (max.size() < k_max) {
                                max.emplace_back(val, v.drop(3) + b->from);
                                std::push_heap(max.begin(), max.end(), Compare());
                            } else if (val > std::get<0>(max[0])) {
                                std::pop_heap(max.begin(), max.end(), Compare());
                                maxs[offset].back() = std::make_tuple(val, v.drop(3) + b->from);
                                std::push_heap(max.begin(), max.end(), Compare());
                            }
                        });
                    } else {
                        for (long i = 0; i < rp.in_link().size(); ++i) {
                            MaxHeapVector in_heaps;
                            rp.dequeue(rp.in_link().target(i).gid, in_heaps);
                            for (size_t j = 0; j < in_heaps.size(); ++j) {
                                for (auto &x: in_heaps[j])
                                    maxs[j].push_back(x);
                            }
                        }
                        for (size_t j = 0; j < maxs.size(); ++j) {
                            auto &max = maxs[j];
                            std::make_heap(max.begin(), max.end(), Compare());
                            while (max.size() > k_max) {
                                std::pop_heap(max.begin(), max.end(), Compare());
                                max.pop_back();
                            }
                        }
                    }

                    if (rp.out_link().size() > 0)
                        rp.enqueue(rp.out_link().target(0), maxs);
                    else {
                        // print out the answer
                        for (size_t i = 0; i < maxs.size(); ++i) {
                            std::sort(maxs[i].begin(), maxs[i].end(), Compare());
                            std::cout << "Max autocorrelations for " << i << ":";
                            for (auto &x: maxs[i])
                                std::cout << " (" << std::get<0>(x) << " at " << std::get<1>(x) << ")";
                            std::cout << std::endl;
                        }
                    }
                });
}
