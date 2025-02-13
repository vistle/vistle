#ifndef VISTLE_UTIL_NETPBMIMAGE_H
#define VISTLE_UTIL_NETPBMIMAGE_H

#include "export.h"

#include <string>
#include <cstdio>
#include <vector>
#include <limits>
#include <ostream>

namespace vistle {

class V_UTILEXPORT NetpbmImage {
public:
    enum Format {
        PBM, // bitmap: black&white
        Bitmap = PBM,
        PGM, // gray scale
        Graymap = PGM,
        PPM, // rgb color
        Color = PPM,
    };
    NetpbmImage(const std::string &name);
    NetpbmImage(const std::string &name, unsigned width, unsigned height, Format = PPM, unsigned highest = 0);
    virtual ~NetpbmImage();
    bool close();

    Format format() const;
    unsigned width() const;
    unsigned height() const;

    unsigned min() const;
    unsigned max() const;

    bool complete() const;

    bool append(float r, float g, float b);
    bool append(float gray);

    const float *gray() const;
    const unsigned char *rgb() const;

protected:
    size_t m_numwritten = 0;
    unsigned m_width = 0, m_height = 0;
    unsigned m_highest = 255;
    Format m_format = PPM;
    FILE *m_fp = nullptr;
    unsigned m_min = std::numeric_limits<unsigned>::max(), m_max = 0;

    std::vector<float> m_gray;
    std::vector<unsigned char> m_rgba;

    void increaseRange(unsigned value);
};

V_UTILEXPORT std::ostream &operator<<(std::ostream &s, const NetpbmImage &img);

} // namespace vistle
#endif
