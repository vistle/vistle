#include "vistle_file.h"

#include <vistle/core/archive_saver.h>
#include <vistle/core/archive_loader.h>

#include <vistle/util/enum.h>
#include <vistle/util/byteswap.h>
#include <vistle/util/fileio.h>

namespace vistle {

//#define DEBUG
//#define UNUSED

template<>
bool Read<shm_name_t>(int fd, shm_name_t &name);


#define CERR std::cerr << "vistle_file: "

namespace {

auto constexpr file_endian = little_endian;

ssize_t swrite(int fd, const void *buf, size_t n)
{
    size_t tot = 0;
    while (tot < n) {
        ssize_t result = write(fd, static_cast<const char *>(buf) + tot, n - tot);
        if (result < 0) {
            CERR << "write error: " << strerror(errno) << std::endl;
            return result;
        }
        tot += result;
    }

    return tot;
}

ssize_t sread(int fd, void *buf, size_t n)
{
    size_t tot = 0;
    while (tot < n) {
        ssize_t result = read(fd, static_cast<char *>(buf) + tot, n - tot);
        if (result < 0) {
            CERR << "read error: " << strerror(errno) << std::endl;
            return result;
        }
        tot += result;
        if (result == 0)
            break;
    }

    return tot;
}

} // namespace

template<class T>
bool Write(int fd, const T &t)
{
    T tt = byte_swap<host_endian, file_endian>(t);
    ssize_t n = swrite(fd, &tt, sizeof(tt));
    if (n != sizeof(tt)) {
        CERR << "failed to write " << sizeof(tt) << " bytes, result was " << n << ", errno=" << strerror(errno)
             << std::endl;
        return false;
    }

    return true;
}

template<class T>
bool Read(int fd, T &t)
{
    T tt;
    ssize_t n = sread(fd, &tt, sizeof(tt));
    if (n != sizeof(tt)) {
        if (n != 0)
            CERR << "failed to read " << sizeof(tt) << " bytes, result was " << n << ", errno=" << strerror(errno)
                 << std::endl;
        return false;
    }

    t = byte_swap<host_endian, file_endian>(tt);

    return true;
}

bool Skip(int fd, size_t skip)
{
    off_t n = lseek(fd, skip, SEEK_CUR);
    if (n == off_t(-1)) {
        CERR << "failed to skip " << skip << " bytes, result was " << n << ", errno=" << strerror(errno) << std::endl;
        return false;
    }

    return true;
}

template<>
bool Write<shm_name_t>(int fd, const shm_name_t &name)
{
    ssize_t n = swrite(fd, &name[0], sizeof(name));
    if (n == -1) {
        CERR << "failed to write " << sizeof(name) << " bytes, result was " << n << ", errno=" << strerror(errno)
             << std::endl;
        return false;
    }
    return true;
}

template<>
bool Read<shm_name_t>(int fd, shm_name_t &name)
{
    ssize_t n = sread(fd, &name[0], sizeof(name));
    if (n == -1) {
        CERR << "failed to read " << sizeof(name) << " bytes, result was " << n << ", errno=" << strerror(errno)
             << std::endl;
        return false;
    }
    return true;
}

#ifdef DEBUG
std::ostream &operator<<(std::ostream &os, const ChunkHeader &ch)
{
    os << "type " << toString(ChunkType(ch.type)) << "=" << int(ch.type);
    os << " version " << ch.version;
    os << " size " << ch.size;

    return os;
}
#endif

template<>
bool Write<ChunkHeader>(int fd, const ChunkHeader &h)
{
    ssize_t n = swrite(fd, h.Vistle, sizeof(h.Vistle));
    if (n != sizeof(h.Vistle))
        return false;

    if (!Write(fd, h.type))
        return false;

    if (!Write(fd, h.version))
        return false;

    if (!Write(fd, h.size))
        return false;

    return true;
}

template<>
bool Read<ChunkHeader>(int fd, ChunkHeader &h)
{
    const ChunkHeader hgood;
    ssize_t n = sread(fd, h.Vistle, sizeof(h.Vistle));
    if (n != sizeof(h.Vistle))
        return false;
    if (strncmp(h.Vistle, hgood.Vistle, sizeof(h.Vistle)) != 0)
        return false;

    if (!Read(fd, h.type))
        return false;

    if (!Read(fd, h.version))
        return false;

    if (!Read(fd, h.size))
        return false;

    return true;
}

#ifdef DEBUG
std::ostream &operator<<(std::ostream &os, const ChunkFooter &cf)
{
    os << "type " << toString(ChunkType(cf.type)) << "=" << int(cf.type);
    os << " size " << cf.size;

    return os;
}
#endif

template<>
bool Write<ChunkFooter>(int fd, const ChunkFooter &f)
{
    if (!Write(fd, f.size))
        return false;
    if (!Write(fd, f.type))
        return false;
    ssize_t n = swrite(fd, f.Vistle, sizeof(f.Vistle));
    if (n != sizeof(f.Vistle))
        return false;

    return true;
}

template<>
bool Read<ChunkFooter>(int fd, ChunkFooter &f)
{
    const ChunkFooter fgood;
    if (!Read(fd, f.size))
        return false;

    if (!Read(fd, f.type))
        return false;
    ssize_t n = sread(fd, f.Vistle, sizeof(f.Vistle));
    if (n != sizeof(f.Vistle))
        return false;
    if (strncmp(f.Vistle, fgood.Vistle, sizeof(f.Vistle)) != 0)
        return false;

    return true;
}

#ifdef UNUSED
std::ostream &operator<<(std::ostream &os, const PortObjectHeader &poh)
{
    os << "version " << poh.port;
    os << " port " << poh.port;
    os << " time " << poh.timestep;
    os << " block " << poh.block;
    os << " object " << poh.object.str();

    return os;
}
#endif

template<>
bool Write<PortObjectHeader>(int fd, const PortObjectHeader &h)
{
    if (!Write(fd, h.version))
        return false;
    if (!Write(fd, h.port))
        return false;
    if (!Write(fd, h.timestep))
        return false;
    if (!Write(fd, h.block))
        return false;
    if (!Write(fd, h.object))
        return false;

    return true;
}

template<>
bool Read<PortObjectHeader>(int fd, PortObjectHeader &h)
{
    const PortObjectHeader hgood;
    if (!Read(fd, h.version))
        return false;
    if (hgood.version != h.version)
        return false;
    if (!Read(fd, h.port))
        return false;
    if (!Read(fd, h.timestep))
        return false;
    if (!Read(fd, h.block))
        return false;
    if (!Read(fd, h.object))
        return false;
    return true;
}


struct ArchiveHeader {
    uint32_t version = 1;
    int16_t indexSize = sizeof(Index);
    int16_t scalarSize = sizeof(Scalar);
    shm_name_t object;

