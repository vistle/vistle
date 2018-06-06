#include "archive_loader.h"
#include "archives.h"
#include "object.h"

#include <iostream>

namespace vistle {

DeepArchiveFetcher::DeepArchiveFetcher(std::map<std::string, std::vector<char>>& objects, std::map<std::string, std::vector<char>>& arrays)
    : m_objects(objects), m_arrays(arrays) {}

void DeepArchiveFetcher::requestArray(const std::string & name, int type, const std::function<void()>& completeCallback) {
    //std::cerr << "DeepArchiveFetcher: trying array " << name << std::endl;
    auto it = m_arrays.find(name);
    if (it == m_arrays.end()) {
        std::cerr << "DeepArchiveFetcher: did not find array " << name << std::endl;
        return;
    }
    vecistreambuf<char> vb(it->second);
    iarchive ar(vb);
    ArrayLoader loader(name, type, ar);
    if (loader.load()) {
        //std::cerr << "DeepArchiveFetcher: success array " << name << std::endl;
        completeCallback();
    }
    else {
        std::cerr << "DeepArchiveFetcher: failed to load array " << name << std::endl;
    }
}

void DeepArchiveFetcher::requestObject(const std::string & name, const std::function<void()>& completeCallback) {
    std::cerr << "DeepArchiveFetcher: trying object " << name << std::endl;
    auto it = m_objects.find(name);
    if (it == m_objects.end()) {
        std::cerr << "DeepArchiveFetcher: did not find object " << name << std::endl;
        return;
    }
    vecistreambuf<char> vb(it->second);
    iarchive ar(vb);
    ar.setFetcher(shared_from_this());
    Object::ptr obj(Object::loadObject(ar));
    if (obj->isComplete()) {
        //std::cerr << "DeepArchiveFetcher: success object " << name << std::endl;
        completeCallback();
    }
    else {
        std::cerr << "DeepArchiveFetcher: failed to load object " << name << std::endl;
    }
}

}
