#include "archive_saver.h"
#include "archives.h"
#include "shmvector.h"
#include "object.h"

namespace vistle {

void DeepArchiveSaver::saveArray(const std::string & name, int type, const void * array) {
    if (m_arrays.find(name) != m_arrays.end())
        return;

    vecostreambuf<char> vb;
    oarchive ar(vb);
    ar.setSaver(shared_from_this());
    ArraySaver as(name, type, ar, array);
    if (as.save()) {
        m_arrays.emplace(name, vb.get_vector());
    }
}

void vistle::DeepArchiveSaver::saveObject(const std::string & name, Object::const_ptr obj) {
    if (m_objects.find(name) != m_objects.end())
        return;

    vecostreambuf<char> vb;
    oarchive ar(vb);
    ar.setSaver(shared_from_this());
    obj->saveObject(ar);
    m_objects.emplace(name, vb.get_vector());
}

SubArchiveDirectory DeepArchiveSaver::getDirectory() {
    SubArchiveDirectory dir;
    for (auto &obj : m_objects) {
        dir.emplace_back(obj.first, false, obj.second.size(), obj.second.data());
    }
    for (auto &arr : m_arrays) {
        dir.emplace_back(arr.first, true, arr.second.size(), arr.second.data());
    }
    return dir;
}

}
