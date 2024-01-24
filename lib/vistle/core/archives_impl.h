#ifndef VISTLE_ARCHIVES_IMPL_H
#define VISTLE_ARCHIVES_IMPL_H

#include "archives_config.h"
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
#if BOOST_VERSION >= 106400
#include <boost/serialization/array_wrapper.hpp>
#include <boost/serialization/array_optimization.hpp>
#endif
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

template<typename T>
struct lossy_type_map {
#ifdef HAVE_ZFP
    static const zfp_type zfptypeid = zfp_type_none;
#endif
#ifdef HAVE_SZ3
    typedef void sz3type;
#endif
};

template<>
struct lossy_type_map<int32_t> {
#ifdef HAVE_ZFP
    static const zfp_type zfptypeid = zfp_type_int32;
#endif
#ifdef HAVE_SZ3
    typedef int32_t sz3type;
#endif
};
template<>
struct lossy_type_map<int64_t> {
#ifdef HAVE_ZFP
    static const zfp_type zfptypeid = zfp_type_int64;
#endif
#ifdef HAVE_SZ3
    typedef int64_t sz3type;
#endif
};
template<>
struct lossy_type_map<float> {
#ifdef HAVE_ZFP
    static const zfp_type zfptypeid = zfp_type_float;
#endif
#ifdef HAVE_SZ3
    typedef float sz3type;
#endif
};
template<>
struct lossy_type_map<double> {
#ifdef HAVE_ZFP
    static const zfp_type zfptypeid = zfp_type_double;
#endif
#ifdef HAVE_SZ3
    typedef double sz3type;
#endif
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
    bool compPredict = false;
    bool compZfp = false;
    bool compSz3 = false;
    bool compress = false;
    ar &compress;
    if (compress) {
        ar &compPredict;
        if (!compPredict) {
            ar &compZfp;
            compSz3 = !compZfp;
        }
    }
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
#ifdef HAVE_SZ3
        Index dim[3];
        for (int c = 0; c < 3; ++c)
            dim[c] = m_dim[c] == 1 ? 0 : m_dim[c];
        decompressSz3<typename lossy_type_map<T>::sz3type>(m_begin, compressed, dim);
#endif
    } else {
        yas::detail::concepts::array::load<yas_flags>(ar, *this);
    }
}

