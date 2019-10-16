#include <boost/lexical_cast.hpp>
#include <module/module.h>

#include <core/archive_saver.h>
#include <core/archive_loader.h>

#include <util/enum.h>
#include <util/byteswap.h>
#include <util/fileio.h>

using namespace vistle;

static const int NumPorts = 5;

auto constexpr file_endian = little_endian;


DEFINE_ENUM_WITH_STRING_CONVERSIONS(OperationMode,
                                    (Memory)
                                    (From_Disk)
                                    (To_Disk)
                                    (Automatic))

class Cache: public vistle::Module {

 public:
   Cache(const std::string &name, int moduleID, mpi::communicator comm);
   ~Cache();

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

Cache::Cache(const std::string &name, int moduleID, mpi::communicator comm)
: Module("cache input objects", name, moduleID, comm)
{
   setDefaultCacheMode(ObjectCache::CacheDeleteLate);

   for (int i=0; i<NumPorts; ++i) {
      std::string suffix = std::to_string(i);

      m_inPort[i] = createInputPort("data_in"+suffix, "input data "+suffix);
      m_outPort[i] = createOutputPort("data_out"+suffix, "output data "+suffix);
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

   m_compressionMode = addIntParameter("field_compression", "compression mode for data fields", Uncompressed, Parameter::Choice);
   V_ENUM_SET_CHOICES(m_compressionMode, FieldCompressionMode);
   m_zfpRate = addFloatParameter("zfp_rate", "ZFP fixed compression rate", 8.);
   setParameterRange(m_zfpRate, Float(1), Float(64));
   m_zfpPrecision = addIntParameter("zfp_precision", "ZFP fixed precision", 16);
   setParameterRange(m_zfpPrecision, Integer(1), Integer(64));
   m_zfpAccuracy = addFloatParameter("zfp_accuracy", "ZFP compression error tolerance", 1e-10);
   setParameterRange(m_zfpAccuracy, Float(0.), Float(1e10));
   m_archiveCompression = addIntParameter("archive_compression", "compression mode for archives", message::CompressionZstd, Parameter::Choice);
   V_ENUM_SET_CHOICES(m_archiveCompression, message::CompressionMode);
   m_archiveCompressionSpeed = addIntParameter("archive_compression_speed", "speed parameter of compression algorithm", -1);
   setParameterRange(m_archiveCompressionSpeed, Integer(-1), Integer(100));

   p_reorder = addIntParameter("reorder", "reorder timesteps", false, Parameter::Boolean);
   p_renumber = addIntParameter("renumber", "renumber timesteps consecutively", true, Parameter::Boolean);
}

Cache::~Cache() {

}

#define CERR std::cerr << "Cache: "

namespace {

struct Header {
    endianness endian = file_endian;
    char Vistle[7] = "vistle";
    uint32_t version = 1;
    int16_t indexSize = sizeof(Index);
    int16_t scalarSize = sizeof(Scalar);
};

ssize_t swrite(int fd, const void *buf, size_t n) {
    ssize_t tot = 0;
    while (tot < n) {
        ssize_t result = write(fd, static_cast<const char *>(buf)+tot, n-tot);
        if (result < 0) {
            CERR << "write error: " << strerror(errno) << std::endl;
            return result;
        }
        tot += result;
    }

    return tot;
}

ssize_t sread(int fd, void *buf, size_t n) {
    ssize_t tot = 0;
    while (tot < n) {
        ssize_t result = read(fd, static_cast<char *>(buf)+tot, n-tot);
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
bool Write(int fd, const T &t) {
    T tt = byte_swap<host_endian, file_endian>(t);
    ssize_t n = swrite(fd, &tt, sizeof(tt));
    if (n != sizeof(tt)) {
        CERR << "failed to write " << sizeof(tt) << " bytes, result was " << n << ", errno=" << strerror(errno) << std::endl;
        return false;
    }

    return true;
}

template<class T>
bool Read(int fd, T &t) {
    T tt;
    ssize_t n = sread(fd, &tt, sizeof(tt));
    if (n != sizeof(tt)) {
        if (n != 0)
            CERR << "failed to read " << sizeof(tt) << " bytes, result was " << n << ", errno=" << strerror(errno) << std::endl;
        return false;
    }

    t = byte_swap<host_endian, file_endian>(tt);

    return true;
}

template<>
bool Write<Header>(int fd, const Header &h) {
    ssize_t n = swrite(fd, h.Vistle, sizeof(h.Vistle));
    if (n != sizeof (h.Vistle))
        return false;
    char endianbyte = '0';
    if (h.endian == little_endian)
        endianbyte = 'L';
    if (h.endian == big_endian)
        endianbyte = 'B';
    if (!Write(fd, endianbyte))
        return false;
    if (!Write(fd, h.version))
        return false;
    if (!Write(fd, h.indexSize))
        return false;
    if (!Write(fd, h.scalarSize))
        return false;

    return true;
}

template<>
bool Read<Header>(int fd, Header &h) {
    Header hgood;
    ssize_t n = sread(fd, h.Vistle, sizeof(h.Vistle));
    if (n != sizeof (h.Vistle))
        return false;
    if (strncmp(h.Vistle, hgood.Vistle, sizeof(h.Vistle)) != 0)
        return false;
    char endianbyte;
    if (!Read(fd, endianbyte))
        return false;
    if (endianbyte == 'B')
        h.endian = big_endian;
    else if (endianbyte == 'L')
        h.endian = little_endian;
    else
        return false;
    if (!Read(fd, h.version))
        return false;
    if (!Read(fd, h.indexSize))
        return false;
    if (!Read(fd, h.scalarSize))
        return false;
    return true;
}

template<>
bool Write<std::string>(int fd, const std::string &s) {
    uint64_t l = s.length();
    if (!Write(fd, l)) {
        CERR << "failed to write string length" << std::endl;
        return false;
    }
    ssize_t n = swrite(fd, s.data(), s.length());
    if (n != s.length()) {
        CERR << "failed to write string data" << std::endl;
        return false;
    }

    return true;
}

template<>
bool Read<std::string>(int fd, std::string &s) {
    uint64_t l;
    if (!Read(fd, l)) {
        CERR << "failed to read string length" << std::endl;
        return false;
    }

    std::vector<char> d(l);
    ssize_t n = sread(fd, d.data(), d.size());
    if (n != d.size()) {
        CERR << "failed to read string data of size " << l << std::endl;
        return false;
    }

    s = std::string(d.data(), d.data()+d.size());

    return true;
}

template<>
bool Write<std::vector<char>>(int fd, const std::vector<char> &v) {
    uint64_t l = v.size();
    if (!Write(fd, l)) {
        CERR << "failed to write vector size" << std::endl;
        return false;
    }
    ssize_t n = swrite(fd, v.data(), v.size());
    if (n != v.size()) {
        CERR << "failed to write vector data" << std::endl;
        return false;
    }

    return true;
}

template<>
bool Read<std::vector<char>>(int fd, std::vector<char> &v) {
    uint64_t l;
    if (!Read(fd, l)) {
        CERR << "failed to read vector size" << std::endl;
        return false;
    }
    v.resize(l);
    ssize_t n = sread(fd, v.data(), v.size());
    if (n != v.size()) {
        CERR << "failed to read vector data of size " << l << std::endl;
        return false;
    }

    return true;
}

bool Write(int fd, const SubArchiveDirectoryEntry &ent, message::CompressionMode comp, int speed) {

    if (!Write(fd, ent.name)) {
        CERR << "failed to write entry name" << std::endl;
        return false;
    }

    char flag = ent.is_array ? 1 : 0;
    if (!Write(fd, flag)) {
        CERR << "failed to write entry array flag" << std::endl;
        return false;
    }

    uint64_t length = ent.size;
    if (!Write(fd, length)) {
        CERR << "failed to write entry length" << std::endl;
        return false;
    }

    std::vector<char> compressed;
    if (comp != message::CompressionNone) {
        compressed = message::compressPayload(comp, ent.data, ent.size, speed);
    }
    uint32_t compMode = comp;
    if (!Write(fd, compMode)) {
        CERR << "failed to write compression mode" << std::endl;
        return false;
    }

    uint64_t compLength = comp==message::CompressionNone ? ent.size : compressed.size();
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
    if (n != compLength) {
        CERR << "failed to write entry data of size " << compLength << " (raw: " << ent.size << "): result=" << n  << std::endl;
        if (n == -1)
            std::cerr << "  ERRNO=" << errno << ": " << strerror(errno) << std::endl;
        return false;
    }

    return true;
}

template<>
bool Read<SubArchiveDirectoryEntry>(int fd, SubArchiveDirectoryEntry &ent) {

    if (!Read(fd, ent.name)) {
        CERR << "failed to read entry name" << std::endl;
        return false;
    }

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

    ent.storage.reset(new std::vector<char>(compLength));
    ent.data = ent.storage->data();

    ssize_t n = sread(fd, ent.data, ent.compressedSize);
    if (n != ent.compressedSize) {
        CERR << "failed to read entry data" << std::endl;
        return false;
    }

#if 0
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

    return true;
}

}

bool Cache::compute() {

    if (m_fromDisk) {
        for (int i=0; i<NumPorts; ++i) {
            accept<Object>(m_inPort[i]);
        return true;
    }

    for (int i=0; i<NumPorts; ++i) {
        Object::const_ptr obj = accept<Object>(m_inPort[i]);
        if (obj) {
            passThroughObject(m_outPort[i], obj);

            if (m_toDisk) {
                assert(m_fd != -1);

                vecostreambuf<char> memstr;
                vistle::oarchive memar(memstr);
                memar.setCompressionSettings(m_compressionSettings);
                memar.setSaver(m_saver);
                obj->saveObject(memar);
                message::CompressionMode comp = message::CompressionMode(m_archiveCompression->getValue());
                int speed = m_archiveCompressionSpeed->getValue();
                const std::vector<char> &mem = memstr.get_vector();

                Header header;
                if (!Write(m_fd, header))
                    return false;

                uint32_t port(i);
                if (!Write(m_fd, port))
                    return false;

                int32_t timestep(obj->getTimestep());
                if (!Write(m_fd, timestep))
                    return false;

                uint32_t block(obj->getBlock());
                if (!Write(m_fd, block))
                    return false;

                auto dir = m_saver->getDirectory();
                uint32_t num = dir.size() + 1;
                if (!Write(m_fd, num))
                    return false;

                SubArchiveDirectoryEntry ent{obj->getName(), false, mem.size(), const_cast<char *>(mem.data())};
                if (!Write(m_fd, ent, comp, speed))
                    return false;

                for (const auto &ent: dir) {
                    if (!Write(m_fd, ent, comp, speed))
                        return false;
                }

                m_saver->flushDirectory();
            }
        }
    }

    return true;
}

bool Cache::prepare() {

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
            for (int i=0; i<NumPorts; ++i) {
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
        m_fd = open(file.c_str(), O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
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

    std::map<std::string, std::vector<char>> objects, arrays;
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

    auto restoreObject = [this, &compression, &size, &objects, &fetcher, start, stop, step, renumber](const std::string &name0, int port) {
        //CERR << "output to port " << port << ", " << num << " objects/arrays read" << std::endl;
        //CERR << "output to port " << port << ", initial " << name0 << " of size " << objects[name0].size() << std::endl;

        const auto &objbuf = objects[name0];
        std::vector<char> raw;
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
        const auto &buf = comp==message::CompressionNone ? objbuf : raw;
        vecistreambuf<char> membuf(buf);
        vistle::iarchive memar(membuf);
        memar.setFetcher(fetcher);
        //CERR << "output to port " << port << ", trying to load " << name0 << std::endl;
        Object::ptr obj(Object::loadObject(memar));
        updateMeta(obj);
        auto t = obj->getTimestep();
        if (renumber && t >= 0) {
            t -= start;
            t /= step;
            obj->setTimestep(t);
        }
        auto nt = obj->getNumTimesteps();
        if (renumber && nt >= 0) {
            if (nt > stop)
                nt = stop;
            nt -= start;
            nt = (nt+step-1)/step;
            obj->setNumTimesteps(nt);
        }
        if (auto db = DataBase::as(obj)) {
            if (auto cgrid = db->grid()) {
                auto grid = std::const_pointer_cast<Object>(cgrid);
                auto t = grid->getTimestep();
                if (renumber && t >= 0) {
                    t -= start;
                    t /= step;
                    grid->setTimestep(t);
                }
                auto nt = grid->getNumTimesteps();
                if (renumber && nt >= 0) {
                    if (nt > stop)
                        nt = stop;
                    nt -= start;
                    nt = (nt+step-1)/step;
                    grid->setNumTimesteps(nt);
                }
            }
        }
        passThroughObject(m_outPort[port], obj);
        fetcher->releaseArrays();
    };

    for (;;) {
        Header header, hgood;
        if (!Read(m_fd, header)) {
            break;
        }
        if (header.version != hgood.version) {
            sendError("Cannot read Vistle files of version %d, only %d is supported", (int)header.version, hgood.version);
            break;
        }
        if (header.endian != file_endian) {
            sendError("Cannot read Vistle file, unsupported endianness");
            break;
        }
        if (header.indexSize != hgood.indexSize) {
            sendError("Cannot read Vistle file, its index size %d does not match mine of %d", (int)header.indexSize, hgood.indexSize);
            break;
        }
        if (header.scalarSize != hgood.scalarSize) {
            sendError("Cannot read Vistle file, its scalar size %d does not match mine of %d", (int)header.scalarSize, hgood.scalarSize);
            break;
        }

        uint32_t port;
        if (!Read(m_fd, port)) {
            ok = false;
            break;
        }
        int32_t timestep;
        if (!Read(m_fd, timestep)) {
            ok = false;
            break;
        }
        uint32_t block;
        if (!Read(m_fd, block)) {
            ok = false;
            break;
        }
        uint32_t num;
        if (!Read(m_fd, num)) {
            ok = false;
            break;
        }

        //CERR << "output to port " << port << ", " << num << " objects/subobjects/arrays" << std::endl;

        std::string name0;
        for (size_t i=0; i<num; ++i) {
            SubArchiveDirectoryEntry ent;
            if (!Read(m_fd, ent)) {
                return false;
            }
            CERR << "entry " << ent.name << " of size " << ent.storage->size() << std::endl;
            if (i==0) {
                name0 = ent.name;
                assert(!ent.is_array);
            }
            compression[ent.name] = ent.compression;
            size[ent.name] = ent.size;
            if (ent.is_array) {
                arrays[ent.name] = std::move(*ent.storage);
            } else {
                objects[ent.name] = std::move(*ent.storage);
            }
        }

        int tplus = timestep+1;
        assert(tplus >= 0);
        if (numTime < tplus)
            numTime = tplus;
        if (portObjects[port].size() <= numTime) {
            portObjects[port].resize(numTime+1);
        }
        portObjects[port][tplus].push_back(name0);

        if (reorder)
            continue;

        if (!m_outPort[port]->isConnected()) {
            //CERR << "skipping " << name0 << ", output " << port << " not connected" << std::endl;
            continue;
        }

        if (timestep>=0 && timestep<start)
            continue;

        if (timestep>=0 && timestep>stop)
            continue;

        if (timestep>=0 && (timestep-start)%step != 0)
            continue;

        ++numObjects;

        restoreObject(name0, port);
    }

    if (reorder) {

        std::vector<int> timesteps;
        timesteps.push_back(-1);
        for (int timestep=0; timestep<numTime; ++timestep) {

            if (timestep<start)
                continue;

            if (timestep>stop)
                continue;

            if ((timestep-start)%step != 0)
                continue;

            timesteps.push_back(timestep);
        }

        for (auto &timestep: timesteps) {
            for (int port=0; port<NumPorts; ++port) {
                if (!m_outPort[port]->isConnected()) {
                    //CERR << "skipping " << name0 << ", output " << port << " not connected" << std::endl;
                    continue;
                }

                const auto &o = portObjects[port][timestep+1];
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

bool Cache::reduce(int t) {

    if (t != -1)
        return true;

    m_saver.reset();

    if (m_fd >= 0)
        close(m_fd);
    m_fd = -1;

    return true;
}

bool Cache::changeParameter(const Parameter *p) {

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

