#ifndef VISTLE_CORE_ARCHIVE_SAVER_H
#define VISTLE_CORE_ARCHIVE_SAVER_H

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
    void operator()(T);

    bool save();

    bool m_ok;
    std::string m_name;
    unsigned m_type;
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
