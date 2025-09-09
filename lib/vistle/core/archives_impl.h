#ifndef VISTLE_CORE_ARCHIVES_IMPL_H
#define VISTLE_CORE_ARCHIVES_IMPL_H

#include "archives_config.h"
#include "archives_compress_bigwhoop.h"
#include "archives_compress_sz3.h"
#include "archives_compress.h"

//#define USE_BOOST_ARCHIVE
#define USE_BOOST_ARCHIVE_MPI
#define USE_YAS

#ifdef USE_BOOST_ARCHIVE
#ifndef USE_BOOST_ARCHIVE_MPI
#define USE_BOOST_ARCHIVE_MPI
#endif
#endif

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>
#include <boost/core/span.hpp>

#include <vistle/util/enum.h>
#include <vistle/util/buffer.h>

#include "export.h"
#include "index.h"
#include "archives_config.h"

#ifdef USE_BOOST_ARCHIVE
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/array_wrapper.hpp>
#include <boost/serialization/array_optimization.hpp>
#endif

#ifdef USE_YAS
#include <yas/binary_iarchive.hpp>
#include <yas/binary_oarchive.hpp>
#include <yas/serialize.hpp>
#include <yas/boost_types.hpp>
#include <yas/types/concepts/array.hpp>
#include "yas_span.h"

namespace vistle {

class yas_oarchive;
class yas_iarchive;

namespace detail {

inline CompressionSettings getCompressionSettings(const CompressionSettings &cs, bool requireExact)
{
    CompressionSettings settings = cs;
    if (requireExact) {
        switch (cs.mode) {
        case Uncompressed:
            break;
        case Predict:
            break;
        default:
            settings.mode = Uncompressed;
            break;
        }
    }
    return settings;
}


template<typename T>
struct lossy_type_map {
    static const zfp_type zfptypeid = zfp_type_none;
    typedef void sz3type;
};

template<>
struct lossy_type_map<int32_t> {
    static const zfp_type zfptypeid = zfp_type_int32;
    typedef int32_t sz3type;
};
template<>
struct lossy_type_map<int64_t> {
    static const zfp_type zfptypeid = zfp_type_int64;
    typedef int64_t sz3type;
};
template<>
struct lossy_type_map<float> {
    static const zfp_type zfptypeid = zfp_type_float;
    typedef float sz3type;
};
template<>
struct lossy_type_map<double> {
    static const zfp_type zfptypeid = zfp_type_double;
    typedef double sz3type;
};

template<typename S>
struct Xor {
    S operator()(S cur, S prev) { return cur ^ prev; }
};
template<typename S>
struct Add {
    S operator()(S cur, S prev) { return cur + prev; }
};
template<typename S>
struct Sub {
    S operator()(S cur, S prev) { return cur - prev; }
};
template<typename S>
struct Ident {
    S operator()(S cur, S prev) { return cur; }
};

template<typename T>
struct PredictTransform {
    using type = T;
    using Op = Ident<T>;
    using Rop = Op;
    static constexpr bool use = false;
};
template<>
struct PredictTransform<float> {
    using type = uint32_t;
    using Op = Xor<type>;
    using Rop = Op;
    static constexpr bool use = true;
};

template<>
struct PredictTransform<double> {
    using type = uint64_t;
    using Op = Xor<type>;
    using Rop = Op;
    static constexpr bool use = true;
};
template<>
struct PredictTransform<char> {
    using type = uint8_t;
    using Op = Sub<type>;
    using Rop = Add<type>;
    static constexpr bool use = true;
};
template<>
struct PredictTransform<unsigned char> {
    using type = uint8_t;
    using Op = Sub<type>;
    using Rop = Add<type>;
    static constexpr bool use = true;
};
template<>
struct PredictTransform<signed char> {
    using type = uint8_t;
    using Op = Sub<type>;
    using Rop = Add<type>;
    static constexpr bool use = true;
};

template<>
struct PredictTransform<unsigned int> {
    using type = uint32_t;
    using Op = Sub<type>;
    using Rop = Add<type>;
    static constexpr bool use = true;
};

template<>
struct PredictTransform<unsigned long int> {
    using type = uint64_t;
    using Op = Sub<type>;
    using Rop = Add<type>;
    static constexpr bool use = true;
};
template<class T, class Op>
struct TransformStream {
    typedef typename PredictTransform<T>::type Uint;
    static_assert(sizeof(Uint) == sizeof(T), "TransformStream: Uint must have the same size as T");

