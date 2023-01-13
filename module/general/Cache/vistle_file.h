#ifndef VISTLE_FILE_H
#define VISTLE_FILE_H

#include <vistle/util/enum.h>
#include <vistle/core/shmname.h>
#include <vistle/core/message.h>

namespace vistle {

class ArchiveCompressionSettings {
public:
    virtual message::CompressionMode archiveCompression() const = 0;
    virtual int archiveCompressionSpeed() const = 0;
};

DEFINE_ENUM_WITH_STRING_CONVERSIONS(ChunkType, (Invalid)(Directory)(PortObject)(Archive))

struct ChunkHeader {
    char Vistle[7] = "Vistle";
    char type = '\0';
    uint32_t version = 1;
    uint64_t size = 0;
};

struct ChunkFooter {
    uint64_t size = 0;
    char type = '\0';
    char Vistle[7] = "vistle";

    ChunkFooter() = default;
    ChunkFooter(const ChunkHeader &cheader): size(cheader.size), type(cheader.type) {}
};

struct PortObjectHeader {
    uint32_t version = 1;
    int32_t port = 0;
    int32_t timestep = -1;
    int32_t block = -1;
    shm_name_t object;

    PortObjectHeader() = default;
    PortObjectHeader(int port, int timestep, int block, const std::string &object)
    : port(port), timestep(timestep), block(block), object(object)
    {}
};

template<class T>
bool Read(int fd, T &t);
#if 0
template<>
bool Read<PortObjectHeader>(int fd, PortObjectHeader &h);
template<>
bool Read<shm_name_t>(int fd, shm_name_t &name);
#endif

template<class Chunk>
bool WriteChunk(ArchiveCompressionSettings *mod, int fd, const Chunk &chunk);

template<class Chunk>
bool ReadChunk(ArchiveCompressionSettings *mod, int fd, const ChunkHeader &cheader, Chunk &chunk);

bool SkipChunk(ArchiveCompressionSettings *mod, int fd, const ChunkHeader &cheader);

} // namespace vistle
#endif
