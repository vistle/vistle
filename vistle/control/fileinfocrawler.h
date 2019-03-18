#ifndef VISTLE_FILEINFO_CRAWLER_H
#define VISTLE_FILEINFO_CRAWLER_H

#include <core/filequery.h>
#include <core/messages.h>

#include "hub.h"

namespace vistle {

class FileInfoCrawler {
public:

    FileInfoCrawler(Hub &hub);
    bool handle(const message::FileQuery &query, const std::vector<char> &payload);

private:
    bool sendResponse(const message::FileQuery &query, message::FileQueryResult::Status s, const std::vector<char> &payload);

    Hub &m_hub;
};

}
#endif
