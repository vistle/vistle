#ifndef VISTLE_CORE_MESSAGEPAYLOADTEMPLATES_H
#define VISTLE_CORE_MESSAGEPAYLOADTEMPLATES_H

#include <vistle/util/tools.h>
#include "archives.h"
namespace vistle {
namespace message {


template<class Payload>
buffer addPayload(Message &message, const Payload &payload)
{
    vecostreambuf<buffer> buf;
    oarchive ar(buf);
    ar &const_cast<Payload &>(payload);
    auto vec = buf.get_vector();
    message.setPayloadSize(vec.size());
    return vec;
}

template<class Payload>
void getFromPayload(const buffer &data, Payload &payload)
{
    vecistreambuf<buffer> buf(data);
    try {
        iarchive ar(buf);
        ar &payload;
#ifdef USE_YAS
    } catch (::yas::io_exception &ex) {
        std::cerr << "ERROR: failed to get message payload from " << data.size() << " bytes: " << ex.what()
                  << std::endl;
        std::cerr << vistle::backtrace() << std::endl;
#endif
    } catch (...) {
        throw;
    }
}

template<class Payload>
Payload getPayload(const buffer &data)
{
    Payload payload;
    getFromPayload(data, payload);
    return payload;
}

} // namespace message

} // namespace vistle


#endif