template<class T>
template<class Archive>
void archive_helper<yas_tag>::ArrayWrapper<T>::save(Archive &ar) const
{
    auto cs = ar.compressionSettings();
    bool compPredict = PredictTransform<T>::use && (cs.mode == Predict || (m_exact && cs.mode != Uncompressed));
    bool compSz3 = !m_exact && !compPredict && cs.mode == SZ;
    bool compZfp = !m_exact && !compPredict && cs.mode == Zfp;
    bool compress = compPredict || compZfp || compSz3;
    //std::cerr << "ar.compressed()=" << compress << std::endl;
    if (compPredict) {
        ar &compress;
        ar &compPredict;
        std::vector<T> diff;
        diff.reserve(size());
        std::transform(m_begin, m_end, std::back_inserter(diff), CompressStream<T>());
        yas::detail::concepts::array::save<yas_flags>(ar, diff);
    } else if (compZfp) {
        assert(!compPredict);
        assert(!compSz3);
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
            ar &compress;
            ar &compPredict;
            ar &compZfp;
            ar &m_dim[0] & m_dim[1] & m_dim[2];
            ar &compressed;
        } else {
            std::cerr << "zfp compression for type id " << lossy_type_map<T>::zfptypeid << " failed" << std::endl;
            compZfp = false;
            compress = false;
        }
#else
        compZfp = false;
        compress = false;
#endif
    } else if (compSz3) {
        assert(!compPredict);
        assert(!compZfp);
#ifdef HAVE_SZ3
        std::vector<size_t> dims;
        for (int c = 0; c < 3; ++c)
            dims.push_back(m_dim[c]);
        while (dims.back() == 1 && dims.size() > 1)
            dims.pop_back();
        SZ3::Config conf;
        if (dims.size() == 1)
            conf = SZ3::Config(dims[0]);
        else if (dims.size() == 2)
            conf = SZ3::Config(dims[0], dims[1]);
        else if (dims.size() == 3)
            conf = SZ3::Config(dims[0], dims[1], dims[2]);
        switch (cs.szAlgo) {
        case SzInterp:
            conf.cmprAlgo = SZ3::ALGO_INTERP;
            break;
        case SzInterpLorenzo:
            conf.cmprAlgo = SZ3::ALGO_INTERP_LORENZO;
            break;
        case SzLorenzoReg:
            conf.cmprAlgo = SZ3::ALGO_LORENZO_REG;
            break;
        }
        switch (cs.szError) {
        case SzAbs:
            conf.errorBoundMode = SZ3::EB_ABS;
            break;
        case SzRel:
            conf.errorBoundMode = SZ3::EB_REL;
            break;
        case SzPsnr:
            conf.errorBoundMode = SZ3::EB_PSNR;
            break;
        case SzL2:
            conf.errorBoundMode = SZ3::EB_L2NORM;
            break;
        case SzAbsAndRel:
            conf.errorBoundMode = SZ3::EB_ABS_AND_REL;
            break;
        case SzAbsOrRel:
            conf.errorBoundMode = SZ3::EB_ABS_OR_REL;
            break;
        }
        conf.absErrorBound = cs.szAbsError;
        conf.relErrorBound = cs.szRelError;
        conf.psnrErrorBound = cs.szPsnrError;
        conf.l2normErrorBound = cs.szL2Error;
        conf.encoder = 0;
        conf.lossless = 0;
        size_t outSize = 0;
        std::vector<T> input(m_begin, m_end);
        char *compressedData = compressSz3<typename lossy_type_map<T>::sz3type>(outSize, input.data(), conf);
        buffer compressed(compressedData, compressedData + outSize);
        delete[] compressedData;
        ar &compress;
        ar &compPredict;
        ar &compZfp;
        ar &m_dim[0] & m_dim[1] & m_dim[2];
        ar &compressed;
#else
        compSz3 = false;
        compress = false;
#endif
    }
    if (!compress) {
        ar &compress;
        yas::detail::concepts::array::save<yas_flags>(ar, *this);
    }
}
} // namespace detail

#if 0
namespace detail {

#ifdef HAVE_SZ3
extern template char *compressSz3<void>(size_t &compressedSize, const void *src, const SZ3::Config &conf);
extern template char *compressSz3<float>(size_t &compressedSize, const float *src, const SZ3::Config &conf);
extern template char *compressSz3<double>(size_t &compressedSize, const double *src, const SZ3::Config &conf);
extern template char *compressSz3<int32_t>(size_t &compressedSize, const int32_t *src, const SZ3::Config &conf);
extern template char *compressSz3<int64_t>(size_t &compressedSize, const int64_t *src, const SZ3::Config &conf);

extern template bool V_COREEXPORT decompressSz3<void>(void *dest, const buffer &compressed, const Index dim[3]);
extern template bool V_COREEXPORT decompressSz3<float>(float *dest, const buffer &compressed, const Index dim[3]);
extern template bool V_COREEXPORT decompressSz3<double>(double *dest, const buffer &compressed, const Index dim[3]);
extern template bool V_COREEXPORT decompressSz3<int32_t>(int32_t *dest, const buffer &compressed, const Index dim[3]);
extern template bool V_COREEXPORT decompressSz3<int64_t>(int64_t *dest, const buffer &compressed, const Index dim[3]);
#endif

} // namespace detail
#endif

#ifdef HAVE_ZFP
using detail::ZfpParameters;
using detail::compressZfp;
using detail::decompressZfp;
#endif

#ifdef HAVE_SZ3
using detail::compressSz3;
using detail::decompressSz3;
#endif
} // namespace vistle
#endif

#endif
