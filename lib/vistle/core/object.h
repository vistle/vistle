#ifndef OBJECT_H
#define OBJECT_H

#include <vector>
#include <memory>
#include <string>
#include <atomic>
#include <functional>
#include <ostream>

#ifdef NO_SHMEM
#include <map>
#else
#include "shm.h"
#endif
#include <vistle/util/boost_interprocess_config.h>
#include <boost/interprocess/exceptions.hpp>

#include <boost/mpl/size.hpp>

#include <vistle/util/sysdep.h>
#include <vistle/util/enum.h>

#include "export.h"
#include "shmname.h"
#include "shm_array.h"
#include "objectmeta.h"
#include "scalars.h"
#include "dimensions.h"
#include "vectortypes.h"

#include "archives_config.h"


namespace vistle {

namespace interprocess = boost::interprocess;

#ifdef NO_SHMEM
#else
typedef managed_shm::handle_t shm_handle_t;
#endif

class Shm;

struct ObjectData;
class Object;


class V_COREEXPORT ObjectInterfaceBase {
public:
    //std::shared_ptr<const Object> object() const { static_cast<const Object *>(this)->shared_from_this(); }
    virtual std::shared_ptr<const Object> object() const = 0;

protected:
    virtual ~ObjectInterfaceBase() {}
};

class V_COREEXPORT Object: public std::enable_shared_from_this<Object>, virtual public ObjectInterfaceBase {
    friend class Shm;
    friend class ObjectTypeRegistry;
    friend struct ObjectData;
    template<class ObjType>
    friend class shm_obj_ref;
    friend void Shm::markAsRemoved(const std::string &name);
    friend class Module;

public:
    typedef std::shared_ptr<Object> ptr;
    typedef std::shared_ptr<const Object> const_ptr;
    typedef ObjectData Data;

    template<class Interface>
    const Interface *getInterface() const
    {
        return dynamic_cast<const Interface *>(this);
    }

    std::shared_ptr<const Object> object() const override
    {
        return static_cast<const Object *>(this)->shared_from_this();
    }

    enum InitializedFlags { Initialized };

    enum Type {
        COORD = -9,
        DATABASE = -7,

        UNKNOWN = -1,

        EMPTY = 1,
        PLACEHOLDER = 11,

        TEXTURE1D = 16,

        POINTS = 18,
        LINES = 20,
        TRIANGLES = 22,
        POLYGONS = 23,
        UNSTRUCTUREDGRID = 24,
        UNIFORMGRID = 25,
        RECTILINEARGRID = 26,
        STRUCTUREDGRID = 27,
        QUADS = 28,
        LAYERGRID = 29,

        VERTEXOWNERLIST = 95,
        CELLTREE1 = 96,
        CELLTREE2 = 97,
        CELLTREE3 = 98,
        NORMALS = 99,

        VEC = 100, // base type id for all Vec types
    };

    static const char *toString(Type v);

    virtual ~Object();

    bool operator==(const Object &other) const;
    bool operator!=(const Object &other) const;

    bool isComplete() const; //! check whether all references have been resolved

    virtual std::set<Object::const_ptr> referencedObjects() const;

    static const char *typeName() { return "Object"; }
    Object::ptr clone() const;
    virtual Object::ptr cloneInternal() const = 0;

    Object::ptr cloneType() const;
    virtual Object::ptr cloneTypeInternal() const = 0;

    static Object *createEmpty(const std::string &name = std::string());

    virtual void refresh() const; //!< refresh cached pointers from shm
    virtual bool check(std::ostream &os, bool quick = true) const;
    virtual void print(std::ostream &os, bool verbose = false) const;
    virtual void updateInternals();

    virtual bool isEmpty() const;
    virtual bool isEmpty();

    static std::shared_ptr<Object> as(std::shared_ptr<Object> ptr) { return ptr; }
    static std::shared_ptr<const Object> as(std::shared_ptr<const Object> ptr) { return ptr; }

    shm_handle_t getHandle() const;

    Type getType() const;
    std::string getName() const;

