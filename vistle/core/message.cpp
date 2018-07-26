#include "message.h"
#include "messagerouter.h"
#include "shm.h"
#include "parameter.h"
#include "port.h"
#include "assert.h"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include <algorithm>

#ifdef HAVE_SNAPPY
#include <snappy.h>
#endif

#ifdef HAVE_ZSTD
#include <zstd.h>
#endif

#ifdef HAVE_LZ4
#include <lz4.h>
#endif


namespace vistle {
namespace message {

DefaultSender DefaultSender::s_instance;

DefaultSender::DefaultSender()
: m_id(Id::Invalid)
, m_rank(-1)
{
   Router::the();
}

int DefaultSender::id() {

   return s_instance.m_id;
}

int DefaultSender::rank() {

   return s_instance.m_rank;
}

void DefaultSender::init(int id, int rank) {

   s_instance.m_id = id;
   s_instance.m_rank = rank;
}

const DefaultSender &DefaultSender::instance() {

   return s_instance;
}

static boost::uuids::random_generator s_uuidGenerator;

Message::Message(const Type t, const unsigned int s)
: m_type(t)
, m_size(s)
, m_senderId(DefaultSender::id())
, m_rank(DefaultSender::rank())
, m_destId(Id::NextHop)
, m_destRank(-1)
, m_uuid(t == ANY ? boost::uuids::nil_generator()() : s_uuidGenerator())
, m_referrer(boost::uuids::nil_generator()())
, m_payloadSize(0)
, m_payloadRawSize(0)
, m_payloadCompression(CompressionNone)
, m_broadcast(false)
, m_notification(false)
, m_pad{0,0}
{

   vassert(m_size <= MESSAGE_SIZE);
   vassert(m_type > INVALID);
   vassert(m_type < NumMessageTypes);
}

unsigned long Message::typeFlags() const {

   vassert(type() > ANY && type() > INVALID && type() < NumMessageTypes);
   if (type() <= ANY || type() <= INVALID) {
      return 0;
   }
   if (type() >= NumMessageTypes) {
      return 0;
   }

   return Router::rt[type()];
}

const uuid_t &Message::uuid() const {

   return m_uuid;
}

void Message::setUuid(const uuid_t &uuid) {

   m_uuid = uuid;
}

const uuid_t &Message::referrer() const {

   return m_referrer;
}

void Message::setReferrer(const uuid_t &uuid) {

   m_referrer = uuid;
}

int Message::senderId() const {

   return m_senderId;
}

void Message::setSenderId(int id) {

   m_senderId = id;
}

int Message::destId() const {

   return m_destId;
}

void Message::setDestId(int id) {

   m_destId = id;
}

int Message::destRank() const {
   return m_destRank;
}

void Message::setDestRank(int r) {
    m_destRank = r;
}

int Message::destUiId() const {
    return -m_destRank;
}

void Message::setDestUiId(int id) {
    m_destRank = -id;
}

int Message::rank() const {

   return m_rank;
}

void Message::setRank(int rank) {
   
    m_rank = rank;
}

int Message::uiId() const
{
    return -m_rank;
}

Type Message::type() const {

   return m_type;
}

size_t Message::size() const {

   return m_size;
}

size_t Message::payloadSize() const {
   return m_payloadSize;
}

void Message::setPayloadSize(size_t size) {
    m_payloadSize = size;
}

CompressionMode Message::payloadCompression() const {
   return CompressionMode(m_payloadCompression);
}

void Message::setPayloadCompression(CompressionMode mode) {
   m_payloadCompression = mode;
}

size_t Message::payloadRawSize() const {
    return m_payloadRawSize;
}

void Message::setPayloadRawSize(size_t size) {
    m_payloadRawSize = size;
}

bool Message::isBroadcast() const {

    return m_broadcast;
}

void Message::setBroadcast(bool enable) {
    m_broadcast = enable;
}

bool Message::isNotification() const {

    return m_notification;
}

void Message::setNotify(bool enable) {

    m_notification = enable;
}

std::vector<char> compressPayload(CompressionMode mode, Message &msg, std::vector<char> &raw, int speed) {

    std::vector<char> compressed;
    msg.setPayloadRawSize(raw.size());
    switch (mode) {
#ifdef HAVE_SNAPPY
    case CompressionSnappy: {
        size_t maxsize = snappy::MaxCompressedLength(raw.size());
        compressed.resize(maxsize);
        size_t compressedSize = 0;
        snappy::RawCompress(raw.data(), raw.size(), compressed.data(), &compressedSize);
        compressed.resize(compressedSize);
        msg.setPayloadCompression(CompressionSnappy);
        break;
    }
#endif
#ifdef HAVE_ZSTD
    case CompressionZstd: {
        size_t maxsize = ZSTD_compressBound(raw.size());
        compressed.resize(maxsize);
        size_t compressedSize = ZSTD_compress(compressed.data(), compressed.size(), raw.data(), raw.size(), speed);
        if (!ZSTD_isError(compressedSize)) {
            compressed.resize(compressedSize);
            msg.setPayloadCompression(CompressionZstd);
        } else {
            compressed = std::move(raw);
            msg.setPayloadCompression(CompressionNone);
        }
        break;
    }
#endif
#ifdef HAVE_LZ4
    case CompressionLz4: {
        size_t maxsize = LZ4_compressBound(raw.size());
        compressed.resize(maxsize);
        size_t compressedSize = LZ4_compress_fast(raw.data(), compressed.data(), raw.size(), compressed.size(), speed);
        if (compressedSize > 0) {
            compressed.resize(compressedSize);
            msg.setPayloadCompression(CompressionLz4);
        } else {
            compressed = std::move(raw);
            msg.setPayloadCompression(CompressionNone);
        }
        break;
    }
#endif
    default:
        compressed = std::move(raw);
        msg.setPayloadCompression(CompressionNone);
        break;
    }

    msg.setPayloadSize(compressed.size());
    std::cerr << "compressPayload " << msg.payloadRawSize() << " -> " << msg.payloadSize() << std::endl;
    return compressed;
}

std::vector<char> decompressPayload(const Message &msg, std::vector<char> &compressed) {

    if (msg.payloadCompression() == CompressionNone)
        return std::move(compressed);

    assert(compressed.size() >= msg.payloadSize());

    std::vector<char> decompressed(msg.payloadRawSize());
    switch (msg.payloadCompression()) {
    case CompressionSnappy: {
#ifdef HAVE_SNAPPY

        snappy::RawUncompress(compressed.data(), compressed.size(), decompressed.data());
#else
        throw vistle::exception("cannot decompress Snappy");
#endif
        break;
    }
    case CompressionZstd: {
#ifdef HAVE_ZSTD
        size_t n = ZSTD_decompress(decompressed.data(), decompressed.size(), compressed.data(), msg.payloadSize());
        if (ZSTD_isError(n)) {
            throw vistle::exception("Zstd decompression failed");
        }
        assert(n == msg.payloadRawSize());
#else
        throw vistle::exception("cannot decompress Zstd");
#endif
        break;
    }
    case CompressionLz4: {
#ifdef HAVE_LZ4
        int n = LZ4_decompress_fast(compressed.data(), decompressed.data(), decompressed.size());
        if (n < 0) {
            throw vistle::exception("LZ4 decompression failed");
        }
        assert(n == msg.payloadSize());
#else
        throw vistle::exception("cannot decompress LZ4");
#endif
        break;
    }
    }

    return decompressed;
}

} // namespace message
} // namespace vistle
