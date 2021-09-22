#include "NodeConfig.h"

std::ostream &operator<<(std::ostream &stream, const NodeConfig &nc)
{
    stream << "off=" << nc.viewIndexOffset << ", #chan=" << nc.numChannels << ", #view=" << nc.numViews
           << ", eye=" << (nc.haveMiddle ? "M" : ".") << (nc.haveLeft ? "L" : ".") << (nc.haveRight ? "R" : ".");

    return stream;
}
