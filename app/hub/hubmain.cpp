#include <vistle/control/hub.h>

#include <iostream>

int main(int argc, char *argv[])
{
    vistle::Hub hub;
    if (!hub.init(argc, argv)) {
        return -1;
    }
    return hub.run();
}
