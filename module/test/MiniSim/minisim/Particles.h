#ifndef particles_h
#define particles_h

#include <diy/types.hpp>
#include <diy/point.hpp>

#include <random>
#include <vector>


// container for a single particle
struct Particle {
    int id;
    diy::Point<float, 3> position;
    diy::Point<float, 3> velocity;
};

// put the particle in the stream in human readable format
std::ostream &operator<<(std::ostream &os, const Particle &particle);

// strips the ghost zones from the block, returns a copy
// with ghosts removed. ghost zones don't go outside of the
// computational domain.
diy::DiscreteBounds remove_ghosts(const diy::DiscreteBounds &domain, const diy::DiscreteBounds &bounds, int nghosts);

// convert from index space into world space accounting for ghost zones
template<typename coord_type>
diy::Bounds<coord_type> world_space_bounds(const diy::DiscreteBounds &bounds, const diy::Point<coord_type, 3> &origin,
                                           const diy::Point<coord_type, 3> &spacing)
{
    // the unit of bounds member are in cell centered index space
    // convert from index space bounding box into world space
    diy::Bounds<coord_type> world_bounds{3};

    world_bounds.min[0] = origin[0] + spacing[0] * bounds.min[0];
    world_bounds.min[1] = origin[1] + spacing[1] * bounds.min[1];
    world_bounds.min[2] = origin[2] + spacing[2] * bounds.min[2];

    world_bounds.max[0] = origin[0] + spacing[0] * (bounds.max[0] + 1);
    world_bounds.max[1] = origin[1] + spacing[1] * (bounds.max[1] + 1);
    world_bounds.max[2] = origin[2] + spacing[2] * (bounds.max[2] + 1);

    // handle planar data
    for (int i = 0; i < 3; ++i)
        if (bounds.min[i] == bounds.max[i])
            world_bounds.max[i] = world_bounds.min[i];

    return world_bounds;
}

// convert from index space into world space accounting for ghost zones
template<typename coord_type>
diy::Bounds<coord_type> world_space_bounds(const diy::DiscreteBounds &domain, const diy::DiscreteBounds &gbounds,
                                           const diy::Point<coord_type, 3> &origin,
                                           const diy::Point<coord_type, 3> &spacing, int nghost)
{
    // remove ghost zones
    diy::DiscreteBounds bounds = remove_ghosts(domain, gbounds, nghost);

    // the unit of bounds member are in cell centered index space
    // convert from index space bounding box into world space
    return world_space_bounds(bounds, origin, spacing);
}

// gerate count particles
template<typename coord_type>
std::vector<Particle>
GenerateRandomParticles(std::default_random_engine &rng, const diy::DiscreteBounds &domain,
                        const diy::DiscreteBounds &gbounds, const diy::Point<coord_type, 3> &origin,
                        const diy::Point<coord_type, 3> &spacing, int nghost, int startId, int count)
{
    diy::Bounds<coord_type> world_bounds = world_space_bounds(domain, gbounds, origin, spacing, nghost);

    std::uniform_real_distribution<coord_type> rgx(world_bounds.min[0], world_bounds.max[0]);
    std::uniform_real_distribution<coord_type> rgy(world_bounds.min[1], world_bounds.max[1]);
    std::uniform_real_distribution<coord_type> rgz(world_bounds.min[2], world_bounds.max[2]);

    std::vector<Particle> particles;
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.id = startId + i;
        p.position = {rgx(rng), rgy(rng), rgz(rng)};
        p.velocity = {0, 0, 0};
        particles.push_back(p);
    }

    return particles;
}

// return true if the particle is inside this block
template<typename coord_type>
bool contains(const diy::DiscreteBounds &bounds, const diy::Point<coord_type, 3> &origin,
              const diy::Point<coord_type, 3> &spacing, const typename diy::Point<coord_type, 3> &v)
{
    diy::Bounds<coord_type> world_bounds = world_space_bounds(bounds, origin, spacing);

    // test if the particle is inside this block
    return (v[0] >= world_bounds.min[0]) && (v[0] <= world_bounds.max[0]) && (v[1] >= world_bounds.min[1]) &&
           (v[1] <= world_bounds.max[1]) && (v[2] >= world_bounds.min[2]) && (v[2] <= world_bounds.max[2]);
}


// return true if the particle is inside this block
template<typename coord_type>
bool contains(const diy::DiscreteBounds &domain, const diy::DiscreteBounds &gbounds,
              const diy::Point<coord_type, 3> &origin, const diy::Point<coord_type, 3> &spacing, int nghost,
              const typename diy::Point<coord_type, 3> &v)
{
    diy::Bounds<coord_type> world_bounds = world_space_bounds(domain, gbounds, origin, spacing, nghost);

    // test if the particle is inside this block
    return (v[0] >= world_bounds.min[0]) && (v[0] <= world_bounds.max[0]) && (v[1] >= world_bounds.min[1]) &&
           (v[1] <= world_bounds.max[1]) && (v[2] >= world_bounds.min[2]) && (v[2] <= world_bounds.max[2]);
}

#endif
