/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// binary_oarchive.cpp:
// binary_iarchive.cpp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <ostream>
#include <istream>

#include "archives_config.h"
#include "archives_impl.h"
#include "archives.h"
#include "object.h"

#include "shmvector.h"

#ifdef USE_BOOST_ARCHIVE
#define BOOST_ARCHIVE_SOURCE
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/detail/archive_serializer_map.hpp>

// explicitly instantiate for this type of binary stream
#include <boost/archive/impl/archive_serializer_map.ipp>
#include <boost/archive/impl/basic_binary_oprimitive.ipp>
#include <boost/archive/impl/basic_binary_iprimitive.ipp>
#include <boost/archive/impl/basic_binary_oarchive.ipp>
#include <boost/archive/impl/basic_binary_iarchive.ipp>
#endif

//#include <boost/mpl/for_each.hpp>

#ifdef USE_BOOST_ARCHIVE
namespace ba = boost::archive;

//#define COMP_DEBUG

namespace boost {
namespace archive {

template class V_COREEXPORT detail::archive_serializer_map<vistle::boost_oarchive>;
template class detail::common_oarchive<vistle::boost_oarchive>;
template class basic_binary_oprimitive<vistle::boost_oarchive, std::ostream::char_type, std::ostream::traits_type>;

template class basic_binary_oarchive<vistle::boost_oarchive>;
template class binary_oarchive_impl<vistle::boost_oarchive, std::ostream::char_type, std::ostream::traits_type>;


// explicitly instantiate for this type of stream
template class V_COREEXPORT detail::archive_serializer_map<vistle::boost_iarchive>;
template class basic_binary_iprimitive<vistle::boost_iarchive, std::istream::char_type, std::istream::traits_type>;
template class basic_binary_iarchive<vistle::boost_iarchive>;
template class binary_iarchive_impl<vistle::boost_iarchive, std::istream::char_type, std::istream::traits_type>;

} // namespace archive
} // namespace boost
#endif


