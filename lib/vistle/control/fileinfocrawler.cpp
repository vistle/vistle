#include "fileinfocrawler.h"
#include <vistle/core/filequery.h>
#include <vistle/util/filesystem.h>
#include <vistle/util/hostname.h>

#include <future>

#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#define CERR std::cerr << "FileInfoCrawler: "

#ifndef FILEBROWSER_DEBUG
//#define FILEBROWSER_DEBUG
#endif


namespace vistle {

namespace fs = vistle::filesystem;
using message::FileQuery;
using message::FileQueryResult;

namespace {
FileQueryResult::Status readEntry(const fs::path &path, FileInfo &info)
{
    try {
        info.name = path.filename().string();

#ifdef WIN32
        info.casesensitive = false;
        info.hidden = false;
#else
        info.hidden = info.name.length() >= 1 && info.name[0] == '.';
#ifdef __APPLE__
        if (pathconf(path.c_str(), _PC_CASE_SENSITIVE)) {
            info.casesensitive = true;
        } else {
            info.casesensitive = false;
        }
#else
        info.casesensitive = true;
#endif
#endif

        info.symlink = false;
        auto stat = fs::status(path);
        if (stat.type() == fs::symlink_file) {
            info.symlink = true;
            stat = fs::symlink_status(path);
        }

        switch (stat.type()) {
        case fs::status_error:
            info.status = false;
            info.exists = false;
            return FileQueryResult::Error;
        case fs::file_not_found:
            info.status = true;
            info.exists = false;
            return FileQueryResult::DoesNotExist;
        default:
            info.status = true;
            info.exists = true;
            break;
        }

        info.permissions = -1;
        if (fs::permissions_present(stat)) {
            info.permissions = stat.permissions() & fs::all_all;
        }

        info.size = -1;
        switch (stat.type()) {
            {
            case fs::regular_file:
                info.type = FileInfo::File;
                info.size = fs::file_size(path);
                break;
            case fs::directory_file:
                info.type = FileInfo::Directory;
                break;
            case fs::symlink_file:
                CERR << "readEntry: symlink not fully followed: " << path.string() << std::endl;
                info.type = FileInfo::System;
                break;
            default:
                info.type = FileInfo::System;
                break;
            }
        }

        info.lastmod = fs::last_write_time(path);

        return FileQueryResult::Ok;

    } catch (fs::filesystem_error &ex) {
        CERR << "readEntry: exception " << ex.what() << ": " << path.string() << std::endl;
        info.status = false;
        return FileQueryResult::Error;
    }
}

std::vector<std::string> readDirectory(const std::string &path, message::FileQueryResult::Status &status)
{
    status = FileQueryResult::Ok;

    std::vector<std::string> results;
    if (path.empty()) {
        // this is a query for drives
#ifdef WIN32
        char physical[65536];
        QueryDosDevice(NULL, physical, sizeof(physical));
        for (char *pos = physical; *pos; pos += strlen(pos) + 1) {
            char logical[65536];
            QueryDosDevice(pos, logical, sizeof(logical));
            results.emplace_back(logical);
        }
#else
        results.emplace_back("/");
#endif
        return results;
    }

    FileInfo fi;
    status = readEntry(path, fi);
    if (status == FileQueryResult::Ok) {
        fs::path p(path);
        boost::system::error_code ec;
        auto it = fs::directory_iterator(p, ec);
        if (ec) {
            status = FileQueryResult::Error;
            if (ec == boost::system::errc::permission_denied)
                status = FileQueryResult::NoPermission;
            return results;
        }

        for (; it != fs::directory_iterator(); ++it) {
            results.push_back(it->path().filename().string());
        }
    }

    return results;
}

} // namespace

FileInfoCrawler::FileInfoCrawler(Hub &hub): m_hub(hub)
{}

bool FileInfoCrawler::handle(const message::FileQuery &query, const buffer &payload)
{
#ifdef FILEBROWSER_DEBUG
    CERR << "FileQuery(" << query.command() << ") for " << query.moduleId() << ":" << query.path() << std::endl;
#endif
    using namespace message;
    FileQueryResult::Status status = FileQueryResult::Ok;

    try {
        switch (query.command()) {
        case FileQuery::SystemInfo: {
            SystemInfo info;
            info.hostname = hostname();
#ifdef WIN32
            info.iswindows = true;

            if (const char *h = getenv("USERPROFILE"))
                info.homepath = h;
            std::replace(info.homepath.begin(), info.homepath.end(), '\\', '/');

            if (const char *u = getenv("USERNAME"))
                info.username = u;
#else
            info.iswindows = false;
            if (const char *h = getenv("HOME"))
                info.homepath = h;
            else
                info.homepath = "/";
            if (const char *u = getenv("USER"))
                info.username = u;


#endif
            info.currentdir = fs::current_path().string();
            auto payload = createPayload(info);
            return sendResponse(query, FileQueryResult::Ok, payload);
            break;
        }
        case FileQuery::LookUpFiles: {
            std::string path(query.path());
            if (!path.empty() && path.back() != '/')
                path += "/";
            auto files = unpackFileList(payload);
            std::mutex mtx;
            std::vector<FileInfo> results;
            std::vector<std::future<FileQueryResult::Status>> queryFutures;
            //CERR << "querying " << files.size() << " files in '" << query.path() << "'" << std::endl;
            for (const auto &f: files) {
                auto fn = path + f;
                queryFutures.emplace_back(
                    std::async(std::launch::async, [fn, &mtx, &results]() mutable -> FileQueryResult::Status {
                        FileInfo fi;
                        auto s = readEntry(fs::path(fn), fi);
                        if (s != FileQueryResult::Ok) {
#ifdef FILEBROWSER_DEBUG
                            CERR << "status not ok (" << vistle::message::FileQueryResult::toString(s) << ") for '"
                                 << fn << "'" << std::endl;
#endif
                        }
                        std::lock_guard<std::mutex> guard(mtx);
                        results.push_back(fi);
                        return s;
                    }));
            }
            for (auto &f: queryFutures) {
                f.get();
            }
            auto payload = createPayload(results);
            return sendResponse(query, status, payload);
            break;
        }
        case FileQuery::ReadDirectory: {
            auto results = readDirectory(query.path(), status);
            auto payload = packFileList(results);
            return sendResponse(query, status, payload);
            break;
        }
        case FileQuery::MakeDirectory: {
            break;
        }
        }
    } catch (fs::filesystem_error &) {
        status = FileQueryResult::Error;
        sendResponse(query, status, buffer());
    }

    return false;
}

bool FileInfoCrawler::sendResponse(const message::FileQuery &query, message::FileQueryResult::Status s,
                                   const buffer &payload)
{
    message::FileQueryResult response(query, s, payload.size());
    if (m_hub.id() == query.senderId()) {
        //CERR << "sending to " << query.uiId() << ", path=" << query.path() << ", payload sz=" << payload.size() << std::endl;
        return m_hub.sendUi(response, query.uiId(), &payload);
    }

    return m_hub.sendHub(query.senderId(), response, &payload);
}

} // namespace vistle
