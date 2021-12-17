#include "sharedParam.h"
#include <algorithm>


bool operator==(const vistle::insitu::message::IntParam &p, const std::string &name)
{
    return p.name == name;
}

int vistle::insitu::message::getIntParamValue(const std::vector<IntParam> &c, const std::string &name)
{
    auto it = std::find_if(c.begin(), c.end(), [&name](const IntParam &param) { return param.name == name; });
    if (it == c.end()) {
        std::cerr << "get int value for param " << name << "failed" << std::endl;
        return 0;
    }
    return it->value;
}

void vistle::insitu::message::updateIntParam(std::vector<IntParam> &c, const IntParam &p)
{
    auto it = std::find_if(c.begin(), c.end(), [&p](const IntParam &param) { return param.name == p.name; });
    if (it != c.end())
        it->value = p.value;
    else
        c.push_back(p);
}
