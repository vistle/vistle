#include "fileinfocrawler.h"
#include <core/filequery.h>
#include <util/filesystem.h>

namespace vistle {

namespace fs = vistle::filesystem;
using message::FileQuery;
using message::FileQueryResult;

namespace {
FileQueryResult::Status readEntry(const fs::path &path, FileInfo &info) {

    try {

        info.name = path.filename().string();

#ifdef WIN32
        info.casesensitive = false;
        info.hidden = false;
#else
        info.hidden = info.name.length()>1 && info.name[0] == '.' && info.name != "..";
        info.casesensitive = true;
#endif

        info.symlink = false;
        auto stat = fs::status(path);
        if (stat.type() == fs::symlink_file) {
            info.symlink = true;
            stat = fs::symlink_status(path);
        }

        switch (stat.type()) {
        case fs::status_error:
            info.exists = false;
            return FileQueryResult::Error;
        case fs::file_not_found:
            info.exists = false;
            return FileQueryResult::DoesNotExist;
        default:
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
                std::cerr << "FileInfoCrawler: symlink not fully followed" << std::endl;
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
        std::cerr << "FileInfoCrawler: exception " << ex.what() << std::endl;
        return FileQueryResult::Error;
    }
}

std::vector<std::string> readDirectory(const std::string &path, message::FileQueryResult::Status &status)
{
    status = FileQueryResult::Ok;

    std::vector<std::string> results;
    if (path.empty()) {
        results.push_back("/");
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

        for ( ; it != fs::directory_iterator(); ++it) {
            results.push_back(it->path().filename().string());
        }
    }

    return results;
}

}

FileInfoCrawler::FileInfoCrawler(Hub &hub)
: m_hub(hub)
{
}

bool FileInfoCrawler::handle(const message::FileQuery &query, const std::vector<char> &payload)
{
    using namespace message;
    FileQueryResult::Status status = FileQueryResult::Ok;

    try {
        switch (query.command()) {
        case FileQuery::SystemInfo: {
            SystemInfo info;
#ifdef WIN32
            info.iswindows = true;
#else
            info.iswindows = false;
            if (const char *h = getenv("HOME"))
                info.homepath = h;
            else
                info.homepath = "/";
#endif
            info.currentdir = fs::current_path().string();
            auto payload = createPayload(info);
            return sendResponse(query, FileQueryResult::Ok, payload);
            break;
        }
        case FileQuery::LookUpFiles: {
            const std::string path(query.path());
            auto files = unpackFileList(payload);
            std::vector<FileInfo> results;
            for (const auto &f: files) {
                auto fn = path + "/" + f;
                std::cerr << "LookUpFiles: name=" << fn << std::endl;
                FileInfo fi;
                status = readEntry(fs::path(fn), fi);
                if (status != FileQueryResult::Ok) {
                    break;
                }
                results.push_back(fi);
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
    } catch (fs::filesystem_error &ex) {
        status = FileQueryResult::Error;
        sendResponse(query, status, std::vector<char>());
    }

    return false;
}

bool FileInfoCrawler::sendResponse(const message::FileQuery &query, message::FileQueryResult::Status s, const std::vector<char> &payload)
{
    message::FileQueryResult response(query, s, payload.size());
    if (m_hub.id() == query.senderId()) {
        std::cerr << "sending to " << query.uiId() << ", path=" << query.path() << ", payload sz=" << payload.size() << std::endl;
        return m_hub.sendUi(response, query.uiId(), &payload);
    }

    return m_hub.sendHub(response, query.senderId(), &payload);
}

}
