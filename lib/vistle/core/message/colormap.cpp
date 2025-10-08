#include "colormap.h"
#include <algorithm>
#include <cassert>

#include "../messagepayloadtemplates.h"

#define COPY_STRING(dst, src) \
    do { \
        const size_t size = std::min(src.size(), dst.size() - 1); \
        src.copy(dst.data(), size); \
        dst[size] = '\0'; \
        assert(src.size() < dst.size()); \
    } while (false)

namespace vistle {
namespace message {

template V_COREEXPORT buffer addPayload<Colormap::Payload>(Message &message, const Colormap::Payload &payload);
template V_COREEXPORT Colormap::Payload getPayload(const buffer &data);

Colormap::Payload::Payload() = default;

Colormap::Payload::Payload(const std::vector<RGBA> &rgba): rgba(rgba)
{}

Colormap::Payload::Payload(size_t width, const uint8_t *rgbaData)
{
    rgba.resize(width);
    std::copy(rgbaData, rgbaData + width * 4, reinterpret_cast<uint8_t *>(rgba.data()));
}

Colormap::Colormap(const std::string &species, int module): m_sourceModule(module)
{
    COPY_STRING(m_species, species);
}

int Colormap::source() const
{
    return m_sourceModule;
}

const char *Colormap::species() const
{
    return m_species.data();
}

double Colormap::min() const
{
    return m_range[0];
}

double Colormap::max() const
{
    return m_range[1];
}

void Colormap::setRange(double min, double max)
{
    m_range[0] = min;
    m_range[1] = max;
}

bool Colormap::blendWithMaterial() const
{
    return m_blendWithMaterial;
}

void Colormap::setBlendWithMaterial(bool enable)
{
    m_blendWithMaterial = enable;
}

RemoveColormap::RemoveColormap(const std::string &species, int module): m_sourceModule(module)
{
    COPY_STRING(m_species, species);
}

int RemoveColormap::source() const
{
    return m_sourceModule;
}
const char *RemoveColormap::species() const
{
    return m_species.data();
}

} // namespace message
} // namespace vistle
