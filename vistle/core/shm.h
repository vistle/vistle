#ifndef SHM_H
#define SHM_H

#include <memory>
#include <array>
#include <mutex>
#include <atomic>
#include <map>

#ifdef NO_SHMEM
#include <memory>
#include <string>
#include <vector>
#include <map>
#else
#include <vistle/util/boost_interprocess_config.h>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/map.hpp>
#ifdef _WIN32
#include <boost/interprocess/managed_windows_shared_memory.hpp>
#else
#include <boost/interprocess/managed_shared_memory.hpp>
#include <pthread.h>
#endif
#endif

#include "archives_config.h"

#include <vistle/util/exception.h>
#include <vistle/util/allocator.h>

#include "export.h"
#include "index.h"
//#include "shm_array.h"
#include "shmname.h"
#include "shmdata.h"

#if defined(BOOST_INTERPROCESS_POSIX_BARRIERS) && defined(BOOST_INTERPROCESS_POSIX_PROCESS_SHARED)
#define SHMBARRIER
#endif

//#define SHMDEBUG
//#define SHMPUBLISH

namespace vistle {

#ifdef NO_SHMEM

typedef ShmData *shm_handle_t;

#else

#ifdef _WIN32
typedef boost::interprocess::managed_windows_shared_memory managed_shm;
#else
typedef boost::interprocess::managed_shared_memory managed_shm;
#endif

typedef managed_shm::handle_t shm_handle_t;
#endif

namespace message {
class MessageQueue;
}

class V_COREEXPORT shm_exception: public exception {
public:
    shm_exception(const std::string &what = "shared memory error"): exception(what) {}
};

class Object;
struct ObjectData;
template<class T, class allocator>
class shm_array;

#ifdef SHMDEBUG
struct ShmDebugInfo {
    shm_name_t name;
    shm_handle_t handle;
    char deleted;
    char type;

    ShmDebugInfo(char type = '\0', const std::string &name = "", shm_handle_t handle = 0)
    : handle(handle), deleted(0), type(type)
    {
        memset(this->name, '\0', sizeof(this->name));
        strncpy(this->name, name.c_str(), sizeof(this->name) - 1);
    }
};
#endif

template<typename T>
struct shm {
#ifdef NO_SHMEM
    typedef vistle::default_init_allocator<T> allocator;
    typedef std::basic_string<T> string;
    typedef std::vector<T> vector;
    typedef vistle::shm_array<T, allocator> array;
    typedef array *array_ptr;
    struct Constructor {
        std::string name;

        Constructor(const std::string &name);
        ~Constructor();
        template<typename... Args>
        T *operator()(Args &&...args);
    };
    static Constructor construct(const std::string &name) { return Constructor(name); }
#else
    typedef boost::interprocess::allocator<T, managed_shm::segment_manager> allocator;
    typedef boost::interprocess::basic_string<T, std::char_traits<T>, allocator> string;
    typedef boost::interprocess::vector<T, allocator> vector;
    typedef vistle::shm_array<T, allocator> array;
    typedef boost::interprocess::offset_ptr<array> array_ptr;
    static typename managed_shm::segment_manager::template construct_proxy<T>::type construct(const std::string &name);
#endif
    static T *find(const std::string &name);
    static bool destroy(const std::string &name);
    static T *find_and_ref(const std::string &name); // as find, but reference array
    static bool destroy_array(const std::string &name, array_ptr arr);
};

template<class T>
class shm_array_ref;
template<class T>
using ShmVector = shm_array_ref<shm_array<T, typename shm<T>::allocator>>;

class V_COREEXPORT Shm {
    template<typename T>
    friend struct shm;
    friend class Communicator;

public:
    static std::string instanceName(const std::string &host, unsigned short port);
    static Shm &the();
    static bool perRank();
    static bool remove(const std::string &shmname, int moduleID, int rank, bool perRank);
    static Shm &create(const std::string &shmname, int moduleID, int rank, bool perRank);
    static Shm &attach(const std::string &shmname, int moduleID, int rank, bool perRank);
    static bool isAttached();

    void detach();
    void setRemoveOnDetach();

    std::string name() const;
    const std::string &instanceName() const;
    int owningRank() const;

#ifdef NO_SHMEM
    typedef vistle::default_init_allocator<void> void_allocator;
#else
    typedef boost::interprocess::allocator<void, managed_shm::segment_manager> void_allocator;
    managed_shm &shm();
    const managed_shm &shm() const;
#endif
    const void_allocator &allocator() const;

    std::string createArrayId(const std::string &name = "");
    std::string createObjectId(const std::string &name = "");

    void lockObjects() const;
    void unlockObjects() const;
    void lockDictionary() const;
    void unlockDictionary() const;

    int objectID() const;
    int arrayID() const;
    void setObjectID(int id);
    void setArrayID(int id);

    std::shared_ptr<const Object> getObjectFromHandle(const shm_handle_t &handle) const;
    shm_handle_t getHandleFromObject(std::shared_ptr<const Object> object) const;
    shm_handle_t getHandleFromObject(const Object *object) const;
    shm_handle_t getHandleFromArray(const ShmData *array) const;
    ObjectData *getObjectDataFromName(const std::string &name) const;
    ObjectData *getObjectDataFromHandle(const shm_handle_t &handle) const;
    std::shared_ptr<const Object> getObjectFromName(const std::string &name, bool onlyComplete = true) const;
    template<typename T>
    const ShmVector<T> getArrayFromName(const std::string &name) const;

