#include <vistle/module/module.h>

#include <vistle/core/archive_saver.h>
#include <vistle/core/archive_loader.h>

#include <vistle/util/enum.h>
#include <vistle/util/byteswap.h>
#include <vistle/util/fileio.h>

#include "vistle_file.h"

using namespace vistle;

static const int NumPorts = 5;


DEFINE_ENUM_WITH_STRING_CONVERSIONS(OperationMode, (Memory)(From_Disk)(To_Disk)(Automatic))

class Cache: public vistle::Module, public vistle::ArchiveCompressionSettings {
public:
    Cache(const std::string &name, int moduleID, mpi::communicator comm);
    ~Cache();

    message::CompressionMode archiveCompression() const override;
    int archiveCompressionSpeed() const override;

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
        addIntParameter("field_compression", "compression mode for data fields", Predict, Parameter::Choice);
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
    m_compressionSettings.mode = (FieldCompressionMode)m_compressionMode->getValue();
    m_compressionSettings.zfpRate = m_zfpRate->getValue();
    m_compressionSettings.zfpAccuracy = m_zfpAccuracy->getValue();
    m_compressionSettings.zfpPrecision = m_zfpPrecision->getValue();

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
    file += ".vsld";

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
        //std::cerr << "output to port " << port << ", " << num << " objects/arrays read" << std::endl;
        //std::cerr << "output to port " << port << ", initial " << name0 << " of size " << objects[name0].size() << std::endl;

        const auto &objbuf = objects[name0];
        buffer raw;
        const auto &comp = compression[name0];
        if (comp != message::CompressionNone) {
            const auto &sz = size[name0];
            try {
                raw = message::decompressPayload(comp, objbuf.size(), sz, objbuf.data());
            } catch (const std::exception &ex) {
                std::cerr << "failed to decompress object " << name0 << ": " << ex.what() << std::endl;
                return;
            }
        }
        const auto &buf = comp == message::CompressionNone ? objbuf : raw;
        vecistreambuf<buffer> membuf(buf);
        vistle::iarchive memar(membuf);
        memar.setFetcher(fetcher);
        //std::cerr << "output to port " << port << ", trying to load " << name0 << std::endl;
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
        std::string mode = "";

        m_mode = OperationMode(p_mode->getValue());
        switch (m_mode) {
        case Memory:
            m_fromDisk = m_toDisk = false;
            mode = "Mem";
            break;
        case To_Disk:
            m_fromDisk = false;
            m_toDisk = true;
            mode = "Write";
            break;
        case From_Disk:
            m_fromDisk = true;
            m_toDisk = false;
            mode = "Read";
            break;
        case Automatic:
            m_fromDisk = m_toDisk = false;
            mode = "Auto";
            break;
        }

        setItemInfo(mode);
    }

    return Module::changeParameter(p);
}

MODULE_MAIN(Cache)
