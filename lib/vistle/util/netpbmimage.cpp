#include "netpbmimage.h"
#include "exception.h"
#include "math.h"

namespace vistle {

#define CHECK(expect, func) \
    if (expect != func) { \
        throw vistle::exception("fscanf parse error"); \
    }

NetpbmImage::NetpbmImage(const std::string &name)
{
    FILE *fp = fopen(name.c_str(), "r");
    if (!fp) {
        throw vistle::exception("could not open file for reading");
    }

    int level = 0;
    CHECK(1, fscanf(fp, "P%d", &level));
    switch (level) {
    case 1:
        m_format = PBM;
        m_highest = 1;
        break;
    case 2:
        m_format = PGM;
        break;
    case 3:
        m_format = PPM;
        break;
    default:
        fclose(fp);
        throw vistle::exception("unsupported format");
    }

    CHECK(2, fscanf(fp, "%u %u", &m_width, &m_height));
    if (m_format != PBM) {
        CHECK(1, fscanf(fp, "%u", &m_highest));
    }

    size_t numpix = m_width * m_height;
    m_rgba.resize(numpix * 4);
    m_gray.resize(numpix);

    for (size_t i = 0; i < numpix; ++i) {
        if (m_format == PPM) {
            unsigned r = 0, g = 0, b = 0;
            CHECK(3, fscanf(fp, "%u %u %u", &r, &g, &b));
            increaseRange(r);
            increaseRange(g);
            increaseRange(b);
            float rr = float(r) / float(m_highest);
            float gg = float(g) / float(m_highest);
            float bb = float(b) / float(m_highest);
            m_gray[i] = (rr + gg + bb) / 3.f;
            m_rgba[i * 4 + 0] = int(rr * 255.99f);
            m_rgba[i * 4 + 1] = int(gg * 255.99f);
            m_rgba[i * 4 + 2] = int(bb * 255.99f);
        } else {
            unsigned gray = 0;
            CHECK(1, fscanf(fp, "%u", &gray));
            increaseRange(gray);
            m_gray[i] = float(gray) / float(m_highest);
            unsigned g = m_gray[i] * 255.99f;
            m_rgba[i * 4 + 0] = g;
            m_rgba[i * 4 + 1] = g;
            m_rgba[i * 4 + 2] = g;
        }
        m_rgba[i * 4 + 3] = 255;
    }

    m_numwritten = m_width * m_height; // make it complete

    fclose(fp);
}

NetpbmImage::NetpbmImage(const std::string &name, unsigned width, unsigned height, NetpbmImage::Format format,
                         unsigned highest)
: m_width(width), m_height(height), m_highest(highest), m_format(format)
{
    m_fp = fopen(name.c_str(), "wb");
    if (!m_fp) {
        throw vistle::exception("could not open file for writing");
    }

    switch (format) {
    case PBM:
        if (m_highest == 0)
            m_highest = 1;
        fprintf(m_fp, "P1\n%u %u\n", m_width, m_height);
        break;
    case PGM:
        if (m_highest == 0)
            m_highest = (1 << 16) - 1;
        fprintf(m_fp, "P2\n%u %u\n%d\n", m_width, m_height, m_highest);
        break;
    case PPM:
        if (m_highest == 0)
            m_highest = (1 << 8) - 1;
        fprintf(m_fp, "P3\n%u %u\n%d\n", m_width, m_height, m_highest);
        break;
    }
}

NetpbmImage::~NetpbmImage()
{
    if (m_fp) {
        while (m_numwritten < m_width * m_height) {
            append(0.f);
        }
    }
    close();
}

bool NetpbmImage::close()
{
    if (!m_fp)
        return false;

    FILE *fp = m_fp;
    m_fp = nullptr;
    return fclose(fp) == 0;
}

NetpbmImage::Format NetpbmImage::format() const
{
    return m_format;
}

unsigned NetpbmImage::width() const
{
    return m_width;
}

unsigned NetpbmImage::height() const
{
    return m_height;
}

unsigned NetpbmImage::min() const
{
    return m_min;
}

unsigned NetpbmImage::max() const
{
    return m_max;
}

bool NetpbmImage::complete() const
{
    return m_numwritten >= m_width * m_height;
}

bool NetpbmImage::append(float r, float g, float b)
{
    if (!m_fp)
        return false;

    if (m_numwritten >= m_width * m_height)
        return false;

    r = vistle::clamp(r, 0.f, 1.f);
    g = vistle::clamp(g, 0.f, 1.f);
    b = vistle::clamp(b, 0.f, 1.f);

    bool good = true;

    switch (m_format) {
    case PBM: {
        unsigned val = unsigned(r + g + b > 1.5 ? 1 : 0);
        increaseRange(val);
        if (fprintf(m_fp, "%u", val) <= 0)
            good = false;
        break;
    }
    case PGM: {
        unsigned val = unsigned((r + g + b) / 3.f * m_highest);
        increaseRange(val);
        if (fprintf(m_fp, "%u", val) <= 0)
            good = false;
        break;
    }
    case PPM: {
        unsigned rr(r * m_highest), gg(g * m_highest), bb(b * m_highest);
        increaseRange(rr);
        increaseRange(gg);
        increaseRange(bb);
        if (fprintf(m_fp, "%u %u %u", rr, gg, bb) <= 0)
            good = false;
        break;
    }
    }

    ++m_numwritten;
    if (fprintf(m_fp, m_numwritten % 16 ? " " : "\n") <= 0)
        good = false;

    return good;
}

bool NetpbmImage::append(float gray)
{
    return append(gray, gray, gray);
}

const float *NetpbmImage::gray() const
{
    if (m_gray.empty())
        return nullptr;

    return m_gray.data();
}

const unsigned char *NetpbmImage::rgb() const
{
    if (m_rgba.empty())
        return nullptr;

    return m_rgba.data();
}

void NetpbmImage::increaseRange(unsigned value)
{
    if (value > m_max)
        m_max = value;

    if (value < m_min)
        m_min = value;
}

std::ostream &operator<<(std::ostream &s, const NetpbmImage &img)
{
    s << "Portable ";
    switch (img.format()) {
    case NetpbmImage::PBM:
        s << "bitmap";
        break;
    case NetpbmImage::PGM:
        s << "graymap";
        break;
    case NetpbmImage::PPM:
        s << "color";
        break;
    }
    s << " image, " << img.width() << "x" << img.height() << " pixels";
    s << ", range " << img.min() << "-" << img.max();

    if (!img.complete())
        s << ", not complete";

    return s;
}

} // namespace vistle