    static std::string shmIdFilename();
    static bool cleanAll(int rank);

    void markAsRemoved(const std::string &name);
    void addObject(const std::string &name, const shm_handle_t &handle);
    void addArray(const std::string &name, const ShmData *array);
#ifdef SHMDEBUG
#ifdef NO_SHMEM
    static std::vector<ShmDebugInfo, vistle::shm<ShmDebugInfo>::allocator> *s_shmdebug;
    static std::recursive_mutex *s_shmdebugMutex;
#else
    static boost::interprocess::vector<ShmDebugInfo, vistle::shm<ShmDebugInfo>::allocator> *s_shmdebug;
    static boost::interprocess::interprocess_recursive_mutex *s_shmdebugMutex;
#endif
#endif

#ifdef SHMBARRIER
    pthread_barrier_t *newBarrier(const std::string &name, int count);
    void deleteBarrier(const std::string &name);
#endif

private:
    // create on size>0, otherwise attach
    Shm(const std::string &name, const int moduleID, const int rank, size_t size = 0);
    ~Shm();

    void setId(int id);

    void_allocator *m_allocator;
    std::string m_name;
    bool m_remove;
    int m_id;
    const int m_rank;
    int m_owningRank = -1;
    std::atomic<int> m_objectId, m_arrayId;
    static Shm *s_singleton;
#ifdef NO_SHMEM
    mutable std::recursive_mutex *m_shmDeletionMutex;
    mutable std::recursive_mutex *m_objectDictionaryMutex;
    std::map<std::string, shm_handle_t> m_objectDictionary;
#else
    static bool s_perRank;
    mutable boost::interprocess::interprocess_recursive_mutex *m_shmDeletionMutex;
    mutable boost::interprocess::interprocess_recursive_mutex *m_objectDictionaryMutex;
    managed_shm *m_shm;
#endif
    mutable std::atomic<int> m_lockCount;
#ifdef SHMBARRIER
#ifndef NO_SHMEM
    std::map<std::string, boost::interprocess::ipcdetail::barrier_initializer> m_barriers;
#endif
#endif
};

template<typename T>
T *shm<T>::find(const std::string &name)
{
#ifdef NO_SHMEM
    Shm::the().lockDictionary();
    auto &dict = Shm::the().m_objectDictionary;
    auto it = dict.find(name);
    if (it == dict.end()) {
        Shm::the().unlockDictionary();
        return nullptr;
    }
    Shm::the().unlockDictionary();
    return static_cast<T *>(it->second);
#else
    return Shm::the().shm().find<T>(name.c_str()).first;
#endif
}

template<typename T>
T *shm<T>::find_and_ref(const std::string &name)
{
#ifdef NO_SHMEM
    Shm::the().lockDictionary();
    T *t = shm<T>::find(name);
    if (t)
        t->ref();
    Shm::the().unlockDictionary();
    return t;
#else
    Shm::the().lockObjects();
    T *t = shm<T>::find(name);
    if (t) {
        t->ref();
        assert(t->refcount() > 0);
    }
    Shm::the().unlockObjects();
    return t;
#endif
}

template<typename T>
bool shm<T>::destroy(const std::string &name)
{
#ifdef NO_SHMEM
    Shm::the().lockDictionary();
    auto &dict = Shm::the().m_objectDictionary;
    auto it = dict.find(name);
    bool ret = true;
    if (it == dict.end()) {
        Shm::the().unlockDictionary();
        std::cerr << "WARNING: shm: did not find object " << name << " to be deleted" << std::endl;
        ret = false;
    } else {
        T *t = static_cast<T *>(it->second);
        dict.erase(it);
        Shm::the().unlockDictionary();
        delete t;
    }
#else
    const bool ret = Shm::the().shm().destroy<T>(name.c_str());
#endif
    Shm::the().markAsRemoved(name);
    return ret;
}

template<typename T>
bool shm<T>::destroy_array(const std::string &name, shm<T>::array_ptr arr)
{
#ifdef NO_SHMEM
    Shm::the().lockDictionary();
    if (arr->refcount() > 0) {
        Shm::the().unlockDictionary();
        return true;
    }
    bool ret = shm<shm<T>::array>::destroy(name);
    Shm::the().unlockDictionary();
    return ret;
#else
    Shm::the().lockObjects();
    if (arr->refcount() > 0) {
        Shm::the().unlockObjects();
        return true;
    }
    assert(arr->refcount() == 0);
    bool ret = shm<shm<T>::array>::destroy(name);
    Shm::the().unlockObjects();
    return ret;
#endif
}

#ifdef NO_SHMEM
template<typename T>
shm<T>::Constructor::Constructor(const std::string &name): name(name)
{
    Shm::the().lockDictionary();
}

template<typename T>
shm<T>::Constructor::~Constructor()
{
    Shm::the().unlockDictionary();
}

template<typename T>
template<typename... Args>
T *shm<T>::Constructor::operator()(Args &&...args)
{
    auto &dict = Shm::the().m_objectDictionary;
    auto it = dict.find(name);
    if (it != dict.end()) {
        std::cerr << "WARNING: shm: already have " << name << std::endl;
        return reinterpret_cast<T *>(it->second);
    }

    T *t = new T(std::forward<Args>(args)...);
    dict[name] = t;
    return t;
}
#endif

} // namespace vistle

#endif

#include "shm_reference.h"

namespace vistle {
//using ShmVector = shm_array_ref<shm_array<T, typename shm<T>::allocator>>;


}

#include "shm_impl.h"
