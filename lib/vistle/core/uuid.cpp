#include "uuid.h"

#include <boost/uuid/random_generator.hpp>
#include <mutex>


namespace vistle {

static std::mutex s_uuidMutex;
static boost::uuids::random_generator s_uuidGenerator;


boost::uuids::uuid Uuid::generate()
{
    std::lock_guard<std::mutex> guard(s_uuidMutex);
    return s_uuidGenerator();
}

} // namespace vistle
