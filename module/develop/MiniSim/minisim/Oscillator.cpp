#include "Oscillator.h"


#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <functional>
#include <diy/point.hpp>

// trim() from http://stackoverflow.com/a/217605/44738
static inline std::string &ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](char c) { return !std::isspace(c); }));
    return s;
}

static inline std::string &rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](char c) { return !std::isspace(c); }).base(), s.end());
    return s;
}

static inline std::string &trim(std::string &s)
{
    return ltrim(rtrim(s));
}

std::vector<Oscillator> read_oscillators(const std::string &fn)
{
    std::vector<Oscillator> res;

    std::ifstream in(fn);
    if (!in)
        throw std::runtime_error("Unable to open " + fn);
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream iss(line);

        std::string stype;
        iss >> stype;

        auto type = Oscillator::periodic;
        if (stype == "damped")
            type = Oscillator::damped;
        else if (stype == "decaying")
            type = Oscillator::decaying;

        float x, y, z;
        iss >> x >> y >> z;

        float r, omega0, zeta = 0.0f;
        iss >> r >> omega0;

        if (type == Oscillator::damped)
            iss >> zeta;
        res.emplace_back(Oscillator{{x, y, z}, r, omega0, zeta, type});
    }
    return res;
}
