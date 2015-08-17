#include "shm.h"

namespace vistle {

#if 0
template<class T, typename... Args>
shm_ref::shm_ref(Args...)
: m_name(Shm::the().createObjectId())
, m_p(Shm::the().construct(m_name)(Args))
{
    m_p->ref();
}

template<class T>
shm_ref::~shm_ref() {
    if (m_p)
        m_p->unref();
}

#endif

}
