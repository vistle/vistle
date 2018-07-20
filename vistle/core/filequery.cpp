#include "filequery.h"
#include <util/vecstreambuf.h>
#include "archives_config.h"
#include "archives.h"

namespace vistle {

std::vector<char> createPayload(const std::vector<FileInfo> &info) {
    vecostreambuf<char> buf;
    oarchive ar(buf);
    ar & Index(info.size());
    for (auto &i: info)
        ar & i;
    return buf.get_vector();
}

std::vector<FileInfo> unpackFileInfos(const std::vector<char> &payload) {
    std::vector<FileInfo> info;
    vecistreambuf<char> buf(payload);
    iarchive ar(buf);
    Index size=0;
    ar & size;
    info.resize(size);
    for (auto &i: info)
        ar & i;
    return info;
}

std::vector<char> createPayload(const SystemInfo &info)
{
    vecostreambuf<char> buf;
    oarchive ar(buf);
    ar & info;
    return buf.get_vector();
}

SystemInfo unpackSystemInfo(const std::vector<char> &payload)
{
    SystemInfo info;
    vecistreambuf<char> buf(payload);
    iarchive ar(buf);
    ar & info;
    return info;
}

std::vector<char> packFileList(const std::vector<std::string> &files)
{
    vecostreambuf<char> buf;
    oarchive ar(buf);
    ar & Index(files.size());
    for (auto &f: files)
        ar & f;
    return buf.get_vector();
}

std::vector<std::string> unpackFileList(const std::vector<char> &payload)
{
    std::vector<std::string> files;
    vecistreambuf<char> buf(payload);
    iarchive ar(buf);
    Index size=0;
    ar & size;
    files.resize(size);
    for (auto &f: files)
        ar & f;
    return files;
}

}
