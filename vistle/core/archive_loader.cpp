#include "archive_loader.h"
#include "archives.h"
#include "object.h"

#include <iostream>

namespace vistle {

DeepArchiveFetcher::DeepArchiveFetcher(const std::map<std::string, std::vector<char> > &objects, const std::map<std::string, std::vector<char> > &arrays)
    : m_objects(objects), m_arrays(arrays) {}

void DeepArchiveFetcher::requestArray(const std::string &arname, int type, const ArrayCompletionHandler& completeCallback) {
    //std::cerr << "DeepArchiveFetcher: trying array " << arname << std::endl;
    auto it = m_arrays.find(arname);
    if (it == m_arrays.end()) {
        std::cerr << "DeepArchiveFetcher: did not find array " << arname << std::endl;
        return;
    }
    vecistreambuf<char> vb(it->second);
    iarchive ar(vb);
    ar.setFetcher(shared_from_this());
    ArrayLoader loader(arname, type, ar);
    if (loader.load()) {
        //std::cerr << "DeepArchiveFetcher: success array " << arname << std::endl;
        m_ownedArrays.emplace(loader.owner());
        completeCallback(ar.translateArrayName(arname));
    }
    else {
        std::cerr << "DeepArchiveFetcher: failed to load array " << arname << std::endl;
    }
}

void DeepArchiveFetcher::requestObject(const std::string &arname, const ObjectCompletionHandler &completeCallback) {
    //std::cerr << "DeepArchiveFetcher: trying object " << arname << std::endl;
    auto it = m_objects.find(arname);
    if (it == m_objects.end()) {
        std::cerr << "DeepArchiveFetcher: did not find object " << arname << std::endl;
        return;
    }
    vecistreambuf<char> vb(it->second);
    iarchive ar(vb);
    ar.setFetcher(shared_from_this());
    Object::ptr obj(Object::loadObject(ar));
    if (obj && obj->isComplete()) {
        //std::cerr << "DeepArchiveFetcher: success object " << arname << ", created as " << obj->getName() << std::endl;
        completeCallback(obj);
    }
    else {
        std::cerr << "DeepArchiveFetcher: failed to load object " << arname << std::endl;
    }
}

bool DeepArchiveFetcher::renameObjects() const {
    return m_rename;
}

std::string DeepArchiveFetcher::translateObjectName(const std::string &name) const {
    if (!m_rename)
        return name;

    auto it = m_transObject.find(name);
    if (it == m_transObject.end())
        return std::string();

    return it->second;
}

std::string DeepArchiveFetcher::translateArrayName(const std::string &name) const {
    if (!m_rename)
        return name;

    auto it = m_transArray.find(name);
    if (it == m_transArray.end())
        return std::string();

    return it->second;
}

void DeepArchiveFetcher::registerObjectNameTranslation(const std::string &arname, const std::string &name) {
    if (!m_rename)
        return;
    auto p = m_transObject.emplace(arname, name);

    if (p.second) {
        //std::cerr << "yas_iarchive: object name translation for " << arname << " was already registered" << std::endl;
    }
}

void DeepArchiveFetcher::registerArrayNameTranslation(const std::string &arname, const std::string &name) {
    if (!m_rename)
        return;

    auto p = m_transArray.emplace(arname, name);
    if (p.second) {
        //std::cerr << "yas_iarchive: array name translation for " << arname << " was already registered" << std::endl;
    }
}

void DeepArchiveFetcher::setRenameObjects(bool rename) {
    m_rename = rename;
}

std::map<std::string, std::string> DeepArchiveFetcher::objectTranslations() const {
    return m_transObject;
}

std::map<std::string, std::string> DeepArchiveFetcher::arrayTranslations() const {
    return m_transArray;
}

void DeepArchiveFetcher::setObjectTranslations(const std::map<std::string, std::string> &objs) {
    m_transObject = objs;
}

void DeepArchiveFetcher::setArrayTranslations(const std::map<std::string, std::string> &arrs) {
    m_transArray = arrs;
}

void DeepArchiveFetcher::releaseArrays() {

    m_ownedArrays.clear();
}

ArrayLoader::ArrayLoader(const std::string &name, int type, const iarchive &ar): m_ok(false), m_arname(name), m_type(type), m_ar(ar) {
    m_name = m_ar.translateArrayName(m_arname);
    //std::cerr << "ArrayLoader: loading array " << m_arname << ", translates to " << m_name << std::endl;
}

bool ArrayLoader::load() {
    try {
        boost::mpl::for_each<VectorTypes>(boost::reference_wrapper<ArrayLoader>(*this));
        if (!m_ok) {
            std::cerr << "ArrayLoader: failed to restore array " << m_name << " from archive" << std::endl;
        }
    } catch (std::exception &ex) {
        m_ok = false;
            std::cerr << "ArrayLoader: failed to restore array " << m_name << " from archive - exception: " << ex.what() << std::endl;
    }
    return m_ok;
}

const std::string &ArrayLoader::name() const {
    return m_name;
}

std::shared_ptr<ArrayLoader::ArrayOwner> ArrayLoader::owner() const {
    return m_unreffer;
}

}
