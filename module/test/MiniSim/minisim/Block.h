#ifndef BlockData_h
#define BlockData_h

#include <diy/master.hpp>
#include <diy/decomposition.hpp>
#include <diy/io/bov.hpp>
#include <diy/grid.hpp>
#include <diy/vertices.hpp>

#include "Oscillator.h"
#include "Particles.h"

#include <vector>
#include <ostream>

struct Block {
    using Vertex = diy::Grid<float, 3>::Vertex;

    Block(int gid_, const diy::DiscreteBounds &bounds_, const diy::DiscreteBounds &domain_,
          const diy::Point<float, 3> &origin_, const diy::Point<float, 3> &spacing_, int nghost_,
          const std::vector<Oscillator> &oscillators_, float velocity_scale_)
    : gid(gid_)
    , velocity_scale(velocity_scale_)
    , bounds(bounds_)
    , domain(domain_)
    , origin(origin_)
    , spacing(spacing_)
    , nghost(nghost_)
    , grid(Vertex(&bounds.max[0]) - Vertex(&bounds.min[0]) + Vertex::one())
    , oscillators(oscillators_)
    {}

    // update scalar and vector fields
    void update_fields(float t);

    // update pareticle positions
    void move_particles(float dt, const diy::Master::ProxyWithLink &cp);

    // handle particle migration
    void handle_incoming_particles(const diy::Master::ProxyWithLink &cp);

    // sdiy memory management hooks
    static void *create() { return new Block; }
    static void destroy(void *b) { delete static_cast<Block *>(b); }

    int gid; // block id
    float velocity_scale;
    diy::DiscreteBounds bounds; // cell centered index space dimensions of this block
    diy::DiscreteBounds domain; // cell centered index space dimensions of the computational domain
    diy::Point<float, 3> origin; // lower left most corner in the computational domain
    diy::Point<float, 3> spacing; // mesh spacing
    int nghost; // number of ghost zones
    diy::Grid<float, 3> grid; // container for the gridded data arrays
    std::vector<Particle> particles;
    std::vector<Oscillator> oscillators;

private:
    // for create; to let Master manage the blocks
    Block(): gid(-1), velocity_scale(1.0f), bounds(0), domain(0), nghost(0)
    {
        origin[0] = origin[1] = origin[2] = 0.0f;
        spacing[0] = spacing[1] = spacing[2] = 1.0f;
    }
};

// send to stream in human readbale format
std::ostream &operator<<(std::ostream &os, const Block &b);

#endif
