#ifndef VISTLE_ARCHIVE_SAVER_H
#define VISTLE_ARCHIVE_SAVER_H

#include "archives.h"
#include "shmvector.h"
#include <vistle/util/buffer.h>

#include <boost/mpl/for_each.hpp>

#include <set>
#include <map>
#include <string>
#include <memory>
#include <iostream>

namespace vistle {

struct V_COREEXPORT ArraySaver {
    ArraySaver(const std::string &name, int type, vistle::oarchive &ar, const void *array = nullptr)
    : m_ok(false), m_name(name), m_type(type), m_ar(ar), m_array(array)
    {}
    ArraySaver() = delete;
    ArraySaver(const ArraySaver &other) = delete;

    template<typename T>
    void operator()(T)
    {
        if (shm_array<T, typename shm<T>::allocator>::typeId() != m_type) {
            //std::cerr << "ArraySaver: type mismatch - looking for " << m_type << ", is " << shm_array<T, typename shm<T>::allocator>::typeId() << std::endl;
            return;
        }

        if (m_ok) {
            m_ok = false;
            std::cerr << "ArraySaver: multiple type matches for data array " << m_name << std::endl;
            return;
        }
        ShmVector<T> arr;
        if (m_array) {
            arr = *reinterpret_cast<const ShmVector<T> *>(m_array);
        } else {
            arr = Shm::the().getArrayFromName<T>(m_name);
        }
        if (!arr) {
            std::cerr << "ArraySaver: did not find data array " << m_name << std::endl;
            return;
        }
        m_ar &m_name;
        m_ar &*arr;
        m_ok = true;
    }

    bool save()
    {
        boost::mpl::for_each<VectorTypes>(boost::reference_wrapper<ArraySaver>(*this));
        if (!m_ok) {
            std::cerr << "ArraySaver: failed to save array " << m_name << " to archive" << std::endl;
        }
        return m_ok;
    }

    bool m_ok;
    std::string m_name;
    int m_type;
    vistle::oarchive &m_ar;
    const void *m_array = nullptr;
};

class V_COREEXPORT DeepArchiveSaver: public Saver, public std::enable_shared_from_this<DeepArchiveSaver> {
public:
    void saveArray(const std::string &name, int type, const void *array) override;
    void saveObject(const std::string &name, obj_const_ptr obj) override;
    SubArchiveDirectory getDirectory();
    void flushDirectory();

    bool isObjectSaved(const std::string &name) const;
    bool isArraySaved(const std::string &name) const;

    std::set<std::string> savedObjects() const;
    std::set<std::string> savedArrays() const;
    void setSavedObjects(const std::set<std::string> &objs);
    void setSavedArrays(const std::set<std::string> &arrs);

    void setCompressionSettings(const CompressionSettings &settings);

private:
    CompressionSettings m_compressionSettings;
    std::map<std::string, buffer> m_objects;
    std::map<std::string, buffer> m_arrays;
    std::set<std::string> m_archivedObjects;
    std::set<std::string> m_archivedArrays;
};


} // namespace vistle
#endif
