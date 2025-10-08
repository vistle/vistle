#ifndef VISTLE_CORE_MESSAGE_COLORMAP_H
#define VISTLE_CORE_MESSAGE_COLORMAP_H

#include "../message.h"
#include "../rgba.h"

namespace vistle {
namespace message {

//! add or update a colormap
class V_COREEXPORT Colormap: public MessageBase<Colormap, COLORMAP> {
public:
    Colormap(const std::string &species, int sourceModule = message::Id::Invalid);
    int source() const;
    const char *species() const;
    double min() const;
    double max() const;
    void setRange(double min, double max);
    bool blendWithMaterial() const;
    void setBlendWithMaterial(bool enable = true);

    struct Payload {
        std::vector<RGBA> rgba;

        Payload();
        Payload(const std::vector<RGBA> &rgba);
        Payload(size_t width, const uint8_t *rgbaData);

        ARCHIVE_ACCESS
        template<class Archive>
        void serialize(Archive &ar)
        {
            ar &rgba;
        }
    };

private:
    int m_sourceModule{message::Id::Invalid};
    description_t m_species;
    double m_range[2]{0., 1.};
    bool m_blendWithMaterial{false};
};

//! remove a colormap
class V_COREEXPORT RemoveColormap: public MessageBase<RemoveColormap, REMOVECOLORMAP> {
public:
    RemoveColormap(const std::string &species, int sourceModule = message::Id::Invalid);
    int source() const;
    const char *species() const;

private:
    int m_sourceModule{message::Id::Invalid};
    description_t m_species;
};

extern template V_COREEXPORT buffer addPayload<Colormap::Payload>(Message &message, const Colormap::Payload &payload);
extern template V_COREEXPORT Colormap::Payload getPayload(const buffer &data);

} // namespace message
} // namespace vistle
#endif
