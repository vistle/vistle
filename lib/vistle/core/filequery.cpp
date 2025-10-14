#include "filequery.h"
#include <vistle/util/vecstreambuf.h>
#include "archives_config.h"
#include "archives.h"

namespace vistle {

buffer createPayload(const std::vector<FileInfo> &info)
{
    vecostreambuf<buffer> buf;
    oarchive ar(buf);
    ar &Index64(info.size());
    for (auto &i: info)
        ar &i;
    return buf.get_vector();
}

std::vector<FileInfo> unpackFileInfos(const buffer &payload)
{
    std::vector<FileInfo> info;
    try {
        vecistreambuf<buffer> buf(payload);
        iarchive ar(buf);
        Index64 size = 0;
        ar &size;
        info.resize(size);
        for (auto &i: info)
            ar &i;
    } catch (std::exception &ex) {
        std::cerr << "unpackFileInfos: unhandled exception " << ex.what() << std::endl;
    }
    return info;
}

buffer createPayload(const SystemInfo &info)
{
    vecostreambuf<buffer> buf;
    oarchive ar(buf);
    ar &info;
    return buf.get_vector();
}

SystemInfo unpackSystemInfo(const buffer &payload)
{
    SystemInfo info;
    try {
        vecistreambuf<buffer> buf(payload);
        iarchive ar(buf);
        ar &info;
    } catch (std::exception &ex) {
        std::cerr << "unpackSystemInfo: unhandled exception " << ex.what() << std::endl;
    }
    return info;
}

buffer packFileList(const std::vector<std::string> &files)
{
    vecostreambuf<buffer> buf;
    oarchive ar(buf);
    ar &Index64(files.size());
    for (auto &f: files)
        ar &f;
    return buf.get_vector();
}

std::vector<std::string> unpackFileList(const buffer &payload)
{
    std::vector<std::string> files;
    try {
        vecistreambuf<buffer> buf(payload);
        iarchive ar(buf);
        Index64 size = 0;
        ar &size;
        files.resize(size);
        for (auto &f: files)
            ar &f;
    } catch (std::exception &ex) {
        std::cerr << "unpackFileList: unhandled exception " << ex.what() << std::endl;
    }
    return files;
}

} // namespace vistle
