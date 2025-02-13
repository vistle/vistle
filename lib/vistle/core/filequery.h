#ifndef VISTLE_CORE_FILEQUERY_H
#define VISTLE_CORE_FILEQUERY_H

#include <cstdint>
#include <string>
#include <vector>

#include "archives_config.h"
#include "export.h"

#include <vistle/util/buffer.h>

namespace vistle {

struct V_COREEXPORT SystemInfo {
    bool iswindows;
    std::string hostname;
    std::string username;
    std::string homepath;
    std::string currentdir;

    ARCHIVE_ACCESS
    template<class Archive>
    void serialize(Archive &ar)
    {
        ar &iswindows;
        ar &hostname;
        ar &username;
        ar &homepath;
        ar &currentdir;
    }
};

struct V_COREEXPORT FileInfo {
    enum FileType {
        File,
        Directory,
        System,
    };

    std::string name;
    bool status = false;
    bool exists = false;
    int32_t permissions = 0;
    FileType type = System;
    bool symlink = false;
    bool hidden = false;
    bool casesensitive = true;
    int64_t size = -1;
    double lastmod = 0.;

    ARCHIVE_ACCESS
    template<class Archive>
    void serialize(Archive &ar)
    {
        ar &name;
        ar &status;
        ar &exists;
        ar &permissions;
        ar &type;
        ar &symlink;
        ar &hidden;
        ar &casesensitive;
        ar &size;
        ar &lastmod;
    }
};

buffer V_COREEXPORT createPayload(const std::vector<FileInfo> &info);
std::vector<FileInfo> V_COREEXPORT unpackFileInfos(const buffer &payload);

buffer V_COREEXPORT createPayload(const SystemInfo &info);
SystemInfo V_COREEXPORT unpackSystemInfo(const buffer &payload);

buffer V_COREEXPORT packFileList(const std::vector<std::string> &files);
std::vector<std::string> V_COREEXPORT unpackFileList(const buffer &payload);

} // namespace vistle
#endif
