#include "archive_saver.h"
#include "archives.h"
#include "shmvector.h"
#include "shm_array_impl.h"
#include "object.h"
#include "archives_impl.h"

namespace vistle {

void DeepArchiveSaver::saveArray(const std::string &name, int type, const void *array)
{
    if (isArraySaved(name))
        return;

    vecostreambuf<buffer> vb;
    oarchive ar(vb);
    ar.setCompressionSettings(m_compressionSettings);
    ar.setSaver(shared_from_this());
    ArraySaver as(name, type, ar, array);
    if (as.save()) {
        m_arrays.emplace(name, std::move(vb.get_vector()));
    }
}

void vistle::DeepArchiveSaver::saveObject(const std::string &name, Object::const_ptr obj)
{
    if (isObjectSaved(name))
        return;

    vecostreambuf<buffer> vb;
    oarchive ar(vb);
    ar.setCompressionSettings(m_compressionSettings);
    ar.setSaver(shared_from_this());
    obj->saveObject(ar);
    m_objects.emplace(name, std::move(vb.get_vector()));
}

SubArchiveDirectory DeepArchiveSaver::getDirectory()
{
    SubArchiveDirectory dir;
    dir.reserve(m_objects.size() + m_arrays.size());
    for (auto &obj: m_objects) {
        dir.emplace_back(obj.first, false, obj.second.size(), obj.second.data());
    }
    for (auto &arr: m_arrays) {
        dir.emplace_back(arr.first, true, arr.second.size(), arr.second.data());
    }
    return dir;
}

void DeepArchiveSaver::flushDirectory()
{
    for (const auto &obj: m_objects) {
        m_archivedObjects.emplace(obj.first);
    }
    m_objects.clear();
    for (const auto &arr: m_arrays) {
        m_archivedArrays.emplace(arr.first);
    }
    m_arrays.clear();
}

bool DeepArchiveSaver::isObjectSaved(const std::string &name) const
{
    if (m_objects.find(name) != m_objects.end())
        return true;
    if (m_archivedObjects.find(name) != m_archivedObjects.end())
        return true;

    return false;
}

bool DeepArchiveSaver::isArraySaved(const std::string &name) const
{
    if (m_arrays.find(name) != m_arrays.end())
        return true;
    if (m_archivedArrays.find(name) != m_archivedArrays.end())
        return true;

    return false;
}

std::set<std::string> DeepArchiveSaver::savedObjects() const
{
    auto objs = m_archivedObjects;
    for (const auto &o: m_objects)
        objs.emplace(o.first);
    return objs;
}

std::set<std::string> DeepArchiveSaver::savedArrays() const
{
    auto arrs = m_archivedArrays;
    for (const auto &a: m_arrays)
        arrs.emplace(a.first);
    return arrs;
}

void DeepArchiveSaver::setSavedObjects(const std::set<std::string> &objs)
{
    m_archivedObjects = objs;
}

void DeepArchiveSaver::setSavedArrays(const std::set<std::string> &arrs)
{
    m_archivedArrays = arrs;
}

void DeepArchiveSaver::setCompressionSettings(const CompressionSettings &settings)
{
    m_compressionSettings = settings;
}

} // namespace vistle
