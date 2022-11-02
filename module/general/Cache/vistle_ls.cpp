#include "vistle_file.h"
#include <iostream>
#include <iomanip>
#include <fcntl.h>
#include <cstring>
#include <vistle/core/archives_config.h>
#include <vistle/core/archive_saver.h>
#include <vistle/core/archive_loader.h>
#include <vistle/core/shm.h>

using namespace vistle;

static const int NumPorts = 6;

int main(int argc, char *argv[])
{
    std::map<int, std::string> comp;
    comp[message::CompressionNone] = "raw";
    comp[message::CompressionZstd] = "zstd";
    comp[message::CompressionSnappy] = "snappy";
    comp[message::CompressionLz4] = "lz4";

    bool deep = false;
    int filenum = 1;

    if (argc > 1) {
        if (argv[1] == std::string("-d")) {
            deep = true;
            ++filenum;

            vistle::Shm::remove("vistle_ls", 1, 0, false);
            vistle::Shm::create("vistle_ls", 1, 0, false);
            vistle::registerTypes();
        }
    }

    if (filenum >= argc) {
        std::cerr << "filename argument required" << std::endl;
        return 1;
    }

    for (; filenum < argc; ++filenum) {
        std::string filename(argv[filenum]);

        auto fd = open(filename.c_str(), O_RDONLY);
        if (fd == -1) {
            std::cerr << "could not open " << filename << ": " << strerror(errno) << std::endl;
            return 1;
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

        auto restoreObject = [&compression, &size, &objects, &fetcher](const std::string &name0, int port) {
            //std::cerr << "output to port " << port << ", " << num << " objects/arrays read" << std::endl;
            //std::cerr << "output to port " << port << ", root " << name0 << " of size " << objects[name0].size() << std::endl;

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
            //std::cerr << "output to port " << port << ", loaded " << name0 << std::endl;
            std::cout << "\t" << *obj << std::endl;
            fetcher->releaseArrays();
        };


        size_t totalRawSize = 0, totalCompressedSize = 0;
        int numArrays = 0;
        int numObjects = 0;
        int numRootObjects = 0;
        int numTime = 0;
        std::set<int> usedPorts;
        int start = 0, stop = 1 << 20, step = 1;
        std::string objectToRestore;
        for (bool error = false; !error;) {
            ChunkHeader cheader, chgood;
            if (!Read(fd, cheader)) {
                break;
            }
            if (cheader.version != chgood.version) {
                fprintf(stderr, "Cannot read Vistle files of version %d, only %d is supported", (int)cheader.version,
                        chgood.version);
                break;
            }
            //std::cerr << "ChunkHeader: " << cheader << std::endl;
            //std::cerr << "ChunkHeader: found type=" << cheader.type << std::endl;

            switch (cheader.type) {
            case ChunkType::Archive: {
                SubArchiveDirectoryEntry ent;
                if (!ReadChunk(nullptr, fd, cheader, ent)) {
                    std::cerr << "failed to read Archive chunk" << std::endl;
                    error = true;
                    continue;
                }
                std::cout << std::left << std::setw(20) << ent.name << std::right << (ent.is_array ? " a " : " o ")
                          << std::setw(10) << ent.size << " " << std::setw(7) << std::left << comp[ent.compression]
                          << std::right;
                if (ent.compression != message::CompressionNone) {
                    std::cout << " " << std::setw(10) << ent.compressedSize;
                    std::cout << " (" << std::setprecision(3) << 100. * ent.compressedSize / ent.size << "%)";
                }
                std::cout << std::endl;
                totalRawSize += ent.size;
                totalCompressedSize += ent.compressedSize;
                if (ent.is_array) {
                    ++numArrays;
                } else {
                    ++numObjects;
                }
                if (deep) {
                    compression[ent.name] = ent.compression;
                    size[ent.name] = ent.size;
                    if (ent.is_array) {
                        arrays[ent.name] = std::move(*ent.storage);
                    } else {
                        objects[ent.name] = std::move(*ent.storage);
                    }
                }
                break;
            }
            case ChunkType::PortObject: {
                PortObjectHeader poh;
                if (!ReadChunk(nullptr, fd, cheader, poh)) {
                    std::cerr << "failed to read PortObject chunk" << std::endl;
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

                if (poh.timestep >= 0 && poh.timestep < start)
                    continue;

                if (poh.timestep >= 0 && poh.timestep > stop)
                    continue;

                if (poh.timestep >= 0 && (poh.timestep - start) % step != 0)
                    continue;

                usedPorts.insert(poh.port);
                ++numRootObjects;
                std::cout << "\t" << poh.object.str() << ", t=" << poh.timestep << " -> port " << poh.port << std::endl;

                if (deep) {
                    restoreObject(objectToRestore, poh.port);
                }
                break;
            }
            case ChunkType::Directory: {
                break;
            }
            default: {
                std::cerr << "unknown chunk type " << cheader.type << std::endl;
                if (!SkipChunk(nullptr, fd, cheader)) {
                    std::cerr << "failed to skip unknown chunk" << std::endl;
                    error = true;
                    continue;
                }
                break;
            }
            }
        }
        close(fd);

        std::cout << totalRawSize << " data bytes";
        if (totalCompressedSize != totalRawSize) {
            std::cout << " (" << totalCompressedSize << " compressed, " << std::setprecision(3)
                      << 100. * totalCompressedSize / totalRawSize << "%)";
        }
        std::cout << std::endl;
        std::cout << numArrays << " arrays, " << numObjects << " objects (" << numRootObjects << " root) on "
                  << usedPorts.size() << " ports" << std::endl;
    }

    if (deep) {
        vistle::Shm::the().detach();
        vistle::Shm::remove("vistle_ls", 1, 0, false);
    }

    return 0;
}