    int getBlock() const;
    int getNumBlocks() const;
    double getRealTime() const;
    int getTimestep() const;
    int getNumTimesteps() const;
    int getIteration() const;
    int getGeneration() const;
    int getCreator() const;
    Matrix4 getTransform() const;

    void setBlock(const int block);
    void setNumBlocks(const int num);
    void setRealTime(double time);
    void setTimestep(const int timestep);
    void setNumTimesteps(const int num);
    void setIteration(const int num);
    void setGeneration(const int count);
    void setCreator(const int id);
    void setTransform(const Matrix4 &transform);

    const Meta &meta() const;
    void setMeta(const Meta &meta);

    void addAttribute(const std::string &key, const std::string &value = "");
    void setAttributeList(const std::string &key, const std::vector<std::string> &values);
    virtual void copyAttributes(Object::const_ptr src, bool replace = true);
    bool hasAttribute(const std::string &key) const;
    std::string getAttribute(const std::string &key) const;
    std::vector<std::string> getAttributes(const std::string &key) const;
    std::vector<std::string> getAttributeList() const;

    // attachments, e.g. Celltrees
    bool addAttachment(const std::string &key, Object::const_ptr att) const;
    bool hasAttachment(const std::string &key) const;
    void copyAttachments(Object::const_ptr src, bool replace = true);
    Object::const_ptr getAttachment(const std::string &key) const;
    bool removeAttachment(const std::string &key) const;

    int ref() const;
    int unref() const;
    int refcount() const;

    template<class Archive>
    static Object *loadObject(Archive &ar);

    template<class Archive>
    void saveObject(Archive &ar) const;

    ARCHIVE_REGISTRATION(= 0)

#ifdef USE_INTROSPECTION_ARCHIVE
    virtual void save(FindObjectReferenceOArchive &ar) const = 0;
#endif

public:
protected:
    Data *m_data;

public:
    Data *d() const
    {
        return m_data;
    }

protected:
    Object(Data *data);
    Object();

    static void publish(const Data *d);
    static Object *create(Data *);

private:
    std::string m_name; // just a debugging aid
    template<class Archive>
    void serialize(Archive &ar);
    ARCHIVE_ACCESS

    // not implemented
    Object(const Object &) = delete;
    Object &operator=(const Object &) = delete;

#ifdef NO_SHMEM
    std::recursive_mutex &attachmentMutex() const;
#else
    boost::interprocess::interprocess_recursive_mutex &attachmentMutex() const;
#endif
};
V_COREEXPORT std::ostream &operator<<(std::ostream &os, const Object &);

ARCHIVE_ASSUME_ABSTRACT(Object)

struct ObjectData: public ShmData {
    Object::Type type;

    std::atomic<int> unresolvedReferences; //!< no. of not-yet-available arrays and referenced objects

    Meta meta;

    typedef shm<char>::string Attribute;
    typedef shm<char>::string Key;
    typedef shm<Attribute>::vector AttributeList;
    typedef std::pair<const Key, AttributeList> AttributeMapValueType;
    typedef shm<AttributeMapValueType>::allocator AttributeMapAllocator;
#ifdef NO_SHMEM
    typedef std::map<Key, AttributeList, std::less<Key>, AttributeMapAllocator> AttributeMap;
#else
    typedef interprocess::map<Key, AttributeList, std::less<Key>, AttributeMapAllocator> AttributeMap;
#endif
    AttributeMap attributes;
    typedef std::map<std::string, std::vector<std::string>> StdAttributeMap; // for serialization
    void addAttribute(const std::string &key, const std::string &value = "");
    V_COREEXPORT void setAttributeList(const std::string &key, const std::vector<std::string> &values);
    void copyAttributes(const ObjectData *src, bool replace);
    bool hasAttribute(const std::string &key) const;
    std::string getAttribute(const std::string &key) const;
    V_COREEXPORT std::vector<std::string> getAttributes(const std::string &key) const;
    V_COREEXPORT std::vector<std::string> getAttributeList() const;

#ifdef NO_SHMEM
    mutable std::recursive_mutex attachment_mutex;
    typedef std::lock_guard<std::recursive_mutex> mutex_lock_type;
    typedef const ObjectData *Attachment;
#else
    mutable boost::interprocess::interprocess_recursive_mutex attachment_mutex; //< protects attachments
    typedef boost::interprocess::scoped_lock<boost::interprocess::interprocess_recursive_mutex> mutex_lock_type;
    typedef interprocess::offset_ptr<const ObjectData> Attachment;
#endif
    typedef std::pair<const Key, Attachment> AttachmentMapValueType;
    typedef shm<AttachmentMapValueType>::allocator AttachmentMapAllocator;
#ifdef NO_SHMEM
    typedef std::map<Key, Attachment, std::less<Key>, AttachmentMapAllocator> AttachmentMap;
#else
    typedef interprocess::map<Key, Attachment, std::less<Key>, AttachmentMapAllocator> AttachmentMap;
#endif
    AttachmentMap attachments;
    bool addAttachment(const std::string &key, Object::const_ptr att);
    void copyAttachments(const ObjectData *src, bool replace);
    bool hasAttachment(const std::string &key) const;
    Object::const_ptr getAttachment(const std::string &key) const;
    bool removeAttachment(const std::string &key);

