#include <boost/lexical_cast.hpp>
#include <vistle/module/module.h>

#include <vistle/core/archive_saver.h>
#include <vistle/core/archive_loader.h>

#include <vistle/util/enum.h>
#include <vistle/util/byteswap.h>
#include <vistle/util/fileio.h>

using namespace vistle;

static const int NumPorts = 5;

auto constexpr file_endian = little_endian;


DEFINE_ENUM_WITH_STRING_CONVERSIONS(OperationMode, (Memory)(From_Disk)(To_Disk)(Automatic))

//#define DEBUG
//#define UNUSED

class Cache: public vistle::Module {
public:
    Cache(const std::string &name, int moduleID, mpi::communicator comm);
    ~Cache();

    message::CompressionMode archiveCompression() const;
    int archiveCompressionSpeed() const;

private:
    bool compute() override;
    bool prepare() override;
    bool reduce(int t) override;
    bool changeParameter(const Parameter *p) override;

    IntParameter *p_mode = nullptr;
    OperationMode m_mode = Memory;
    bool m_toDisk = false, m_fromDisk = false;
    StringParameter *p_file = nullptr;
    IntParameter *p_step = nullptr;
    IntParameter *p_start = nullptr;
    IntParameter *p_stop = nullptr;

    IntParameter *m_compressionMode = nullptr;
    FloatParameter *m_zfpRate = nullptr;
    IntParameter *m_zfpPrecision = nullptr;
    FloatParameter *m_zfpAccuracy = nullptr;
    IntParameter *m_archiveCompression = nullptr;
    IntParameter *m_archiveCompressionSpeed = nullptr;

    IntParameter *p_reorder = nullptr;
    IntParameter *p_renumber = nullptr;

    int m_fd = -1;
    std::shared_ptr<DeepArchiveSaver> m_saver;

    vistle::Port *m_inPort[NumPorts], *m_outPort[NumPorts];

    CompressionSettings m_compressionSettings;
};

using namespace vistle;

Cache::Cache(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    setDefaultCacheMode(ObjectCache::CacheDeleteLate);

    for (int i = 0; i < NumPorts; ++i) {
        std::string suffix = std::to_string(i);

        m_inPort[i] = createInputPort("data_in" + suffix, "input data " + suffix);
        m_outPort[i] = createOutputPort("data_out" + suffix, "output data " + suffix);
    }

    p_mode = addIntParameter("mode", "operation mode", m_mode, Parameter::Choice);
    V_ENUM_SET_CHOICES(p_mode, OperationMode);
    p_file = addStringParameter("file", "filename where cache should be created", "/scratch/", Parameter::Filename);
    setParameterFilters(p_file, "Vistle Data (*.vsld)/All Files (*)");

    p_step = addIntParameter("step", "step width when reading from disk", 1);
    setParameterMinimum(p_step, Integer(1));
    p_start = addIntParameter("start", "start step", 0);
    setParameterMinimum(p_start, Integer(0));
    p_stop = addIntParameter("stop", "stop step", 1000);
    setParameterMinimum(p_stop, Integer(0));

    m_compressionMode =
        addIntParameter("field_compression", "compression mode for data fields", Uncompressed, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_compressionMode, FieldCompressionMode);
    m_zfpRate = addFloatParameter("zfp_rate", "ZFP fixed compression rate", 8.);
    setParameterRange(m_zfpRate, Float(1), Float(64));
    m_zfpPrecision = addIntParameter("zfp_precision", "ZFP fixed precision", 16);
    setParameterRange(m_zfpPrecision, Integer(1), Integer(64));
    m_zfpAccuracy = addFloatParameter("zfp_accuracy", "ZFP compression error tolerance", 1e-10);
    setParameterRange(m_zfpAccuracy, Float(0.), Float(1e10));
    m_archiveCompression = addIntParameter("archive_compression", "compression mode for archives",
                                           message::CompressionZstd, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_archiveCompression, message::CompressionMode);
    m_archiveCompressionSpeed =
        addIntParameter("archive_compression_speed", "speed parameter of compression algorithm", -1);
    setParameterRange(m_archiveCompressionSpeed, Integer(-1), Integer(100));

    p_reorder = addIntParameter("reorder", "reorder timesteps", false, Parameter::Boolean);
    p_renumber = addIntParameter("renumber", "renumber timesteps consecutively", true, Parameter::Boolean);
}

