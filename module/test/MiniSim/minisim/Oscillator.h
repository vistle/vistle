#ifndef Oscillator_h
#define Oscillator_h

#include <string>
#include <cmath>
#include <diy/point.hpp>

struct Oscillator {
    using Vertex = diy::Point<float, 3>;

    static constexpr float pi = 3.14159265358979323846;

    float evaluate(const Vertex &v, float t) const
    {
        t *= 2 * pi;

        float dist2 = (center - v).norm2();
        float dist_damp = exp(-dist2 / (2 * radius * radius));

        if (type == damped) {
            float phi = acos(zeta);
            float val = 1. - exp(-zeta * omega0 * t) * (sin(sqrt(1 - zeta * zeta) * omega0 * t + phi) / sin(phi));
            return val * dist_damp;
        } else if (type == decaying) {
            t += 1. / omega0;
            float val = sin(t / omega0) / (omega0 * t);
            return val * dist_damp;
        } else if (type == periodic) {
            t += 1. / omega0;
            float val = sin(t / omega0);
            return val * dist_damp;
        }

        return 0.0f; // impossible
    }

    Vertex evaluateGradient(const Vertex &x, float t) const
    {
        // let f(x, t) = this->evaluate(x,t) = o(t) * g(x)
        // where o = oscillator, g = gaussian
        // therefore, df/dx = o * dg/dx
        // given g = e^u(t), so dg/dx = e^u * du/dx
        // therefore, df/dx = o * e^u * du/dx = f * du/dx
        return evaluate(x, t) * ((center - x) / (radius * radius));
    }

    Vertex center;
    float radius;

    float omega0;
    float zeta;

    enum { damped, decaying, periodic } type;
};

std::vector<Oscillator> read_oscillators(const std::string &fn);

#endif
