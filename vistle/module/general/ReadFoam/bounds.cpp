#include "foamtoolbox.h"
#include <iostream>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "no mesh directory given" << std::endl;
        return 1;
    }

    Boundaries b = loadBoundary(argv[1]);

    return b.valid ? 0 : 1;
}
