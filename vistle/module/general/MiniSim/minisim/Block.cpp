#include "Block.h"

// --------------------------------------------------------------------------
void Block::update_fields(float t)
{
    // update the scalar oscillator field
    const Vertex &shape = grid.shape();
    int ni = shape[0];
    int nj = shape[1];
    int nk = shape[2];
    int nij = ni*nj;
    int i0 = bounds.min[0];
    int j0 = bounds.min[1];
    int k0 = bounds.min[2];
    float dx = spacing[0];
    float dy = spacing[1];
    float dz = spacing[2];
    float x0 = origin[0] + dx;
    float y0 = origin[1] + dy;
    float z0 = origin[2] + dz;
    float *pdata = grid.data();
    for (int k = 0; k < nk; ++k)
    {
        float z = z0 + dz*(k0 + k);
        float *pdk = pdata + k*nij;
        for (int j = 0; j < nj; ++j)
        {
            float y = y0 + dy*(j0 + j);
            float *pd = pdk + j*ni;
            for (int i = 0; i < ni; ++i)
            {
                float x = x0 + dx*(i0 + i);
                pd[i] = 0.f;
                for (auto& o : oscillators)
                    pd[i] += o.evaluate({x,y,z}, t);
            }
        }
    }

    // update the velocity field on the particle mesh
    for (auto& particle : particles)
    {
        particle.velocity = { 0, 0, 0 };
        for (auto& o : oscillators)
        {
            particle.velocity += o.evaluateGradient(particle.position, t);
        }
        // scale the gradient to get "units" right for velocity
        particle.velocity *= velocity_scale;
    }
}

// --------------------------------------------------------------------------
void Block::move_particles(float dt, const diy::Master::ProxyWithLink& cp)
{
    auto link = static_cast<diy::RegularGridLink*>(cp.link());

    auto particle = particles.begin();
    while (particle != particles.end())
    {
        // update particle position
        particle->position += particle->velocity * dt;

        // warp position if needed
        // applies periodic bci
        diy::Bounds<float> wsdom = world_space_bounds(domain, origin, spacing);
        for (int i = 0; i < 3; ++i)
        {
            if ((particle->position[i] > wsdom.max[i]) ||
              (particle->position[i] < wsdom.min[i]))
            {
                float dm = wsdom.min[i];
                float dx = wsdom.max[i] - dm;
                if (fabs(dx) < 1.0e-6f)
                {
                  particle->position[i] = dm;
                }
                else
                {
                  float dp = particle->position[i] - dm;
                  float dpdx = dp / dx;
                  particle->position[i] = (dpdx - floor(dpdx))*dx + dm;
                }
            }
        }

        // check if the particle has left this block
        // block bounds have ghost zones
        if (!contains(domain, bounds, origin, spacing, nghost, particle->position))
        {
            bool enqueued = false;

            // search neighbor blocks for one that now conatins this particle
            for (int i = 0; i < link->size(); ++i)
            {
                // link bounds do not have ghost zones
                if (contains(link->bounds(i), origin, spacing, particle->position))
                {
                    /*std::cerr << "moving " << *particle << " from " << gid
                      << " to " << link->target(i).gid << std::endl;*/

                    cp.enqueue(link->target(i), *particle);

                    enqueued = true;
                    break;
                }
            }

            if (!enqueued)
            {
                std::cerr << "Error: could not find appropriate neighbor for particle: "
                   << *particle << std::endl;

                abort();
            }

            particle = particles.erase(particle);
        }
        else
        {
            ++particle;
        }
    }
}

// --------------------------------------------------------------------------
void Block::handle_incoming_particles(const diy::Master::ProxyWithLink& cp)
{
    auto link = static_cast<diy::RegularGridLink*>(cp.link());
    for (int i = 0; i < link->size(); ++i)
    {
        auto nbr = link->target(i).gid;
        while(cp.incoming(nbr))
        {
            Particle particle;
            cp.dequeue(nbr, particle);
            particles.push_back(particle);
        }
    }
}

std::ostream &operator<<(std::ostream &os, const Block &b)
{
    os << b.gid << ": " << b.bounds.min << " - " << b.bounds.max << std::endl;

    auto it = b.particles.begin();
    auto end = b.particles.end();
    for (; it != end; ++it)
        os << "    " << *it << std::endl;

    return os;
}