Cache::~Cache()
{}

message::CompressionMode Cache::archiveCompression() const
{
    return message::CompressionMode(m_archiveCompression->getValue());
}

int Cache::archiveCompressionSpeed() const
{
    return message::CompressionMode(m_archiveCompression->getValue());
}

#define CERR std::cerr << "Cache: "

namespace {

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

DEFINE_ENUM_WITH_STRING_CONVERSIONS(ChunkType, (Invalid)(Directory)(PortObject)(Archive))

struct ChunkHeader {
    char Vistle[7] = "Vistle";
    char type = '\0';
    uint32_t version = 1;
    uint64_t size = 0;
};

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

struct ChunkFooter {
    uint64_t size = 0;
    char type = '\0';
    char Vistle[7] = "vistle";

    ChunkFooter() = default;
    ChunkFooter(const ChunkHeader &cheader): size(cheader.size), type(cheader.type) {}
};

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
    if (hgood.indexSize != h.indexSize) {
        CERR << "Archive header Index size mismatch: expecting " << hgood.indexSize << ", found " << h.indexSize
             << std::endl;
        return false;
    }
    if (!Read(fd, h.scalarSize))
        return false;
    if (hgood.scalarSize != h.scalarSize) {
        CERR << "Archive header Scalar size mismatch: expecting " << hgood.scalarSize << ", found " << h.scalarSize
             << std::endl;
        return false;
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

template<class Module, class Chunk>
bool WriteChunk(Module *mod, int fd, const Chunk &chunk)
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

template<class Module, class Chunk>
bool ReadChunk(Module *mod, int fd, const ChunkHeader &cheader, Chunk &chunk)
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

template<class Module>
bool SkipChunk(Module *mod, int fd, const ChunkHeader &cheader)
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
bool WriteChunk<Cache, SubArchiveDirectoryEntry>(Cache *mod, int fd, const SubArchiveDirectoryEntry &ent)
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
bool ReadChunk<Cache, SubArchiveDirectoryEntry>(Cache *mod, int fd, const ChunkHeader &cheader,
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

} // namespace

bool Cache::compute()
{
    if (m_fromDisk) {
        for (int i = 0; i < NumPorts; ++i) {
            accept<Object>(m_inPort[i]);
        }
        return true;
    }

    for (int i = 0; i < NumPorts; ++i) {
        Object::const_ptr obj = accept<Object>(m_inPort[i]);
        if (obj) {
            passThroughObject(m_outPort[i], obj);

            if (m_toDisk) {
                assert(m_fd != -1);

                // serialialize object and all not-yet-serialized sub-objects to memory
                vecostreambuf<buffer> memstr;
                vistle::oarchive memar(memstr);
                memar.setCompressionSettings(m_compressionSettings);
                memar.setSaver(m_saver);
                obj->saveObject(memar);

                // copy serialized sub-objects to disk
                auto dir = m_saver->getDirectory();
                for (const auto &ent: dir) {
                    if (!WriteChunk(this, m_fd, ent))
                        return false;
                }
                m_saver->flushDirectory();

                // copy serialized object to disk
                const buffer &mem = memstr.get_vector();
                SubArchiveDirectoryEntry ent{obj->getName(), false, mem.size(), const_cast<char *>(mem.data())};
                if (!WriteChunk(this, m_fd, ent))
                    return false;

                // add reference to object to port
                PortObjectHeader pheader(i, obj->getTimestep(), obj->getBlock(), obj->getName());
                if (!WriteChunk(this, m_fd, pheader))
                    return false;
            }
        }
    }

    return true;
}

bool Cache::prepare()
{
    m_compressionSettings.m_compress = (FieldCompressionMode)m_compressionMode->getValue();
    m_compressionSettings.m_zfpRate = m_zfpRate->getValue();
    m_compressionSettings.m_zfpAccuracy = m_zfpAccuracy->getValue();
    m_compressionSettings.m_zfpPrecision = m_zfpPrecision->getValue();

    std::string file = p_file->getValue();

    if (p_mode->getValue() == Automatic) {
        if (file.empty()) {
            m_toDisk = m_fromDisk = false;
        } else {
            bool haveInput = false;
            for (int i = 0; i < NumPorts; ++i) {
                if (m_inPort[i]->isConnected())
                    haveInput = true;
            }
            m_toDisk = haveInput;
            m_fromDisk = !haveInput;
        }
    }

    file += ".";
    file += std::to_string(rank());
    file += ".vslp";

    if (m_toDisk) {
        m_saver.reset(new DeepArchiveSaver);
        m_saver->setCompressionSettings(m_compressionSettings);
        m_fd =
            open(file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    }

    if (!m_fromDisk)
        return true;

    int start = p_start->getValue();
    int step = p_step->getValue();
    int stop = p_stop->getValue();

    bool reorder = p_reorder->getValue();
    bool renumber = p_renumber->getValue();

    m_fd = open(file.c_str(), O_RDONLY);
    if (m_fd == -1) {
        sendError("Could not open %s: %s", file.c_str(), strerror(errno));
    }
    int errorfd = mpi::all_reduce(comm(), m_fd, mpi::minimum<int>());
    if (errorfd == -1) {
        m_toDisk = false;
        if (m_fd >= 0)
            close(m_fd);
        m_fd = -1;
        return true;
    }

    typedef std::vector<std::string> TimestepObjects;
    std::vector<TimestepObjects> portObjects[NumPorts];

    std::map<std::string, buffer> objects, arrays;
    std::map<std::string, message::CompressionMode> compression;
    std::map<std::string, size_t> size;
    std::map<std::string, std::string> objectTranslations, arrayTranslations;
    auto fetcher = std::make_shared<DeepArchiveFetcher>(objects, arrays, compression, size);
    fetcher->setRenameObjects(true);
    fetcher->setObjectTranslations(objectTranslations);
    fetcher->setArrayTranslations(arrayTranslations);
    bool ok = true;
    int numObjects = 0;
    int numTime = 0;

    std::set<std::string> renumberedObjects;

    auto renumberObject = [&renumberedObjects, renumber, start, stop, step](Object::ptr obj) {
        if (!renumber)
            return;
        if (renumberedObjects.find(obj->getName()) != renumberedObjects.end())
            return;
        renumberedObjects.insert(obj->getName());

        auto t = obj->getTimestep();
        if (t >= 0) {
            assert(t >= start);
            t -= start;
            t /= step;
            obj->setTimestep(t);
        }
        auto nt = obj->getNumTimesteps();
        if (nt >= 0) {
            nt = (stop - start + step - 1) / step;
            assert(t < nt);
            obj->setNumTimesteps(nt);
        }
    };

    auto restoreObject = [this, &renumberObject, &compression, &size, &objects, &fetcher](const std::string &name0,
                                                                                          int port) {
        //CERR << "output to port " << port << ", " << num << " objects/arrays read" << std::endl;
        //CERR << "output to port " << port << ", initial " << name0 << " of size " << objects[name0].size() << std::endl;

        const auto &objbuf = objects[name0];
        buffer raw;
        const auto &comp = compression[name0];
        if (comp != message::CompressionNone) {
            const auto &sz = size[name0];
            try {
                raw = message::decompressPayload(comp, objbuf.size(), sz, objbuf.data());
            } catch (const std::exception &ex) {
                CERR << "failed to decompress object " << name0 << ": " << ex.what() << std::endl;
                return;
            }
        }
        const auto &buf = comp == message::CompressionNone ? objbuf : raw;
        vecistreambuf<buffer> membuf(buf);
        vistle::iarchive memar(membuf);
        memar.setFetcher(fetcher);
        //CERR << "output to port " << port << ", trying to load " << name0 << std::endl;
        Object::ptr obj(Object::loadObject(memar));
        updateMeta(obj);
        renumberObject(obj);
        if (auto db = DataBase::as(obj)) {
            if (auto cgrid = db->grid()) {
                auto grid = std::const_pointer_cast<Object>(cgrid);
                renumberObject(grid);
            }
        }
        passThroughObject(m_outPort[port], obj);
        fetcher->releaseArrays();
    };

    std::string objectToRestore;
    for (bool error = false; !error;) {
        ChunkHeader cheader, chgood;
        if (!Read(m_fd, cheader)) {
            break;
        }
        if (cheader.version != chgood.version) {
            sendError("Cannot read Vistle files of version %d, only %d is supported", (int)cheader.version,
                      chgood.version);
            break;
        }
        //CERR << "ChunkHeader: " << cheader << std::endl;
        //CERR << "ChunkHeader: found type=" << cheader.type << std::endl;

        switch (cheader.type) {
        case ChunkType::Archive: {
            SubArchiveDirectoryEntry ent;
            if (!ReadChunk(this, m_fd, cheader, ent)) {
                CERR << "failed to read Archive chunk" << std::endl;
                error = true;
                continue;
            }
            //CERR << "entry " << ent.name << " of size " << ent.storage->size() << std::endl;
            compression[ent.name] = ent.compression;
            size[ent.name] = ent.size;
            if (ent.is_array) {
                arrays[ent.name] = std::move(*ent.storage);
            } else {
                objects[ent.name] = std::move(*ent.storage);
            }
            break;
        }
        case ChunkType::PortObject: {
            PortObjectHeader poh;
            if (!ReadChunk(this, m_fd, cheader, poh)) {
                CERR << "failed to read PortObject chunk" << std::endl;
                error = true;
                continue;
            }

            int tplus = poh.timestep + 1;
            assert(tplus >= 0);
            if (numTime < tplus)
                numTime = tplus;
            if (ssize_t(portObjects[poh.port].size()) <= numTime) {
                portObjects[poh.port].resize(numTime + 1);
            }
            portObjects[poh.port][tplus].push_back(poh.object);
            objectToRestore = poh.object.str();

            if (reorder)
                continue;

            if (!m_outPort[poh.port]->isConnected()) {
                //CERR << "skipping " << name0 << ", output " << port << " not connected" << std::endl;
                continue;
            }

            if (poh.timestep >= 0 && poh.timestep < start)
                continue;

            if (poh.timestep >= 0 && poh.timestep > stop)
                continue;

            if (poh.timestep >= 0 && (poh.timestep - start) % step != 0)
                continue;

            ++numObjects;

            restoreObject(objectToRestore, poh.port);
            break;
        }
        case ChunkType::Directory: {
            break;
        }
        default: {
            CERR << "unknown chunk type " << cheader.type << std::endl;
            if (!SkipChunk(this, m_fd, cheader)) {
                CERR << "failed to skip unknown chunk" << std::endl;
                error = true;
                continue;
            }
            break;
        }
        }
    }

    if (reorder) {
        std::vector<int> timesteps;
        timesteps.push_back(-1);
        for (int timestep = 0; timestep < numTime; ++timestep) {
            if (timestep < start)
                continue;

            if (timestep > stop)
                continue;

            if ((timestep - start) % step != 0)
                continue;

            timesteps.push_back(timestep);
        }

        for (auto &timestep: timesteps) {
            for (int port = 0; port < NumPorts; ++port) {
                if (!m_outPort[port]->isConnected()) {
                    //CERR << "skipping " << name0 << ", output " << port << " not connected" << std::endl;
                    continue;
                }

                const auto &o = portObjects[port][timestep + 1];
                for (auto &name0: o) {
                    ++numObjects;

                    restoreObject(name0, port);
                }
            }
        }
    }

    sendInfo("restored %d objects", numObjects);
    objectTranslations = fetcher->objectTranslations();
    arrayTranslations = fetcher->arrayTranslations();

    close(m_fd);
    m_fd = -1;

    return ok;
}

bool Cache::reduce(int t)
{
    if (t != -1)
        return true;

    m_saver.reset();

    if (m_fd >= 0)
        close(m_fd);
    m_fd = -1;

    return true;
}

bool Cache::changeParameter(const Parameter *p)
{
    if (p == p_mode) {
        m_mode = OperationMode(p_mode->getValue());
        switch (m_mode) {
        case Memory:
            m_fromDisk = m_toDisk = false;
            break;
        case To_Disk:
            m_fromDisk = false;
            m_toDisk = true;
            break;
        case From_Disk:
            m_fromDisk = true;
            m_toDisk = false;
            break;
        case Automatic:
            m_fromDisk = m_toDisk = false;
            break;
        }
    }

    return Module::changeParameter(p);
}

MODULE_MAIN(Cache)
