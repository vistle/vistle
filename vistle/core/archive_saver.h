#ifndef VISTLE_ARCHIVE_SAVER_H
#define VISTLE_ARCHIVE_SAVER_H

#include "archives.h"
#include "assert.h"

#include <boost/mpl/for_each.hpp>

namespace vistle {

struct V_COREEXPORT ArraySaver {

    ArraySaver(const std::string &name, int type, vistle::oarchive &ar, const void *array=nullptr): m_ok(false), m_name(name), m_type(type), m_ar(ar), m_array(array) {}
    ArraySaver() = delete;
    ArraySaver(const ArraySaver &other) = delete;

    template<typename T>
    void operator()(T) {
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
        m_ar & m_name;
        m_ar & *arr;
        m_ok = true;
    }

    bool save() {
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
    void saveArray(const std::string &name, int type, const void *array) override {
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

    void saveObject(const std::string &name, Object::const_ptr obj) override {
        if (m_objects.find(name) != m_objects.end())
            return;

        vecostreambuf<char> vb;
        oarchive ar(vb);
        ar.setSaver(shared_from_this());
        obj->save(ar);
        m_objects.emplace(name, vb.get_vector());
    }

    SubArchiveDirectory getDirectory() {
        SubArchiveDirectory dir;
        for (auto &obj: m_objects) {
            dir.emplace_back(obj.first, false, obj.second.size(), obj.second.data());
        }
        for (auto &arr: m_arrays) {
            dir.emplace_back(arr.first, true, arr.second.size(), arr.second.data());
        }
        return dir;
    }

private:
    std::map<std::string,std::vector<char>> m_objects;
    std::map<std::string,std::vector<char>> m_arrays;
};



}
#endif