    T operator()(const T &cur, const T &prev)
    {
        Uint result = Op()(*reinterpret_cast<const Uint *>(&cur), *reinterpret_cast<const Uint *>(&prev));
        T retval;
        memcpy(&retval, &result, sizeof(T));
        return retval;
    }
};

template<class T>
struct TransformStream<T, Ident<T>> {
    T operator()(const T &cur, const T &prev) { return cur; }
};
template<class T>
using CompressStream = TransformStream<T, typename PredictTransform<T>::Op>;
template<class T>
using DecompressStream = TransformStream<T, typename PredictTransform<T>::Rop>;

template<class T>
archive_helper<yas_tag>::ArrayWrapper<T>::ArrayWrapper(T *begin, T *end): m_begin(begin), m_end(end)
{}

template<class T>
T *archive_helper<yas_tag>::ArrayWrapper<T>::begin()
{
    return m_begin;
}
template<class T>
const T *archive_helper<yas_tag>::ArrayWrapper<T>::begin() const
{
    return m_begin;
}
template<class T>
T *archive_helper<yas_tag>::ArrayWrapper<T>::end()
{
    return m_end;
}
template<class T>
const T *archive_helper<yas_tag>::ArrayWrapper<T>::end() const
{
    return m_end;
}
template<class T>
bool archive_helper<yas_tag>::ArrayWrapper<T>::empty() const
{
    return m_end == m_begin;
}
template<class T>
Index archive_helper<yas_tag>::ArrayWrapper<T>::size() const
{
    return (Index)(m_end - m_begin);
}
template<class T>
T &archive_helper<yas_tag>::ArrayWrapper<T>::operator[](size_t idx)
{
    return *(m_begin + idx);
}
template<class T>
const T &archive_helper<yas_tag>::ArrayWrapper<T>::operator[](size_t idx) const
{
    return *(m_begin + idx);
}
template<class T>
void archive_helper<yas_tag>::ArrayWrapper<T>::push_back(const T &)
{
    assert("not supported" == 0);
}
template<class T>
void archive_helper<yas_tag>::ArrayWrapper<T>::resize(std::size_t sz)
{
    if (size() != sz) {
        std::cerr << "requesting resize from " << size() << " to " << sz << std::endl;
        assert("not supported" == 0);
    }
}
template<class T>
void archive_helper<yas_tag>::ArrayWrapper<T>::setDimensions(size_t sx, size_t sy, size_t sz)
{
    m_dim[0] = (Index)sx;
    m_dim[1] = (Index)sy;
    m_dim[2] = (Index)sz;
}
template<class T>
void archive_helper<yas_tag>::ArrayWrapper<T>::setExact(bool exact)
{
    m_exact = exact;
}

template<class T>
template<class Archive>
void archive_helper<yas_tag>::ArrayWrapper<T>::serialize(Archive &ar) const
{
    archive_helper<yas_tag>::ArrayWrapper<T>::save(ar);
}
template<class T>
template<class Archive>
void archive_helper<yas_tag>::ArrayWrapper<T>::serialize(Archive &ar)
{
    archive_helper<yas_tag>::ArrayWrapper<T>::load(ar);
}
template<class T>
template<class Archive>
void archive_helper<yas_tag>::ArrayWrapper<T>::load(Archive &ar)
{
    uint8_t compressMode = Uncompressed;
    ar &compressMode;
    bool compPredict = compressMode == Predict;
    bool compZfp = compressMode == Zfp;
    bool compSz3 = compressMode == SZ;
    bool compBigWhoop = compressMode == BigWhoop;

    if (compPredict) {
        yas::detail::concepts::array::load<yas_flags>(ar, *this);
        if (size() > 0) {
            std::transform(m_begin + 1, m_end, m_begin, m_begin + 1, DecompressStream<T>());
        }
    } else if (compZfp) {
        ar &m_dim[0] & m_dim[1] & m_dim[2];
        buffer compressed;
        ar &compressed;
        Index dim[3];
        for (int c = 0; c < 3; ++c)
            dim[c] = m_dim[c] == 1 ? 0 : m_dim[c];
        decompressZfp<lossy_type_map<T>::zfptypeid>(static_cast<void *>(m_begin), compressed, dim);
    } else if (compSz3) {
        ar &m_dim[0] & m_dim[1] & m_dim[2];
        buffer compressed;
        ar &compressed;
        Index dim[3];
        for (int c = 0; c < 3; ++c)
            dim[c] = m_dim[c] == 1 ? 0 : m_dim[c];
        if (!decompressSz3<typename lossy_type_map<T>::sz3type>(m_begin, compressed, dim)) {
            std::cerr << "sz3 decompression failed" << std::endl;
        }
    } else if (compBigWhoop) {
        ar &m_dim[0] & m_dim[1] & m_dim[2];
        buffer compressed;
        ar &compressed;
        if (!decompressBigWhoop<T>(m_begin, compressed.data(), 0))
            std::cerr << "BigWhoop decompression failed" << std::endl;
    } else {
        assert(compressMode == Uncompressed);
        yas::detail::concepts::array::load<yas_flags>(ar, *this);
    }
}

template<class T>
template<class Archive>
void archive_helper<yas_tag>::ArrayWrapper<T>::save(Archive &ar) const
{
    auto cs = getCompressionSettings(ar.compressionSettings(), m_exact);
    bool compPredict = PredictTransform<T>::use && (cs.mode == Predict);
    bool compSz3 = cs.mode == SZ;
    bool compZfp = cs.mode == Zfp;
    bool compBigWhoop = cs.mode == BigWhoop;
    bool compress = compPredict || compZfp || compSz3 || compBigWhoop;
    uint8_t compressMode(cs.mode);
    //std::cerr << "ar.compressed()=" << compress << std::endl;COMP_DEBUG
    if (compPredict) {
        ar &compressMode;
        std::vector<T> diff;
        diff.reserve(size());
        if (size() > 0) {
            diff.push_back(*m_begin);
            std::transform(m_begin + 1, m_end, m_begin, std::back_inserter(diff), CompressStream<T>());
        }
        yas::detail::concepts::array::save<yas_flags>(ar, diff);
    } else if (compZfp) {
        assert(!compPredict);
        assert(!compSz3);
        assert(!compBigWhoop);
        ZfpParameters param;
        param.mode = cs.zfpMode;
        param.rate = cs.zfpRate;
        param.precision = cs.zfpPrecision;
        param.accuracy = cs.zfpAccuracy;
        //std::cerr << "trying to compresss " << std::endl;
        buffer compressed;
        Index dim[3];
        dim[0] = m_dim[0];
        for (int c = 1; c < 3; ++c)
            dim[c] = m_dim[c] == 1 ? 0 : m_dim[c];
        if (compressZfp<lossy_type_map<T>::zfptypeid>(compressed, static_cast<const void *>(m_begin), dim,
                                                      sizeof(*m_begin), param)) {
            ar &compressMode;
            ar &m_dim[0] & m_dim[1] & m_dim[2];
            ar &compressed;
        } else {
            std::cerr << "zfp compression for type id " << lossy_type_map<T>::zfptypeid << " failed" << std::endl;
            compZfp = false;
            compress = false;
            compressMode = Uncompressed;
        }
    } else if (compSz3) {
        assert(!compPredict);
        assert(!compZfp);
        assert(!compBigWhoop);
        size_t outSize = 0;
        if (char *compressedData = compressSz3<typename lossy_type_map<T>::sz3type>(outSize, m_begin, m_dim, cs)) {
            boost::span compressed(compressedData, outSize);
            ar &compressMode;
            ar &m_dim[0] & m_dim[1] & m_dim[2];
            ar &compressed;
            delete[] compressedData;
        } else {
            compSz3 = false;
            compress = false;
            compressMode = Uncompressed;
        }
    } else if (compBigWhoop) {
        assert(!compPredict);
        assert(!compZfp);
        assert(!compSz3);

        std::vector<T> input(m_begin, m_end);
        buffer compressed;
        compressed.resize(m_dim[0] * m_dim[1] * m_dim[2] * cs.bigWhoopNPar * sizeof(T));
        if (size_t outSize = compressBigWhoop<T>(input.data(), m_dim, compressed.data(), cs)) {
            compressed.resize(outSize);
            ar &compressMode;
            ar &m_dim[0] & m_dim[1] & m_dim[2];
            ar &compressed;
        } else {
            compBigWhoop = false;
            compress = false;
            compressMode = Uncompressed;
        }
    }
    if (!compress) {
        ar &compressMode;
        yas::detail::concepts::array::save<yas_flags>(ar, *this);
    }
}
} // namespace detail

using detail::ZfpParameters;
using detail::compressZfp;
using detail::decompressZfp;

using detail::compressSz3;
using detail::decompressSz3;

using detail::compressBigWhoop;
using detail::decompressBigWhoop;
} // namespace vistle
#endif

#endif
