#ifndef VISTLE_ARCHIVE_LOADER_H
#define VISTLE_ARCHIVE_LOADER_H

#include "archives.h"
#include "assert.h"
#include "shmvector.h"

#include <boost/mpl/for_each.hpp>

namespace vistle {

struct V_COREEXPORT ArrayLoader {

    struct BaseUnreffer {
        virtual ~BaseUnreffer() {}
    };

    template<typename T>
    struct Unreffer: public BaseUnreffer {
        Unreffer(ShmVector<T> &ref): m_ref(ref) {}
        ShmVector<T> m_ref;
    };

    ArrayLoader(const std::string &name, int type, const vistle::iarchive &ar): m_ok(false), m_name(name), m_type(type), m_ar(ar) {}
    ArrayLoader() = delete;
    ArrayLoader(const ArrayLoader &other) = delete;

    std::shared_ptr<BaseUnreffer> m_unreffer;

    template<typename T>
    void operator()(T) {
        if (shm_array<T, typename shm<T>::allocator>::typeId() == m_type) {
            if (m_ok) {
                m_ok = false;
                std::cerr << "ArrayLoader: type matches for data array " << m_name << std::endl;
                return;
            }
            auto arr = Shm::the().getArrayFromName<T>(m_name);
            if (arr) {
                std::cerr << "ArrayLoader: have data array with name " << m_name << std::endl;
                m_unreffer.reset(new Unreffer<T>(arr));
                return;
            }
            auto &ar = const_cast<vistle::iarchive &>(m_ar);
            std::string name;
            ar & name;
            vassert(name == m_name);
            arr = ShmVector<T>((shm_name_t)m_name);
            arr.construct();
            ar & *arr;
            m_unreffer.reset(new Unreffer<T>(arr));
            m_ok = true;
        }
    }

    bool load() {
       boost::mpl::for_each<VectorTypes>(boost::reference_wrapper<ArrayLoader>(*this));
       if (!m_ok) {
           std::cerr << "ArrayLoader: failed to restore array " << m_name << " from archive" << std::endl;
       }
       return m_ok;
    }

    bool m_ok;
    std::string m_name;
    int m_type;
    const vistle::iarchive &m_ar;
};

class V_COREEXPORT DeepArchiveFetcher: public Fetcher, public std::enable_shared_from_this<DeepArchiveFetcher> {
public:
    DeepArchiveFetcher(std::map<std::string, std::vector<char>> &objects, std::map<std::string, std::vector<char>> &arrays);

    void requestArray(const std::string &name, int type, const std::function<void()> &completeCallback) override;
    void requestObject(const std::string &name, const std::function<void()> &completeCallback) override;

private:
    std::map<std::string,std::vector<char>> m_objects;
    std::map<std::string,std::vector<char>> m_arrays;
};



}
#endif
