#include <boost/lexical_cast.hpp>
#include <module/module.h>

#include <core/archive_saver.h>
#include <core/archive_loader.h>

#include <util/enum.h>
#include <util/byteswap.h>

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

   int m_fd = -1;
   std::shared_ptr<DeepArchiveSaver> m_saver;

   vistle::Port *m_inPort[NumPorts], *m_outPort[NumPorts];
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
   p_file = addStringParameter("file", "filename where cache should be created", "", Parameter::Filename);
   setParameterFilters(p_file, "Vistle Data (*.vsld)/All Files (*)");
}

Cache::~Cache() {

}

#define CERR std::cerr << "Cache: "

struct Header {
    endianness endian = file_endian;
    char Vistle[7] = "vistle";
    Index version = 1;
};

template<class T>
bool Write(int fd, const T &t) {
    T tt = byte_swap<host_endian, file_endian>(t);
    ssize_t n = write(fd, &tt, sizeof(tt));
    if (n != sizeof(tt)) {
        CERR << "failed to write " << sizeof(tt) << " bytes, result was " << n << ", errno=" << strerror(errno) << std::endl;
        return false;
    }

    return true;
}

template<class T>
bool Read(int fd, T &t) {
    T tt;
    ssize_t n = read(fd, &tt, sizeof(tt));
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
    ssize_t n = write(fd, h.Vistle, sizeof(h.Vistle));
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

    return true;
}

template<>
bool Read<Header>(int fd, Header &h) {
    Header hgood;
    ssize_t n = read(fd, h.Vistle, sizeof(h.Vistle));
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
    return true;
}

template<>
bool Write<std::string>(int fd, const std::string &s) {
    Index l = s.length();
    if (!Write(fd, l)) {
        CERR << "failed to write string length" << std::endl;
        return false;
    }
    ssize_t n = write(fd, s.data(), s.length());
    if (n != s.length()) {
        CERR << "failed to write string data" << std::endl;
        return false;
    }

    return true;
}

template<>
bool Read<std::string>(int fd, std::string &s) {
    Index l;
    if (!Read(fd, l)) {
        CERR << "failed to read string length" << std::endl;
        return false;
    }

    std::vector<char> d(l);
    ssize_t n = read(fd, d.data(), d.size());
    if (n != d.size()) {
        CERR << "failed to read string data of size " << l << std::endl;
        return false;
    }

    s = std::string(d.data(), d.data()+d.size());

    return true;
}

template<>
bool Write<std::vector<char>>(int fd, const std::vector<char> &v) {
    Index l = v.size();
    if (!Write(fd, l)) {
        CERR << "failed to write vector size" << std::endl;
        return false;
    }
    ssize_t n = write(fd, v.data(), v.size());
    if (n != v.size()) {
        CERR << "failed to write vector data" << std::endl;
        return false;
    }

    return true;
}

template<>
bool Read<std::vector<char>>(int fd, std::vector<char> &v) {
    Index l;
    if (!Read(fd, l)) {
        CERR << "failed to read vector size" << std::endl;
        return false;
    }
    v.resize(l);
    ssize_t n = read(fd, v.data(), v.size());
    if (n != v.size()) {
        CERR << "failed to read vector data of size " << l << std::endl;
        return false;
    }

    return true;
}

template<>
bool Write<SubArchiveDirectoryEntry>(int fd, const SubArchiveDirectoryEntry &ent) {

    if (!Write(fd, ent.name)) {
        CERR << "failed to write entry name" << std::endl;
        return false;
    }

    char flag = ent.is_array ? 1 : 0;
    if (!Write(fd, flag)) {
        CERR << "failed to write entry array flag" << std::endl;
        return false;
    }

    Index length = ent.size;
    if (!Write(fd, length)) {
        CERR << "failed to write entry length" << std::endl;
        return false;
    }

    ssize_t n = write(fd, ent.data, ent.size);
    if (n != ent.size) {
        CERR << "failed to write entry data of size " << ent.size << ": result=" << n  << std::endl;
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

    Index length;
    if (!Read(fd, length)) {
        CERR << "failed to read entry length" << std::endl;
        return false;
    }
    ent.size = length;

    ent.storage.reset(new std::vector<char>(length));
    ent.data = ent.storage->data();

    ssize_t n = read(fd, ent.data, ent.size);
    if (n != ent.size) {
        CERR << "failed to read entry data" << std::endl;
        return false;
    }

    return true;
}

bool Cache::compute() {

    if (m_fromDisk) {
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
                memar.setSaver(m_saver);
                obj->saveObject(memar);
                const std::vector<char> &mem = memstr.get_vector();

                Header header;
                if (!Write(m_fd, header))
                    return false;

                Index port(i);
                if (!Write(m_fd, port))
                    return false;

                SIndex timestep(obj->getTimestep());
                if (!Write(m_fd, timestep))
                    return false;
                Index block(obj->getBlock());
                if (!Write(m_fd, block))
                    return false;

                auto dir = m_saver->getDirectory();
                Index num = dir.size() + 1;
                if (!Write(m_fd, num))
                    return false;

                SubArchiveDirectoryEntry ent{obj->getName(), false, mem.size(), const_cast<char *>(mem.data())};
                if (!Write(m_fd, ent))
                    return false;

                for (const auto &ent: dir) {
                    if (!Write(m_fd, ent))
                        return false;
                }

                m_saver->flushDirectory();
            }
        }
    }

    return true;
}

bool Cache::prepare() {

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
        m_fd = open(file.c_str(), O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    }

    if (!m_fromDisk)
        return true;

    m_fd = open(file.c_str(), O_RDONLY);
    if (m_fd == -1) {
        sendError("Could not open %s: %s", file.c_str(), strerror(errno));
        return true;
    }

    std::map<std::string, std::vector<char>> objects, arrays;
    std::map<std::string, std::string> objectTranslations, arrayTranslations;
    auto fetcher = std::make_shared<DeepArchiveFetcher>(objects, arrays);
    fetcher->setRenameObjects(true);
    fetcher->setObjectTranslations(objectTranslations);
    fetcher->setArrayTranslations(arrayTranslations);
    bool ok = true;
    int numObjects = 0;
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

        Index port;
        if (!Read(m_fd, port)) {
            ok = false;
            break;
        }
        SIndex timestep;
        if (!Read(m_fd, timestep)) {
            ok = false;
            break;
        }
        Index block;
        if (!Read(m_fd, block)) {
            ok = false;
            break;
        }
        Index num;
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
            //CERR << "entry " << ent.name << " of size " << ent.storage->size() << std::endl;
            if (i==0) {
                name0 = ent.name;
                assert(!ent.is_array);
            }
            if (ent.is_array) {
                arrays[ent.name] = std::move(*ent.storage);
            } else {
                objects[ent.name] = std::move(*ent.storage);
            }
        }

        ++numObjects;
        if (!m_outPort[port]->isConnected()) {
            //CERR << "skipping " << name0 << ", output " << port << " not connected" << std::endl;
            continue;
        }

        //CERR << "output to port " << port << ", " << num << " objects/arrays read" << std::endl;
        //CERR << "output to port " << port << ", initial " << name0 << " of size " << objects[name0].size() << std::endl;

        vecistreambuf<char> membuf(objects[name0]);
        vistle::iarchive memar(membuf);
        memar.setFetcher(fetcher);
        //CERR << "output to port " << port << ", trying to load " << name0 << std::endl;
        Object::ptr obj(Object::loadObject(memar));
        updateMeta(obj);
        passThroughObject(m_outPort[port], obj);
        fetcher->releaseArrays();
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