    V_COREEXPORT ObjectData(Object::Type id = Object::UNKNOWN, const std::string &name = "", const Meta &m = Meta());
    V_COREEXPORT ObjectData(const ObjectData &other, const std::string &name,
                            Object::Type id = Object::UNKNOWN); //! shallow copy, except for attributes
    V_COREEXPORT ~ObjectData();
#ifndef NO_SHMEM
    V_COREEXPORT void *operator new(size_t size);
    V_COREEXPORT void *operator new(std::size_t size, void *ptr);
    V_COREEXPORT void operator delete(void *ptr);
    V_COREEXPORT void operator delete(void *ptr, void *voidptr2);
#endif
    V_COREEXPORT int ref() const;
    V_COREEXPORT int unref() const;
    static ObjectData *create(Object::Type id, const std::string &name, const Meta &m);
    V_COREEXPORT bool isComplete() const; //! check whether all references have been resolved
    V_COREEXPORT void unresolvedReference();
    V_COREEXPORT void referenceResolved(const std::function<void()> &completeCallback);

    ARCHIVE_ACCESS_SPLIT
    template<class Archive>
    void save(Archive &ar) const;
    template<class Archive>
    void load(Archive &ar);

    // not implemented
    ObjectData(const ObjectData &) = delete;
    ObjectData &operator=(const ObjectData &) = delete;
};

#ifdef USE_BOOST_ARCHIVE
extern template Object V_COREEXPORT *V_COREEXPORT::loadObject<boost_iarchive>(boost_iarchive &ar);
extern template void V_COREEXPORT Object::saveObject<boost_oarchive>(boost_oarchive &ar) const;
extern template void V_COREEXPORT Object::serialize<boost_iarchive>(boost_iarchive &ar);
extern template void V_COREEXPORT Object::serialize<boost_oarchive>(boost_oarchive &ar);
extern template void V_COREEXPORT Object::Data::load<boost_iarchive>(boost_iarchive &ar);
extern template void V_COREEXPORT Object::Data::save<boost_oarchive>(boost_oarchive &ar) const;
#endif
#ifdef USE_YAS
extern template Object V_COREEXPORT *Object::loadObject<yas_iarchive>(yas_iarchive &ar);
extern template void V_COREEXPORT Object::saveObject<yas_oarchive>(yas_oarchive &ar) const;
extern template void V_COREEXPORT Object::serialize<yas_iarchive>(yas_iarchive &ar);
extern template void V_COREEXPORT Object::serialize<yas_oarchive>(yas_oarchive &ar);
extern template void V_COREEXPORT Object::Data::load<yas_iarchive>(yas_iarchive &ar);
extern template void V_COREEXPORT Object::Data::save<yas_oarchive>(yas_oarchive &ar) const;
#endif

typedef std::function<void(const std::string &name)> ArrayCompletionHandler;
typedef std::function<void(Object::const_ptr)> ObjectCompletionHandler;

class V_COREEXPORT ObjectTypeRegistry {
public:
    typedef Object *(*CreateEmptyFunc)(const std::string &name);
    typedef Object *(*CreateFunc)(Object::Data *d);
    typedef void (*DestroyFunc)(const std::string &name);

