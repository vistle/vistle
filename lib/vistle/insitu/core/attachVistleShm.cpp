#include "attachVistleShm.h"
#include <vistle/core/shm.h>
#include <vistle/util/shmconfig.h>
#include <iostream>

void vistle::insitu::attachShm(const std::string &shmName, int id, int rank)
{
    std::cerr << "attaching to shm: name = " << shmName << std::endl;
    vistle::Shm::attach(shmName, id, rank, vistle::shmPerRank());
    static int numCalls = 0;
    ++numCalls;
    std::string suffix = "_sim";
    suffix += numCalls;
    vistle::Shm::setExternalSuffix(suffix);
}

void vistle::insitu::detachShm()
{
#ifndef MODULE_THREAD
    if (vistle::Shm::isAttached()) {
        vistle::Shm::the().detach();
    }
#endif // !#ifndef MODULE_THREAD
}
