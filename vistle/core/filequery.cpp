#include "filequery.h"
#include <util/vecstreambuf.h>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

namespace vistle {

std::vector<char> createPayload(const std::vector<FileInfo> &info) {
    vecostreambuf<char> buf;
    boost::archive::binary_oarchive ar(buf);
    ar & info.size();
    for (auto &i: info)
        ar & i;
    return buf.get_vector();
}

std::vector<FileInfo> unpackFileInfos(const std::vector<char> &payload) {
    std::vector<FileInfo> info;
    vecistreambuf<char> buf(payload);
    boost::archive::binary_iarchive ar(buf);
    size_t size=0;
    ar & size;
    info.resize(size);
    for (auto &i: info)
        ar & i;
    return info;
}

std::vector<char> createPayload(const SystemInfo &info)
{
    vecostreambuf<char> buf;
    boost::archive::binary_oarchive ar(buf);
    ar & info;
    return buf.get_vector();
}

SystemInfo unpackSystemInfo(const std::vector<char> &payload)
{
    SystemInfo info;
    vecistreambuf<char> buf(payload);
    boost::archive::binary_iarchive ar(buf);
    ar & info;
    return info;
}

std::vector<char> packFileList(const std::vector<std::string> &files)
{
    vecostreambuf<char> buf;
    boost::archive::binary_oarchive ar(buf);
    ar & files.size();
    for (auto &f: files)
        ar & f;
    return buf.get_vector();
}

std::vector<std::string> unpackFileList(const std::vector<char> &payload)
{
    std::vector<std::string> files;
    vecistreambuf<char> buf(payload);
    boost::archive::binary_iarchive ar(buf);
    size_t size=0;
    ar & size;
    files.resize(size);
    for (auto &f: files)
        ar & f;
    return files;
}

}