    struct FunctionTable {
        CreateEmptyFunc createEmpty;
        CreateFunc create;
        DestroyFunc destroy;
    };

    template<class O>
    static void registerType(int id)
    {
        assert(typeMap().find(id) == typeMap().end());
        struct FunctionTable t = {
            O::createEmpty,
            O::createFromData,
            O::destroy,
        };
        typeMap()[id] = t;
    }

    static const struct FunctionTable &getType(int id);

    static CreateFunc getCreator(int id);
    static DestroyFunc getDestroyer(int id);

private:
    typedef std::map<int, FunctionTable> TypeMap;

    static TypeMap &typeMap();
};

//! declare a new Object type
#define V_OBJECT(ObjType) \
public: \
    typedef std::shared_ptr<ObjType> ptr; \
    typedef std::shared_ptr<const ObjType> const_ptr; \
    typedef ObjType Class; \
    static Object::Type type(); \
    static const char *typeName() \
    { \
        return #ObjType; \
    } \
    template<typename ObjType2> \
    static std::shared_ptr<const ObjType> as(std::shared_ptr<const ObjType2> ptr) \
    { \
        return std::dynamic_pointer_cast<const ObjType>(ptr); \
    } \
    template<typename ObjType2> \
    static std::shared_ptr<ObjType> as(std::shared_ptr<ObjType2> ptr) \
    { \
        return std::dynamic_pointer_cast<ObjType>(ptr); \
    } \
    static Object *createFromData(Object::Data *data) \
    { \
        return new ObjType(static_cast<ObjType::Data *>(data)); \
    } \
    std::shared_ptr<const Object> object() const override \
    { \
        return static_cast<const Object *>(this)->shared_from_this(); \
    } \
    Object::ptr cloneInternal() const override \
    { \
        const std::string n(Shm::the().createObjectId()); \
        Data *data = shm<Data>::construct(n)(*d(), n); \
        publish(data); \
        return Object::ptr(createFromData(data)); \
    } \
    ptr clone() const \
    { \
        return ObjType::as(cloneInternal()); \
    } \
    ptr cloneType() const \
    { \
        return ObjType::as(cloneTypeInternal()); \
    } \
    Object::ptr cloneTypeInternal() const override \
    { \
        return Object::ptr(new ObjType(Object::Initialized)); \
    } \
    static Object *createEmpty(const std::string &name) \
    { \
        if (name.empty()) \
            return new ObjType(Object::Initialized); \
        auto d = Data::createNamed(ObjType::type(), name); \
        return new ObjType(d); \
    } \
    template<class OtherType> \
    static ptr clone(typename OtherType::ptr other) \
    { \
        const std::string n(Shm::the().createObjectId()); \
        typename ObjType::Data *data = shm<typename ObjType::Data>::construct(n)(*other->d(), n); \
        assert(data->type == ObjType::type()); \
        ObjType *ret = dynamic_cast<ObjType *>(createFromData(data)); \
        assert(ret); \
        publish(data); \
        return ptr(ret); \
    } \
    template<class OtherType> \
    static ptr clone(typename OtherType::const_ptr other) \
    { \
        return ObjType::clone<OtherType>(std::const_pointer_cast<OtherType>(other)); \
    } \
    static void destroy(const std::string &name) \
    { \
        shm<ObjType::Data>::destroy(name); \
    } \
    void refresh() const override \
    { \
        Base::refresh(); \
        refreshImpl(); \
    } \
    void refreshImpl() const; \
    explicit ObjType(Object::InitializedFlags): Base(ObjType::Data::create()) \
    { \
        refreshImpl(); \
    } \
    virtual bool isEmpty() override; \
    virtual bool isEmpty() const override; \
    bool check(std::ostream &os, bool quick = true) const override \
    { \
        refresh(); \
        if (isEmpty()) { \
        }; \
        if (!Base::check(os, quick)) { \
            std::cerr << *this << std::endl; \
            return false; \
        } \
        if (!checkImpl(os, quick)) { \
            std::cerr << *this << std::endl; \
            return false; \
        } \
        return true; \
    } \
    void print(std::ostream &os, bool verbose = false) const override; \
    struct Data; \
    const Data *d() const \
    { \
        return static_cast<Data *>(Object::m_data); \
    } \
    Data *d() \
    { \
        return static_cast<Data *>(Object::m_data); \
    } \
    /* ARCHIVE_REGISTRATION(override) */ \
    ARCHIVE_REGISTRATION_INLINE \
protected: \
    bool checkImpl(std::ostream &os, bool quick) const; \
    explicit ObjType(Data *data); \
    ObjType(); \
\
private: \
    ARCHIVE_ACCESS_SPLIT \
    template<class Archive> \
    void load(Archive &ar); \
    template<class Archive> \
    void save(Archive &ar) const; \
    friend std::shared_ptr<const Object> Shm::getObjectFromHandle(const shm_handle_t &) const; \
    friend shm_handle_t Shm::getHandleFromObject(std::shared_ptr<const Object>) const; \
    friend Object *Object::create(Object::Data *); \
    friend class ObjectTypeRegistry

#define V_DATA_BEGIN(ObjType) \
public: \
    struct V_COREEXPORT Data: public Base::Data { \
        static Data *createNamed(Object::Type id, const std::string &name); \
        Data(Object::Type id, const std::string &name, const Meta &meta); \
        Data(const Data &other, const std::string &name)

#define V_DATA_END(ObjType) \
private: \
    friend class ObjType; \
    ARCHIVE_ACCESS \
    template<class Archive> \
    void serialize(Archive &ar); \
    void initData(); \
    }

#ifdef USE_BOOST_ARCHIVE
#ifdef USE_YAS
#define V_OBJECT_DECL(ObjType) \
    extern template void V_COREEXPORT ObjType::load<yas_iarchive>(yas_iarchive & ar); \
    extern template void V_COREEXPORT ObjType::load<boost_iarchive>(boost_iarchive & ar); \
    extern template void V_COREEXPORT ObjType::save<yas_oarchive>(yas_oarchive & ar) const; \
    extern template void V_COREEXPORT ObjType::save<boost_oarchive>(boost_oarchive & ar) const; \
    extern template void V_COREEXPORT ObjType::Data::serialize<yas_iarchive>(yas_iarchive & ar); \
    extern template void V_COREEXPORT ObjType::Data::serialize<boost_iarchive>(boost_iarchive & ar); \
    extern template void V_COREEXPORT ObjType::Data::serialize<yas_oarchive>(yas_oarchive & ar); \
    extern template void V_COREEXPORT ObjType::Data::serialize<boost_oarchive>(boost_oarchive & ar);

#define V_OBJECT_INST(ObjType) \
    template void V_COREEXPORT ObjType::load<yas_iarchive>(yas_iarchive & ar); \
    template void V_COREEXPORT ObjType::load<boost_iarchive>(boost_iarchive & ar); \
    template void V_COREEXPORT ObjType::save<yas_oarchive>(yas_oarchive & ar) const; \
    template void V_COREEXPORT ObjType::save<boost_oarchive>(boost_oarchive & ar) const; \
    template void V_COREEXPORT ObjType::Data::serialize<yas_iarchive>(yas_iarchive & ar); \
    template void V_COREEXPORT ObjType::Data::serialize<boost_iarchive>(boost_iarchive & ar); \
    template void V_COREEXPORT ObjType::Data::serialize<yas_oarchive>(yas_oarchive & ar); \
    template void V_COREEXPORT ObjType::Data::serialize<boost_oarchive>(boost_oarchive & ar);
#else
#define V_OBJECT_DECL(ObjType) \
    extern template void V_COREEXPORT ObjType::load<boost_iarchive>(boost_iarchive & ar); \
    extern template void V_COREEXPORT ObjType::save<boost_oarchive>(boost_oarchive & ar) const; \
    extern template void V_COREEXPORT ObjType::Data::serialize<boost_iarchive>(boost_iarchive & ar); \
    extern template void V_COREEXPORT ObjType::Data::serialize<boost_oarchive>(boost_oarchive & ar);

#define V_OBJECT_INST(ObjType) \
    template void V_COREEXPORT ObjType::load<boost_iarchive>(boost_iarchive & ar); \
    template void V_COREEXPORT ObjType::save<boost_oarchive>(boost_oarchive & ar) const; \
    template void V_COREEXPORT ObjType::Data::serialize<boost_iarchive>(boost_iarchive & ar); \
    template void V_COREEXPORT ObjType::Data::serialize<boost_oarchive>(boost_oarchive & ar);
#endif
#else
#define V_OBJECT_DECL(ObjType) \
    extern template void V_COREEXPORT ObjType::load<yas_iarchive>(yas_iarchive & ar); \
    extern template void V_COREEXPORT ObjType::save<yas_oarchive>(yas_oarchive & ar) const; \
    extern template void V_COREEXPORT ObjType::Data::serialize<yas_iarchive>(yas_iarchive & ar); \
    extern template void V_COREEXPORT ObjType::Data::serialize<yas_oarchive>(yas_oarchive & ar);

#define V_OBJECT_INST(ObjType) \
    template void V_COREEXPORT ObjType::load<yas_iarchive>(yas_iarchive & ar); \
    template void V_COREEXPORT ObjType::save<yas_oarchive>(yas_oarchive & ar) const; \
    template void V_COREEXPORT ObjType::Data::serialize<yas_iarchive>(yas_iarchive & ar); \
    template void V_COREEXPORT ObjType::Data::serialize<yas_oarchive>(yas_oarchive & ar);
#endif

//! register a new Object type (simple form for non-templates, symbol suffix determined automatically)
#define V_OBJECT_TYPE(ObjType, id) \
    Object::Type ObjType::type() \
    { \
        return id; \
    }

#define V_OBJECT_CREATE_NAMED(ObjType) \
    ObjType::Data::Data(Object::Type id, const std::string &name, const Meta &meta) \
    : ObjType::Base::Data(id, name, meta) \
    { \
        initData(); \
    } \
    ObjType::Data *ObjType::Data::createNamed(Object::Type id, const std::string &name) \
    { \
        Data *t = shm<Data>::construct(name)(id, name, Meta()); \
        publish(t); \
        return t; \
    }

#define V_OBJECT_CTOR(ObjType) \
    V_OBJECT_CREATE_NAMED(ObjType) \
    ObjType::ObjType(ObjType::Data *data): ObjType::Base(data) \
    { \
        refreshImpl(); \
    } \
    ObjType::ObjType(): ObjType::Base() \
    { \
        refreshImpl(); \
    }

#define V_OBJECT_IMPL_LOAD(ObjType) \
    template<class Archive> \
    void ObjType::load(Archive &ar) \
    { \
        std::string name; \
        ar &V_NAME(ar, "name", name); \
        int type = Object::UNKNOWN; \
        ar &V_NAME(ar, "type", type); \
        if (!ar.currentObject()) \
            ar.setCurrentObject(Object::m_data); \
        ar.registerObjectNameTranslation(name, getName()); \
        d()->template serialize<Archive>(ar); \
        assert(type == Object::getType()); \
    }

#define V_OBJECT_IMPL_SAVE(ObjType) \
    template<class Archive> \
    void ObjType::save(Archive &ar) const \
    { \
        int type = Object::getType(); \
        std::string name = d()->name; \
        ar &V_NAME(ar, "name", name); \
        ar &V_NAME(ar, "type", type); \
        const_cast<Data *>(d())->template serialize<Archive>(ar); \
    }

#define V_OBJECT_IMPL(ObjType) \
    V_OBJECT_IMPL_LOAD(ObjType) \
    V_OBJECT_IMPL_SAVE(ObjType)

void V_COREEXPORT registerTypes();

V_ENUM_OUTPUT_OP(Type, Object)

template<typename O, typename... Args>
typename O::ptr make_ptr(Args &&...args)
{
    return typename O::ptr(new O{std::forward<Args>(args)...});
}

} // namespace vistle
#endif
