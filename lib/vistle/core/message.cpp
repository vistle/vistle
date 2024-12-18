#include "message.h"
#include "messagerouter.h"
#include "shm.h"
#include "parameter.h"
#include "port.h"
#include <cassert>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <algorithm>

#ifdef HAVE_ZSTD
#include <zstd.h>
#endif

#ifdef HAVE_LZ4
#include <lz4.h>
#endif


namespace vistle {
namespace message {

bool Id::isHub(int id)
{
    return id <= Id::LocalHub;
}

bool Id::isModule(int id)
{
    return id >= Id::ModuleBase;
}

std::string Id::toString(Reserved id)
{
    switch (id) {
    case Invalid:
        return "Invalid";
    case Vistle:
        return "Vistle session";
    case Config:
        return "Per-workflow settings";
    case Broadcast:
        return "Broadcast";
    case ForBroadcast:
        return "For broadcast";
    case NextHop:
        return "Next hop";
    case UI:
        return "User interface";
    case LocalManager:
        return "Local cluster manager";
    case LocalHub:
        return "Local hub";
    case MasterHub:
        return "Master hub";
    case ModuleBase:
        return "Modules";
    }

    return "ERROR";
}

std::string Id::toString(int id)
{
    if (isModule(id)) {
        return "Module " + std::to_string(id);
    }
    if (id >= MasterHub && id < ModuleBase) {
        return toString(static_cast<Reserved>(id));
    }
    if (isHub(id) && id != MasterHub) {
        return "Client hub " + std::to_string(MasterHub - id - 1);
    }

    return std::to_string(id);
}

std::string Id::name(const std::string &desc, int id)
{
    if (isModule(id)) {
        return desc + "_" + std::to_string(id);
    }
    switch (id) {
    case Vistle:
    case Config:
    case Broadcast:
    case ForBroadcast:
    case NextHop:
    case UI:
    case LocalManager:
    case LocalHub:
    case MasterHub:
        return toString(static_cast<Reserved>(id));
    }

    if (isHub(id)) {
        return desc + " " + std::to_string(MasterHub - id - 1);
    }

    return desc + "_" + std::to_string(id);
}

codec_error::codec_error(const std::string &what): vistle::exception(what)
{
    std::string codecs;
#ifdef HAVE_ZSTD
    codecs += " ";
    codecs += "Zstd";
#endif
#ifdef HAVE_LZ4
    codecs += " ";
    codecs += "LZ4";
#endif

    m_info += "\n";
    if (codecs.empty()) {
        m_info += "No compression codec support";
    } else {
        m_info += "Supported codecs:";
        m_info += codecs;
    }
}

DefaultSender DefaultSender::s_instance;

DefaultSender::DefaultSender(): m_id(Id::Invalid), m_rank(-1)
{
    Router::the();
}

int DefaultSender::id()
{
    return s_instance.m_id;
}

int DefaultSender::rank()
{
    return s_instance.m_rank;
}

void DefaultSender::init(int id, int rank)
{
    s_instance.m_id = id;
    s_instance.m_rank = rank;
}

const DefaultSender &DefaultSender::instance()
{
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
, m_priority(0)
, m_forBroadcast(false)
, m_wasBroadcast(false)
, m_notification(false)
, m_pad{0}
{
    assert(m_size <= MESSAGE_SIZE);
    assert(m_type > INVALID);
    assert(m_type < NumMessageTypes);
}

unsigned long Message::typeFlags() const
{
    assert(type() > ANY && type() > INVALID && type() < NumMessageTypes);
    if (type() <= ANY || type() <= INVALID) {
        return 0;
    }
    if (type() >= NumMessageTypes) {
        return 0;
    }

    return Router::rt[type()];
}

const uuid_t &Message::uuid() const
{
    return m_uuid;
}

void Message::setUuid(const uuid_t &uuid)
{
    m_uuid = uuid;
}

const uuid_t &Message::referrer() const
{
    return m_referrer;
}

void Message::setReferrer(const uuid_t &uuid)
{
    m_referrer = uuid;
}

int Message::senderId() const
{
    return m_senderId;
}

void Message::setSenderId(int id)
{
    m_senderId = id;
}

int Message::destId() const
{
    return m_destId;
}

void Message::setDestId(int id)
{
    m_destId = id;
}

int Message::destRank() const
{
    return m_destRank;
}

void Message::setDestRank(int r)
{
    m_destRank = r;
}

int Message::destUiId() const
{
    return -m_destRank;
}

void Message::setDestUiId(int id)
{
    m_destRank = -id;
}

int Message::rank() const
{
    return m_rank;
}

void Message::setRank(int rank)
{
    m_rank = rank;
}

int Message::uiId() const
{
    return -m_rank;
}

Type Message::type() const
{
    return m_type;
}

size_t Message::size() const
{
    return m_size;
}

unsigned int Message::priority() const
{
    return m_priority;
}

void Message::setPriority(unsigned int prio)
{
    m_priority = prio;
}

size_t Message::payloadSize() const
{
    return m_payloadSize;
}

void Message::setPayloadSize(size_t size)
{
    m_payloadSize = size;
}

std::string Message::payloadName() const
{
    return m_payloadName;
}

void Message::setPayloadName(const shm_name_t &name)
{
    m_payloadName = name;
}

CompressionMode Message::payloadCompression() const
{
    return CompressionMode(m_payloadCompression);
}

void Message::setPayloadCompression(CompressionMode mode)
{
    m_payloadCompression = mode;
}

size_t Message::payloadRawSize() const
{
    return m_payloadRawSize;
}

void Message::setPayloadRawSize(size_t size)
{
    m_payloadRawSize = size;
}

bool Message::isForBroadcast() const
{
    return m_forBroadcast;
}

void Message::setForBroadcast(bool enable)
{
    m_forBroadcast = enable;
}

bool Message::wasBroadcast() const
{
    return m_wasBroadcast;
}

void Message::setWasBroadcast(bool enable)
{
    m_wasBroadcast = enable;
}

bool Message::isNotification() const
{
    return m_notification;
}

void Message::setNotify(bool enable)
{
    m_notification = enable;
}

buffer compressPayload(CompressionMode &mode, const buffer &raw, int speed)
{
    return compressPayload(mode, raw.data(), raw.size(), speed);
}

buffer compressPayload(CompressionMode &mode, const char *raw, size_t size, int speed)
{
    auto m = mode;
    mode = message::CompressionNone;
    buffer compressed;
    switch (m) {
#ifdef HAVE_ZSTD
    case CompressionZstd: {
        size_t maxsize = ZSTD_compressBound(size);
        compressed.resize(maxsize);
        size_t compressedSize = ZSTD_compress(compressed.data(), compressed.size(), raw, size, speed);
        if (ZSTD_isError(compressedSize)) {
            std::cerr << "Zstd compression failed: " << ZSTD_getErrorName(compressedSize) << std::endl;
        } else if (compressedSize > size) {
            std::cerr << "Zstd compression without size reduction: " << size << " -> " << compressedSize << std::endl;
        } else {
            compressed.resize(compressedSize);
            mode = CompressionZstd;
        }
        break;
    }
#endif
#ifdef HAVE_LZ4
    case CompressionLz4: {
        size_t maxsize = LZ4_compressBound(size);
        compressed.resize(maxsize);
        size_t compressedSize = LZ4_compress_fast(raw, compressed.data(), size, compressed.size(), speed);
        if (compressedSize > 0 && compressedSize < size) {
            compressed.resize(compressedSize);
            mode = CompressionLz4;
        }
        break;
    }
#endif
    default:
        mode = CompressionNone;
        break;
    }

    if (mode != CompressionNone) {
        //std::cerr << "compressed from " << size << " to " << compressed.size() << std::endl;
        assert(size >= compressed.size());
    }

    return compressed;
}

buffer compressPayload(CompressionMode mode, Message &msg, buffer &raw, int speed)
{
    CompressionMode m = mode;
    msg.setPayloadRawSize(raw.size());
    msg.setPayloadCompression(CompressionNone);

    auto compressed = compressPayload(m, raw, speed);
    msg.setPayloadCompression(m);

    if (msg.payloadCompression() == CompressionNone) {
        compressed = std::move(raw);
    }

    msg.setPayloadSize(compressed.size());
    //std::cerr << "compressPayload " << msg.payloadRawSize() << " -> " << msg.payloadSize() << std::endl;
    return compressed;
}

buffer decompressPayload(CompressionMode mode, size_t size, size_t rawsize, buffer &compressed)
{
    if (mode == CompressionNone)
        return std::move(compressed);

    assert(compressed.size() >= size);
    return decompressPayload(mode, size, rawsize, compressed.data());
}

buffer decompressPayload(CompressionMode mode, size_t size, size_t rawsize, const char *compressed)
{
    buffer decompressed(rawsize);

    switch (mode) {
    case CompressionZstd: {
#ifdef HAVE_ZSTD
        size_t n = ZSTD_decompress(decompressed.data(), decompressed.size(), compressed, size);
        if (n != rawsize) {
            std::cerr << "Zstd decompression WARNING: decompressed size " << n << " does not match raw size " << rawsize
                      << std::endl;
        }
        if (ZSTD_isError(n)) {
            std::string err;
            if (const char *e = ZSTD_getErrorName(n)) {
                err = e;
            }
            std::cerr << "Zstd decompression ERROR: " << err << std::endl;
            throw codec_error("Zstd decompression failed: " + err);
        }
        //assert(n == msg.payloadRawSize());
#else
        throw codec_error("cannot decompress Zstd");
#endif
        break;
    }
    case CompressionLz4: {
#ifdef HAVE_LZ4
        int n = LZ4_decompress_safe(compressed, decompressed.data(), size, decompressed.size());
        if (size_t(n) != rawsize) {
            std::cerr << "LZ4 decompression WARNING: decompressed size " << n << " does not match raw size " << rawsize
                      << std::endl;
        }
        if (n < 0) {
            throw codec_error("LZ4 decompression failed");
        }
        //assert(n == msg.payloadRawSize());
#else
        throw codec_error("cannot decompress LZ4");
#endif
        break;
    }
    case CompressionNone: {
        break;
    }
    default: {
        if (int(mode) == CompressionSnappy) {
            throw codec_error("cannot decompress Snappy: no longer supported");
        } else {
            throw codec_error("cannot decompress: unknown format tag");
        }
        break;
    }
    }

    return decompressed;
}

buffer decompressPayload(const Message &msg, buffer &compressed)
{
    assert(compressed.size() >= msg.payloadSize());

    return decompressPayload(msg.payloadCompression(), msg.payloadSize(), msg.payloadRawSize(), compressed);
}

MessageFactory::MessageFactory(int id, int rank): m_id(id), m_rank(rank)
{}

int MessageFactory::id() const
{
    return m_id;
}

void MessageFactory::setId(int id)
{
    m_id = id;
}

void MessageFactory::setRank(int rank)
{
    m_rank = rank;
}

int MessageFactory::rank() const
{
    return m_rank;
}

} // namespace message
} // namespace vistle