namespace vistle {

Saver::~Saver()
{}

#ifdef USE_BOOST_ARCHIVE
boost_oarchive::boost_oarchive(std::streambuf &bsb, unsigned int flags)
: boost::archive::binary_oarchive_impl<boost_oarchive, std::ostream::char_type, std::ostream::traits_type>(bsb, flags)
{}

boost_oarchive::~boost_oarchive()
{}

void boost_oarchive::setCompressionSettings(const CompressionSettings &other)
{}

void boost_oarchive::setCompressionMode(FieldCompressionMode mode)
{}

void boost_oarchive::setSaver(std::shared_ptr<Saver> saver)
{
    m_saver = saver;
}
#endif


Fetcher::~Fetcher()
{}

bool Fetcher::renameObjects() const
{
    return false;
}

std::string Fetcher::translateObjectName(const std::string &name) const
{
    assert(!renameObjects());
    return name;
}

std::string Fetcher::translateArrayName(const std::string &name) const
{
    assert(!renameObjects());
    return name;
}

void Fetcher::registerObjectNameTranslation(const std::string &arname, const std::string &name)
{
    assert(!renameObjects());
}

void Fetcher::registerArrayNameTranslation(const std::string &arname, const std::string &name)
{
    assert(!renameObjects());
}

#ifdef USE_YAS
void yas_oarchive::setCompressionMode(FieldCompressionMode mode)
{
    m_compress = mode;
}

FieldCompressionMode yas_oarchive::compressionMode() const
{
    return m_compress;
}

void yas_oarchive::setZfpRate(double rate)
{
    m_zfpRate = rate;
}

double yas_oarchive::zfpRate() const
{
    return m_zfpRate;
}

void yas_oarchive::setZfpAccuracy(double accuracy)
{
    m_zfpAccuracy = accuracy;
}

double yas_oarchive::zfpAccuracy() const
{
    return m_zfpAccuracy;
}

void yas_oarchive::setZfpPrecision(int precision)
{
    m_zfpPrecision = precision;
}

int yas_oarchive::zfpPrecision() const
{
    return m_zfpPrecision;
}

void yas_oarchive::setCompressionSettings(const CompressionSettings &other)
{
    m_compress = other.m_compress;
    m_zfpRate = other.m_zfpRate;
    m_zfpPrecision = other.m_zfpPrecision;
    m_zfpAccuracy = other.m_zfpAccuracy;
}

yas_oarchive::yas_oarchive(yas_oarchive::Stream &mo, unsigned int flags): yas_oarchive::Base(mo), m_os(mo)
{}

void yas_oarchive::setSaver(std::shared_ptr<Saver> saver)
{
    m_saver = saver;
}

yas_iarchive::yas_iarchive(yas_iarchive::Stream &mi, unsigned int flags): yas_iarchive::Base(mi)
{}

yas_iarchive::~yas_iarchive()
{
    setCurrentObject(nullptr);
}

std::string yas_iarchive::translateObjectName(const std::string &name) const
{
    if (!m_fetcher)
        return name;
    return m_fetcher->translateObjectName(name);
}

std::string yas_iarchive::translateArrayName(const std::string &name) const
{
    if (!m_fetcher)
        return name;
    return m_fetcher->translateArrayName(name);
}

void yas_iarchive::registerObjectNameTranslation(const std::string &arname, const std::string &name) const
{
    if (!m_fetcher)
        return;
    m_fetcher->registerObjectNameTranslation(arname, name);
}

void yas_iarchive::registerArrayNameTranslation(const std::string &arname, const std::string &name) const
{
    if (!m_fetcher)
        return;
    m_fetcher->registerArrayNameTranslation(arname, name);
}

void yas_iarchive::setFetcher(std::shared_ptr<Fetcher> fetcher)
{
    m_fetcher = fetcher;
}

void yas_iarchive::setCurrentObject(ObjectData *data)
{
    if (data)
        data->ref();
    if (m_currentObject)
        m_currentObject->unref();
    m_currentObject = data;
}

ObjectData *yas_iarchive::currentObject() const
{
    return m_currentObject;
}

std::shared_ptr<Fetcher> yas_iarchive::fetcher() const
{
    return m_fetcher;
}

void yas_iarchive::setObjectCompletionHandler(const std::function<void()> &completer)
{
    m_completer = completer;
}

const std::function<void()> &yas_iarchive::objectCompletionHandler() const
{
    return m_completer;
}

obj_const_ptr yas_iarchive::getObject(const std::string &arname, const ObjectCompletionHandler &completeCallback) const
{
    std::string name = arname;
    if (m_fetcher)
        name = m_fetcher->translateObjectName(arname);
    //std::cerr << "yas_iarchive::getObject(arname=" << arname << ", name=" << name << ")" << std::endl;
    auto obj = Shm::the().getObjectFromName(name);
    if (!obj) {
        assert(m_fetcher);
        m_fetcher->requestObject(arname, completeCallback);
        obj = Shm::the().getObjectFromName(name);
    }
    return obj;
}
#endif


#ifdef USE_BOOST_ARCHIVE
boost_iarchive::boost_iarchive(std::streambuf &bsb, unsigned int flags): Base(bsb, flags), m_currentObject(nullptr)
{}

boost_iarchive::~boost_iarchive()
{
    setCurrentObject(nullptr);
}

std::string boost_iarchive::translateObjectName(const std::string &name) const
{
    if (!m_fetcher)
        return name;
    return m_fetcher->translateObjectName(name);
}

std::string boost_iarchive::translateArrayName(const std::string &name) const
{
    if (!m_fetcher)
        return name;
    return m_fetcher->translateArrayName(name);
}

void boost_iarchive::registerObjectNameTranslation(const std::string &arname, const std::string &name) const
{
    if (!m_fetcher)
        return;
    m_fetcher->registerObjectNameTranslation(arname, name);
}

void boost_iarchive::registerArrayNameTranslation(const std::string &arname, const std::string &name) const
{
    if (!m_fetcher)
        return;
    m_fetcher->registerArrayNameTranslation(arname, name);
}

void boost_iarchive::setFetcher(std::shared_ptr<Fetcher> fetcher)
{
    m_fetcher = fetcher;
}

void boost_iarchive::setCurrentObject(ObjectData *data)
{
    if (data)
        data->ref();
    if (m_currentObject)
        m_currentObject->unref();
    m_currentObject = data;
}

ObjectData *boost_iarchive::currentObject() const
{
    return m_currentObject;
}

std::shared_ptr<Fetcher> boost_iarchive::fetcher() const
{
    return m_fetcher;
}

obj_const_ptr boost_iarchive::getObject(const std::string &name, const ObjectCompletionHandler &completeCallback) const
{
    auto obj = Shm::the().getObjectFromName(name);
    if (!obj) {
        assert(m_fetcher);
        m_fetcher->requestObject(name, completeCallback);
        obj = Shm::the().getObjectFromName(name);
    }
    return obj;
}

void boost_iarchive::setObjectCompletionHandler(const std::function<void()> &completer)
{
    m_completer = completer;
}

const std::function<void()> &boost_iarchive::objectCompletionHandler() const
{
    return m_completer;
}
#endif

#ifdef USE_YAS
#ifdef HAVE_ZFP
namespace detail {

template<zfp_type type>
bool decompressZfp(void *dest, const buffer &compressed, const Index dim[3])
{
#ifdef HAVE_ZFP
    bool ok = true;
    bitstream *stream = stream_open(const_cast<char *>(compressed.data()), compressed.size());
    zfp_stream *zfp = zfp_stream_open(stream);
    zfp_stream_rewind(zfp);
    zfp_field *field = zfp_field_3d(dest, type, dim[0], dim[1], dim[2]);
    if (!zfp_read_header(zfp, field, ZFP_HEADER_FULL)) {
        std::cerr << "decompressZfp: reading zfp compression parameters failed" << std::endl;
    }
    if (field->type != type) {
        std::cerr << "decompressZfp: zfp type not compatible" << std::endl;
    }
    if (field->nx != dim[0] || field->ny != dim[1] || field->nz != dim[2]) {
        std::cerr << "decompressZfp: zfp size mismatch: " << field->nx << "x" << field->ny << "x" << field->nz
                  << " != " << dim[0] << "x" << dim[1] << "x" << dim[2] << std::endl;
    }
    zfp_field_set_pointer(field, dest);
    if (!zfp_decompress(zfp, field)) {
        std::cerr << "decompressZfp: zfp decompression failed" << std::endl;
        ok = false;
    }
    zfp_stream_close(zfp);
    zfp_field_free(field);
    stream_close(stream);
    return ok;
#else
    std::cerr << "cannot decompress array: no support for ZFP floating point compression" << std::endl;
    return false;
#endif
}

template<>
bool decompressZfp<zfp_type_none>(void *dest, const buffer &compressed, const Index dim[3])
{
    return false;
}

template bool decompressZfp<zfp_type_int32>(void *dest, const buffer &compressed, const Index dim[3]);
template bool decompressZfp<zfp_type_int64>(void *dest, const buffer &compressed, const Index dim[3]);
template bool decompressZfp<zfp_type_float>(void *dest, const buffer &compressed, const Index dim[3]);
template bool decompressZfp<zfp_type_double>(void *dest, const buffer &compressed, const Index dim[3]);

template<zfp_type type>
bool compressZfp(buffer &compressed, const void *src, const Index dim[3], const Index typeSize,
                 const ZfpParameters &param)
{
#ifdef HAVE_ZFP
    bool ok = true;
    int ndims = 1;
    size_t sz = dim[0];
    if (dim[1] != 0) {
        ndims = 2;
        sz *= dim[1];
    }
    if (dim[2] != 0) {
        ndims = 3;
        sz *= dim[2];
    }

    if (sz < 1000) {
#ifdef COMP_DEBUG
        std::cerr << "compressZfp: not compressing - fewer than 1000 elements" << std::endl;
#endif
        return false;
    }

    zfp_stream *zfp = zfp_stream_open(nullptr);
    switch (param.mode) {
    case Uncompressed:
    case Predict:
        assert("invalid mode for ZFP compression" == 0);
        break;
    case ZfpAccuracy:
        zfp_stream_set_accuracy(zfp, param.accuracy);
        break;
    case ZfpPrecision:
        zfp_stream_set_precision(zfp, param.precision);
        break;
    case ZfpFixedRate:
        zfp_stream_set_rate(zfp, param.rate, type, ndims, 0);
        break;
    }

    zfp_field *field = zfp_field_3d(const_cast<void *>(src), type, dim[0], dim[1], dim[2]);
    size_t bufsize = zfp_stream_maximum_size(zfp, field);
    compressed.resize(bufsize);
    bitstream *stream = stream_open(compressed.data(), bufsize);
    zfp_stream_set_bit_stream(zfp, stream);

    zfp_stream_rewind(zfp);
    size_t header = zfp_write_header(zfp, field, ZFP_HEADER_FULL);
#ifdef COMP_DEBUG
    std::cerr << "compressZfp: wrote " << header << " header bytes" << std::endl;
#endif
    size_t zfpsize = zfp_compress(zfp, field);
    if (zfpsize == 0) {
        std::cerr << "compressZfp: zfp compression failed" << std::endl;
        ok = false;
    } else {
        compressed.resize(zfpsize);
#ifdef COMP_DEBUG
        std::cerr << "compressZfp: compressed " << dim[0] << "x" << dim[1] << "x" << dim[2] << " elements/"
                  << sz * sizeof(float) << " to " << zfpsize * typeSize << " bytes" << std::endl;
#endif
    }
    zfp_field_free(field);
    zfp_stream_close(zfp);
    stream_close(stream);

    return ok;
#else
    assert("no support for ZFP floating point compression" == 0);
    return false;
#endif
}

template<>
bool compressZfp<zfp_type_none>(buffer &compressed, const void *src, const Index dim[3], Index typeSize,
                                const ZfpParameters &param)
{
    return false;
}
template bool compressZfp<zfp_type_int32>(buffer &compressed, const void *src, const Index dim[3], Index typeSize,
                                          const ZfpParameters &param);
template bool compressZfp<zfp_type_int64>(buffer &compressed, const void *src, const Index dim[3], Index typeSize,
                                          const ZfpParameters &param);
template bool compressZfp<zfp_type_float>(buffer &compressed, const void *src, const Index dim[3], Index typeSize,
                                          const ZfpParameters &param);
template bool compressZfp<zfp_type_double>(buffer &compressed, const void *src, const Index dim[3], Index typeSize,
                                           const ZfpParameters &param);

} // namespace detail
#endif
#endif
} // namespace vistle
