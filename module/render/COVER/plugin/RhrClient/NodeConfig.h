#ifndef VISTLE_COVER_PLUGIN_RHRCLIENT_NODECONFIG_H
#define VISTLE_COVER_PLUGIN_RHRCLIENT_NODECONFIG_H

#include <ostream>

struct NodeConfig {
    int viewIndexOffset = 0;
    int numChannels = 0;
    int numViews = 0;
    bool haveMiddle = false;
    bool haveLeft = false;
    bool haveRight = false;

    float screenMin[3], screenMax[3]; // screen bounding box
};

std::ostream &operator<<(std::ostream &stream, const NodeConfig &nc);
#endif
