#include "prepareShmName.h"
#include "exeption.h"

std::string vistle::insitu::cutRankSuffix(const std::string &shmName) {
#ifdef SHMPERRANK
    // remove the"_r" + std::to_string(rank) because that gets added by attach
    // again
    auto p = shmName.find("_r");
    if (p >= shmName.size()) {
        throw vistle::insitu::InsituExeption{} << "failed to create shmName: expected it to end with _r + "
                                                  "std::to_string(rank)";
    }
    return std::string(shmName, 0, p);

#endif // SHMPERRANK
    return shmName;
}
