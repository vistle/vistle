#ifndef VISTLE_CORE_ARCHIVES_IMPL_H
#define VISTLE_CORE_ARCHIVES_IMPL_H

#include "archives_config.h"
#include "archives_compress_bigwhoop.h"
#include "archives_compress_sz3.h"
#include "archives_compress.h"

//#define USE_INTROSPECTION_ARCHIVE
//#define USE_BOOST_ARCHIVE
#define USE_BOOST_ARCHIVE_MPI
#define USE_YAS

#ifdef USE_INTROSPECTION_ARCHIVE
#ifndef USE_BOOST_ARCHIVE
#define USE_BOOST_ARCHIVE
#endif
#endif

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
#ifdef HAVE_ZFP
    static const zfp_type zfptypeid = zfp_type_none;
#endif
    typedef void sz3type;
};

template<>
struct lossy_type_map<int32_t> {
#ifdef HAVE_ZFP
    static const zfp_type zfptypeid = zfp_type_int32;
#endif
    typedef int32_t sz3type;
};
template<>
struct lossy_type_map<int64_t> {
#ifdef HAVE_ZFP
    static const zfp_type zfptypeid = zfp_type_int64;
#endif
    typedef int64_t sz3type;
};
template<>
struct lossy_type_map<float> {
#ifdef HAVE_ZFP
    static const zfp_type zfptypeid = zfp_type_float;
#endif
    typedef float sz3type;
};
template<>
struct lossy_type_map<double> {
#ifdef HAVE_ZFP
    static const zfp_type zfptypeid = zfp_type_double;
#endif
    typedef double sz3type;
};

template<typename S>
struct Xor {
    S operator()(S prev, S cur) { return cur ^ prev; }
};
template<typename S>
struct Add {
    S operator()(S prev, S cur) { return cur + prev; }
};
template<typename S>
struct Sub {
    S operator()(S prev, S cur) { return cur - prev; }
};
template<typename S>
struct Ident {
    S operator()(S prev, S cur) { return cur; }
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
    static constexpr bool use = false;
};

template<>
struct PredictTransform<double> {
    using type = uint64_t;
    using Op = Xor<type>;
    using Rop = Op;
    static constexpr bool use = false;
};
template<>
struct PredictTransform<char> {
    using type = uint8_t;
    using Op = Sub<type>;
    using Rop = Add<type>;
    static constexpr bool use = false;
};
template<>
struct PredictTransform<unsigned char> {
    using type = uint8_t;
    using Op = Sub<type>;
    using Rop = Add<type>;
    static constexpr bool use = false;
};
template<>
struct PredictTransform<signed char> {
    using type = uint8_t;
    using Op = Sub<type>;
    using Rop = Add<type>;
    static constexpr bool use = false;
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
template<class T, class Op, bool reverse>
struct TransformStream {
    typedef typename PredictTransform<T>::type Int;

    Int prev;
    bool first = true;
    Int operator()(const T &t)
    {
        if (first) {
            first = false;
            prev = *reinterpret_cast<const Int *>(&t);
            return prev;
        }
        Int result = Op()(prev, *reinterpret_cast<const Int *>(&t));
        prev = reverse ? result : *reinterpret_cast<const Int *>(&t);
        return result;
    }
};

template<class T>
struct TransformStream<T, Ident<T>, false> {
    typedef T Int;
    Int operator()(const T &t) { return t; }
};
template<class T>
struct TransformStream<T, Ident<T>, true> {
    typedef T Int;
    Int operator()(const T &t) { return t; }
};

template<class T>
using CompressStream = TransformStream<T, typename PredictTransform<T>::Op, false>;
template<class T>
using DecompressStream = TransformStream<T, typename PredictTransform<T>::Rop, true>;

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
        std::transform(m_begin, m_end, m_begin, DecompressStream<T>());
    } else if (compZfp) {
        ar &m_dim[0] & m_dim[1] & m_dim[2];
        buffer compressed;
        ar &compressed;
#ifdef HAVE_ZFP
        Index dim[3];
        for (int c = 0; c < 3; ++c)
            dim[c] = m_dim[c] == 1 ? 0 : m_dim[c];
        decompressZfp<lossy_type_map<T>::zfptypeid>(static_cast<void *>(m_begin), compressed, dim);
#endif
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
        std::vector<T, allocator<T>> compressed;
        ar &compressed;
        // TODO: find better default value
        uint8_t layer = 0;
        decompressBigWhoop<T>(compressed.data(), m_begin, layer);
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
        std::transform(m_begin, m_end, std::back_inserter(diff), CompressStream<T>());
        yas::detail::concepts::array::save<yas_flags>(ar, diff);
    } else if (compZfp) {
        assert(!compPredict);
        assert(!compSz3);
        assert(!compBigWhoop);
#ifdef HAVE_ZFP
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
#else
        compZfp = false;
        compress = false;
        compressMode = Uncompressed;
#endif
    } else if (compSz3) {
        assert(!compPredict);
        assert(!compZfp);
        assert(!compBigWhoop);
        size_t outSize = 0;
        std::vector<T> input(m_begin, m_end);
        if (char *compressedData = compressSz3<typename lossy_type_map<T>::sz3type>(outSize, input.data(), m_dim, cs)) {
            buffer compressed(compressedData, compressedData + outSize);
            delete[] compressedData;
            ar &compressMode;
            ar &m_dim[0] & m_dim[1] & m_dim[2];
            ar &compressed;
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
        std::vector<T, allocator<T>> compressed;
        compressed.resize(input.size());
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

#ifdef HAVE_ZFP
using detail::ZfpParameters;
using detail::compressZfp;
using detail::decompressZfp;
#endif

using detail::compressSz3;
using detail::decompressSz3;

using detail::compressBigWhoop;
using detail::decompressBigWhoop;
} // namespace vistle
#endif

#endif
