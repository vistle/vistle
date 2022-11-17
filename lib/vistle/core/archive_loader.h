#ifndef VISTLE_ARCHIVE_LOADER_H
#define VISTLE_ARCHIVE_LOADER_H

#include "export.h"
#include "archives.h"
#include "shmvector.h"

#include <vistle/util/buffer.h>

#include <cassert>
#include <set>
#include <map>
#include <string>
#include <memory>
#include <ostream>

namespace vistle {

struct V_COREEXPORT ArrayLoader {
    struct ArrayOwner {
        virtual ~ArrayOwner() {}
    };

    template<typename T>
    struct Unreffer: public ArrayOwner {
        explicit Unreffer(ShmVector<T> &ref): m_ref(ref) {}
        ShmVector<T> m_ref;
    };

    ArrayLoader(const std::string &name, int type, const vistle::iarchive &ar);
    ArrayLoader() = delete;
    ArrayLoader(const ArrayLoader &other) = delete;

    std::shared_ptr<ArrayOwner> m_unreffer;

    template<typename T>
    void operator()(T)
    {
        if (shm_array<T, typename shm<T>::allocator>::typeId() == m_type) {
            if (m_ok) {
                m_ok = false;
                std::cerr << "ArrayLoader: multiple type matches for data array " << m_name << std::endl;
                return;
            }
            ShmVector<T> arr;
            if (!m_name.empty())
                arr = Shm::the().getArrayFromName<T>(m_name);
            if (arr) {
                std::cerr << "ArrayLoader: already have data array with name " << m_name << std::endl;
                m_unreffer.reset(new Unreffer<T>(arr));
                return;
            }
            auto &ar = const_cast<vistle::iarchive &>(m_ar);
            std::string arname;
            ar &arname;
            assert(arname == m_arname);
            m_name = ar.translateArrayName(arname);
            arr = ShmVector<T>((shm_name_t)m_name);
            if (!arr.valid())
                arr.construct();
            //std::cerr << "ArrayLoader: loading " << arname << " as " << m_name << ": arr=" << arr << std::endl;
            m_name = arr.name().str();
            ar.registerArrayNameTranslation(arname, arr.name());
            //std::cerr << "ArrayLoader: constructed " << arname << " as " << arr.name() << std::endl;
            ar &*arr;
            m_unreffer.reset(new Unreffer<T>(arr));
            m_ok = true;
        }
    }

    bool load();
    const std::string &name() const;
    std::shared_ptr<ArrayOwner> owner() const;

    bool m_ok;
    std::string m_arname; //<! name in archive
    std::string m_name; //<! name in shmem
    unsigned m_type;
    const vistle::iarchive &m_ar;
};

class DeepArchiveFetcher;

V_COREEXPORT std::ostream &operator<<(std::ostream &os, const DeepArchiveFetcher &daf);

class V_COREEXPORT DeepArchiveFetcher: public Fetcher, public std::enable_shared_from_this<DeepArchiveFetcher> {
    friend std::ostream &operator<<(std::ostream &os, const DeepArchiveFetcher &daf);
public:
    DeepArchiveFetcher(const std::map<std::string, buffer> &objects, const std::map<std::string, buffer> &arrays,
                       const std::map<std::string, message::CompressionMode> &compressions,
                       const std::map<std::string, size_t> &sizes);

    void requestArray(const std::string &name, int type, const ArrayCompletionHandler &completeCallback) override;
    void requestObject(const std::string &name, const ObjectCompletionHandler &completeCallback) override;

    bool renameObjects() const override;
    std::string translateObjectName(const std::string &name) const override;
    std::string translateArrayName(const std::string &name) const override;
    void registerObjectNameTranslation(const std::string &arname, const std::string &name) override;
    void registerArrayNameTranslation(const std::string &arname, const std::string &name) override;

    void setRenameObjects(bool rename);
    std::map<std::string, std::string> objectTranslations() const;
    std::map<std::string, std::string> arrayTranslations() const;
    void setObjectTranslations(const std::map<std::string, std::string> &objs);
    void setArrayTranslations(const std::map<std::string, std::string> &arrs);

    void releaseArrays();

private:
    bool m_rename = false;
    std::map<std::string, std::string> m_transObject, m_transArray;

    const std::map<std::string, buffer> &m_objects;
    const std::map<std::string, buffer> &m_arrays;
    const std::map<std::string, message::CompressionMode> &m_compression;
    const std::map<std::string, size_t> &m_rawSize;

    std::set<std::shared_ptr<ArrayLoader::ArrayOwner>> m_ownedArrays;
};

} // namespace vistle
#endif