    ArchiveHeader() = default;
    ArchiveHeader(const std::string &object): object(object) {}
};

template<>
bool Write<ArchiveHeader>(int fd, const ArchiveHeader &h)
{
    if (!Write(fd, h.version))
        return false;
    if (!Write(fd, h.indexSize))
        return false;
    if (!Write(fd, h.scalarSize))
        return false;
    if (!Write(fd, h.object))
        return false;

    return true;
}

template<>
bool Read<ArchiveHeader>(int fd, ArchiveHeader &h)
{
    const ArchiveHeader hgood;
    if (!Read(fd, h.version))
        return false;
    if (hgood.version != h.version) {
        CERR << "Archive header version mismatch: expecting " << hgood.version << ", found " << h.version << std::endl;
        return false;
    }
    if (!Read(fd, h.indexSize))
        return false;
    if (hgood.indexSize < h.indexSize) {
        CERR << "Archive header Index size too large: expecting " << hgood.indexSize << ", found " << h.indexSize
             << std::endl;
        return false;
    }
    if (!Read(fd, h.scalarSize))
        return false;
    if (hgood.scalarSize < h.scalarSize) {
        CERR << "Archive header Scalar size larger: have " << hgood.scalarSize << ", found " << h.scalarSize
             << " - will loose precision" << std::endl;
    }
    if (!Read(fd, h.object)) {
        CERR << "Archive header: failed to read entity name" << std::endl;
        return false;
    }
    return true;
}

#ifdef UNUSED
template<>
bool Write<std::string>(int fd, const std::string &s)
{
    uint64_t l = s.length();
    if (!Write(fd, l)) {
        CERR << "failed to write string length" << std::endl;
        return false;
    }
    ssize_t n = swrite(fd, s.data(), s.length());
    if (n != ssize_t(s.length())) {
        CERR << "failed to write string data" << std::endl;
        return false;
    }

    return true;
}

template<>
bool Read<std::string>(int fd, std::string &s)
{
    uint64_t l;
    if (!Read(fd, l)) {
        CERR << "failed to read string length" << std::endl;
        return false;
    }

    buffer d(l);
    ssize_t n = sread(fd, d.data(), d.size());
    if (n != ssize_t(d.size())) {
        CERR << "failed to read string data of size " << l << std::endl;
        return false;
    }

    s = std::string(d.data(), d.data() + d.size());

    return true;
}

template<>
bool Write<buffer>(int fd, const buffer &v)
{
    uint64_t l = v.size();
    if (!Write(fd, l)) {
        CERR << "failed to write vector size" << std::endl;
        return false;
    }
    ssize_t n = swrite(fd, v.data(), v.size());
    if (n != ssize_t(v.size())) {
        CERR << "failed to write vector data" << std::endl;
        return false;
    }

    return true;
}

template<>
bool Read<buffer>(int fd, buffer &v)
{
    uint64_t l;
    if (!Read(fd, l)) {
        CERR << "failed to read vector size" << std::endl;
        return false;
    }
    v.resize(l);
    ssize_t n = sread(fd, v.data(), v.size());
    if (n != ssize_t(v.size())) {
        CERR << "failed to read vector data of size " << l << std::endl;
        return false;
    }

    return true;
}
#endif

template<class Chunk>
struct ChunkTypeMap;

template<>
struct ChunkTypeMap<SubArchiveDirectoryEntry> {
    static const ChunkType type = Archive;
};

template<>
struct ChunkTypeMap<PortObjectHeader> {
    static const ChunkType type = PortObject;
};

template<class Chunk>
bool WriteChunk(ArchiveCompressionSettings *mod, int fd, const Chunk &chunk)
{
    ChunkHeader cheader;
    cheader.type = ChunkTypeMap<Chunk>::type;
    cheader.size = sizeof(ChunkHeader) + sizeof(Chunk) + sizeof(ChunkFooter);

    if (!Write(fd, cheader))
        return false;
#ifdef DEBUG
    CERR << "WriteChunk: header=" << cheader << std::endl;
#endif
    if (!Write(fd, chunk))
        return false;
    ChunkFooter cfooter(cheader);
    if (!Write(fd, cfooter))
        return false;
#ifdef DEBUG
    CERR << "WriteChunk: footer=" << cfooter << std::endl;
#endif
    return true;
}

template<class Chunk>
bool ReadChunk(ArchiveCompressionSettings *mod, int fd, const ChunkHeader &cheader, Chunk &chunk)
{
    if (cheader.type != ChunkTypeMap<Chunk>::type) {
        CERR << "ReadChunk: chunk type mismatch" << std::endl;
        return false;
    }
    if (!Read(fd, chunk))
        return false;
    ChunkFooter cfgood(cheader), cfooter;
    if (!Read(fd, cfooter))
        return false;
    if (cfgood.size != cfooter.size) {
        CERR << "ReadChunk: chunk size mismatch in footer" << std::endl;
        return false;
    }
    if (cheader.type != cfooter.type) {
        CERR << "ReadChunk: chunk type mismatch between footer (type " << toString(ChunkType(cfooter.type)) << "="
             << int(cfooter.type) << ") and header (type " << toString(ChunkType(cheader.type)) << "="
             << int(cheader.type) << ")" << std::endl;
        return false;
    }
    return true;
}

bool SkipChunk(ArchiveCompressionSettings *mod, int fd, const ChunkHeader &cheader)
{
    if (!Skip(fd, cheader.size - sizeof(cheader) - sizeof(ChunkFooter)))
        return false;
    ChunkFooter cfgood(cheader), cfooter;
    if (!Read(fd, cfooter))
        return false;
    if (cfgood.size != cfooter.size) {
        CERR << "SkipChunk: chunk size mismatch in footer" << std::endl;
        return false;
    }
    if (cheader.type != cfooter.type) {
        CERR << "SkipChunk: chunk type mismatch between footer (type " << toString(ChunkType(cfooter.type)) << "="
             << int(cfooter.type) << ") and header (type " << toString(ChunkType(cheader.type)) << "="
             << int(cheader.type) << ")" << std::endl;
        return false;
    }
    return true;
}

template<>
bool WriteChunk<SubArchiveDirectoryEntry>(ArchiveCompressionSettings *mod, int fd, const SubArchiveDirectoryEntry &ent)
{
    message::CompressionMode comp = message::CompressionMode(mod->archiveCompression());
    int speed = mod->archiveCompressionSpeed();

    buffer compressed;
    if (comp != message::CompressionNone) {
        compressed = message::compressPayload(comp, ent.data, ent.size, speed);
    }

    ChunkHeader cheader;
    cheader.type = ChunkTypeMap<SubArchiveDirectoryEntry>::type;
    cheader.size = sizeof(cheader);
    ArchiveHeader aheader(ent.name);
    cheader.size += sizeof(aheader);
    char flag = ent.is_array ? 1 : 0;
    cheader.size += sizeof(flag);
    uint64_t length = ent.size;
    cheader.size += sizeof(length);
    uint32_t compMode = comp;
    cheader.size += sizeof(compMode);
    uint64_t compLength = comp == message::CompressionNone ? ent.size : compressed.size();
    cheader.size += sizeof(compLength);
    cheader.size += compLength;
    cheader.size += sizeof(ChunkFooter);
    //CERR << "WriteChunk: header=" << cheader << std::endl;
    if (!Write(fd, cheader))
        return false;

    if (!Write(fd, aheader))
        return false;

    if (!Write(fd, flag)) {
        CERR << "failed to write entry array flag" << std::endl;
        return false;
    }

    if (!Write(fd, length)) {
        CERR << "failed to write entry length" << std::endl;
        return false;
    }

    if (!Write(fd, compMode)) {
        CERR << "failed to write compression mode" << std::endl;
        return false;
    }

    if (!Write(fd, compLength)) {
        CERR << "failed to write entry compressed length" << std::endl;
        return false;
    }

    ssize_t n = 0;
    if (comp == message::CompressionNone) {
        n = swrite(fd, ent.data, ent.size);
    } else {
        n = swrite(fd, compressed.data(), compressed.size());
    }
    if (n != ssize_t(compLength)) {
        CERR << "failed to write entry data of size " << compLength << " (raw: " << ent.size << "): result=" << n
             << std::endl;
        if (n == -1)
            std::cerr << "  ERRNO=" << errno << ": " << strerror(errno) << std::endl;
        return false;
    }

    ChunkFooter cfooter(cheader);
    if (!Write(fd, cfooter))
        return false;

    return true;
}

template<>
bool ReadChunk<SubArchiveDirectoryEntry>(ArchiveCompressionSettings *mod, int fd, const ChunkHeader &cheader,
                                         SubArchiveDirectoryEntry &ent)
{
    if (cheader.type != ChunkTypeMap<SubArchiveDirectoryEntry>::type) {
        CERR << "failed to read entry type" << std::endl;
        return false;
    }

    ArchiveHeader aheader;
    if (!Read(fd, aheader)) {
        CERR << "failed to read archive header" << std::endl;
        return false;
    }
    ent.name = aheader.object.str();

    char flag;
    if (!Read(fd, flag)) {
        CERR << "failed to read entry array flag" << std::endl;
        return false;
    }
    ent.is_array = flag ? true : false;

    uint64_t length;
    if (!Read(fd, length)) {
        CERR << "failed to read entry length" << std::endl;
        return false;
    }
    ent.size = length;

    uint32_t compMode;
    if (!Read(fd, compMode)) {
        CERR << "failed to read entry compression mode" << std::endl;
        return false;
    }
    message::CompressionMode comp = message::CompressionMode(compMode);
    ent.compression = comp;

    uint64_t compLength;
    if (!Read(fd, compLength)) {
        CERR << "failed to read entry compressed length" << std::endl;
        return false;
    }
    ent.compressedSize = compLength;

    ent.storage.reset(new buffer(compLength));
    ent.data = ent.storage->data();

    ssize_t n = sread(fd, ent.data, ent.compressedSize);
    if (n != ssize_t(ent.compressedSize)) {
        CERR << "failed to read entry data" << std::endl;
        return false;
    }

#if 0 // decompression occurs on demand
    if (comp != message::CompressionNone) {
        CERR << "trying to decompress " << ent.compressedSize << " bytes to " << ent.size << " for " << ent.name << std::endl;
        try {
            auto v = message::decompressPayload(comp, ent.compressedSize, ent.size, ent.data);
            *ent.storage = v;
        } catch (const std::exception &ex) {
            CERR << "failed to decompress " << ent.name << ": " << ex.what() << std::endl;
            return false;
        }
        ent.compression = message::CompressionNone;
        ent.compressedSize = ent.size;
    }
#endif

    ChunkFooter cfooter;
    if (!Read(fd, cfooter)) {
        CERR << "failed to read chunk footer for archive" << std::endl;
        return false;
    }
    if (cfooter.size != cheader.size) {
        CERR << "size mismatch in chunk header and footer, expecting " << cheader.size << ", got " << cfooter.size
             << std::endl;
        return false;
    }

    return true;
}

template bool WriteChunk<PortObjectHeader>(ArchiveCompressionSettings *, int, PortObjectHeader const &);
template bool ReadChunk<PortObjectHeader>(ArchiveCompressionSettings *, int, ChunkHeader const &, PortObjectHeader &);
//template bool SkipChunk<PortObjectHeader>(ArchiveCompressionSettings *, int, ChunkHeader const &);

} // namespace vistle
