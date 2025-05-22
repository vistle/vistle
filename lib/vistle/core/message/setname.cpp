#include "setname.h"

template<typename T>
static T min(T a, T b)
{
    return a < b ? a : b;
}
#define COPY_STRING(dst, src) \
    do { \
        const size_t size = min(src.size(), dst.size() - 1); \
        src.copy(dst.data(), size); \
        dst[size] = '\0'; \
        assert(src.size() < dst.size()); \
    } while (false)

namespace vistle {
namespace message {

SetName::SetName(int module, const std::string &name): m_module(module)
{
    COPY_STRING(m_name, name);
}

const char *SetName::name() const
{
    return m_name.data();
}

int SetName::module() const
{
    return m_module;
}


} // namespace message
} // namespace vistle
