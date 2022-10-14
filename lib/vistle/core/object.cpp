#include "object.h"
#include "object_impl.h"

#include "shm.h"
#include "vector.h"
#include <cassert>

#include "archives.h"
#include <iostream>
#include <iomanip>
#include <climits>

#include <boost/mpl/for_each.hpp>

#include <vistle/util/tools.h>

#include <vistle/util/exception.h>

namespace mpl = boost::mpl;

#ifdef USE_BOOST_ARCHIVE
namespace boost {
namespace serialization {

// XXX: check these

template<>
void access::destroy(const vistle::shm<char>::string *t) // const appropriate here?
{
    const_cast<vistle::shm<char>::string *>(t)->~basic_string();
}

template<>
void access::construct(vistle::shm<char>::string *t)
{
    // default is inplace invocation of default constructor
    // Note the :: before the placement new. Required if the
    // class doesn't have a class-specific placement new defined.
    ::new (t) vistle::shm<char>::string(vistle::Shm::the().allocator());
}

template<>
void access::destroy(const vistle::Object::Data::AttributeList *t)
{
    const_cast<vistle::Object::Data::AttributeList *>(t)->~vector();
}

template<>
void access::construct(vistle::Object::Data::AttributeList *t)
{
    ::new (t) vistle::Object::Data::AttributeList(vistle::Shm::the().allocator());
}

template<>
void access::destroy(const vistle::ObjectData::AttributeMapValueType *t)
{
    const_cast<vistle::Object::Data::AttributeMapValueType *>(t)->~pair();
}

template<>
void access::construct(vistle::Object::Data::AttributeMapValueType *t)
{
    ::new (t) vistle::Object::Data::AttributeMapValueType(
        vistle::Object::Data::Key(vistle::Shm::the().allocator()),
        vistle::Object::Data::AttributeList(vistle::Shm::the().allocator()));
}

} // namespace serialization
} // namespace boost
#endif


namespace vistle {

const char *Object::toString(Type v)
{
#define V_OBJECT_CASE(sym, str) \
    case sym: \
        return #str;
    switch (v) {
        V_OBJECT_CASE(COORD, Coords)
        V_OBJECT_CASE(COORDWRADIUS, CoordWRadius)
        V_OBJECT_CASE(DATABASE, Data)

        V_OBJECT_CASE(UNKNOWN, Unknown)
        V_OBJECT_CASE(EMPTY, Empty)
        V_OBJECT_CASE(PLACEHOLDER, Placeholder)

        V_OBJECT_CASE(TEXTURE1D, Texture1D)

        V_OBJECT_CASE(POINTS, Points)
        V_OBJECT_CASE(SPHERES, Spheres)
        V_OBJECT_CASE(LINES, Lines)
        V_OBJECT_CASE(TUBES, Tubes)
        V_OBJECT_CASE(TRIANGLES, Triangles)
        V_OBJECT_CASE(QUADS, Quads)
        V_OBJECT_CASE(POLYGONS, Polygons)
        V_OBJECT_CASE(UNSTRUCTUREDGRID, UnstructuredGrid)
        V_OBJECT_CASE(UNIFORMGRID, UniformGrid)
        V_OBJECT_CASE(RECTILINEARGRID, RectilinearGrid)
        V_OBJECT_CASE(STRUCTUREDGRID, StructuredGrid)
        V_OBJECT_CASE(LAYERGRID, LayerGrid)

        V_OBJECT_CASE(VERTEXOWNERLIST, VertexOwnerList)
        V_OBJECT_CASE(CELLTREE1, Celltree1)
        V_OBJECT_CASE(CELLTREE2, Celltree2)
        V_OBJECT_CASE(CELLTREE3, Celltree3)
        V_OBJECT_CASE(NORMALS, Normals)

    default:
        break;
    }
#undef V_OBJECT_CASE

    static char buf[80];
    snprintf(buf, sizeof(buf), "invalid Object::Type (%d)", v);
    if (v >= VEC) {
        int dim = (v - Object::VEC) % (MaxDimension + 1);
        size_t scalidx = (v - Object::VEC) / (MaxDimension + 1);
#ifndef NDEBUG
        const int NumScalars = boost::mpl::size<Scalars>::value;
        assert(scalidx < NumScalars);
#endif
        const char *scalstr = "(invalid)";
        if (scalidx < ScalarTypeNames.size())
            scalstr = ScalarTypeNames[scalidx];
            //snprintf(buf, sizeof(buf), "VEC<%s,%d>", scalstr, dim);
#ifdef VISTLE_SCALAR_DOUBLE
        if (std::string("double") == scalstr)
            scalstr = "Scalar";
#else
        if (std::string("float") == scalstr)
            scalstr = "Scalar";
#endif
#ifdef VISTLE_INDEX_64BIT
        if (std::string("uint64_t") == scalstr)
            scalstr = "Index";
        if (std::string("int64_t") == scalstr)
            scalstr = "SIndex";
#else
        if (std::string("uint32_t") == scalstr)
            scalstr = "Index";
        if (std::string("int32_t") == scalstr)
            scalstr = "SIndex";
#endif
        if (std::string("unsigned char") == scalstr)
            scalstr = "Byte";
        snprintf(buf, sizeof(buf), "%s%d", scalstr, dim);
    }

    return buf;
}

std::ostream &operator<<(std::ostream &os, const Object &obj)
{
    obj.print(os);
    return os;
}

void Object::print(std::ostream &os) const
{
    os << toString(getType()) << ":" << getName() << "(#ref:" << d()->refcount();
    if (!d()->isComplete()) {
        os << " INCOMPLETE";
    }
    os << ")";
}

Object *Object::create(Data *data)
{
    if (!data)
        return nullptr;

    return ObjectTypeRegistry::getCreator(data->type)(data);
}

bool Object::isComplete() const
{
    return d()->isComplete();
}

void Object::publish(const Object::Data *d)
{
#if defined(SHMDEBUG) || defined(SHMPUBLISH)
#ifdef NO_SHMEM
    shm_handle_t handle = d;
#else
    shm_handle_t handle = Shm::the().shm().get_handle_from_address(d);
#endif
#else
    (void)d;
#endif

#ifdef SHMDEBUG
    Shm::the().addObject(d->name, handle);
#endif

#ifdef SHMPUBLISH
    Shm::the().publish(handle);
#endif
}

ObjectData::ObjectData(const Object::Type type, const std::string &n, const Meta &m)
: ShmData(ShmData::OBJECT, n)
, type(type)
, unresolvedReferences(0)
, meta(m)
, attributes(std::less<Key>(), Shm::the().allocator())
, attachments(std::less<Key>(), Shm::the().allocator())
{}

ObjectData::ObjectData(const Object::Data &o, const std::string &name, Object::Type id)
: ShmData(ShmData::OBJECT, name)
, type(id == Object::UNKNOWN ? o.type : id)
, unresolvedReferences(0)
, meta(o.meta)
, attributes(std::less<Key>(), Shm::the().allocator())
, attachments(std::less<Key>(), Shm::the().allocator())
{
    copyAttributes(&o, true);
    copyAttachments(&o, true);
}

ObjectData::~ObjectData()
{
    //std::cerr << "SHM DESTROY OBJ: " << name << std::endl;

    assert(refcount() == 0);

    for (auto &objd: attachments) {
        // referenced in addAttachment
        objd.second->unref();
    }
}

bool Object::Data::isComplete() const
{
    // a reference is only established upon return from Object::load
    //assert(unresolvedReferences==0 || refcount==0);
    //return refcount>0 && unresolvedReferences==0;
    return unresolvedReferences == 0;
}

void ObjectData::referenceResolved(const std::function<void()> &completeCallback)
{
    //std::cerr << "reference (from " << unresolvedReferences << ") resolved in " << name << std::endl;
    assert(unresolvedReferences > 0);
    if (unresolvedReferences.fetch_sub(1) == 1 && completeCallback) {
        completeCallback();
    }
}

#ifndef NO_SHMEM
void *Object::Data::operator new(size_t size)
{
    return Shm::the().shm().allocate(size);
}

void *Object::Data::operator new(std::size_t size, void *ptr)
{
    return ptr;
}

void Object::Data::operator delete(void *p)
{
    return Shm::the().shm().deallocate(p);
}

void Object::Data::operator delete(void *p, void *voidp2)
{
    return Shm::the().shm().deallocate(p);
}
#endif


ObjectData *ObjectData::create(Object::Type id, const std::string &objId, const Meta &m)
{
    std::string name = Shm::the().createObjectId(objId);
    return shm<ObjectData>::construct(name)(id, name, m);
}

Object::Object(Object::Data *data): m_data(data)
{
    ref();
#ifndef NDEBUG
    m_name = getName();
#endif
}

Object::Object(): m_data(NULL)
{
#ifndef NDEBUG
    m_name = "(NULL)";
#endif
}

Object::~Object()
{
    unref();
}

bool Object::operator==(const Object &other) const
{
    if (d() == other.d()) {
        assert(getName() == other.getName());
    } else {
        assert(getName() != other.getName());
    }
    return d() == other.d();
}

bool Object::operator!=(const Object &other) const
{
    return !(*this == other);
}

Object::ptr Object::clone() const
{
    return cloneInternal();
}

Object *Object::createEmpty(const std::string &name)
{
    assert("cannot create generic Object" == nullptr);
    return nullptr;
}

Object::ptr Object::cloneType() const
{
    return cloneTypeInternal();
}

void Object::refresh() const
{}

bool Object::check() const
{
    V_CHECK(d()->refcount() > 0); // we are holding a reference
    V_CHECK(d()->isComplete()); // we are holding a reference

    V_CHECK(d()->meta.creator() != -1);

    bool terminated = false;
    for (size_t i = 0; i < sizeof(shm_name_t); ++i) {
        if (d()->name[i] == '\0') {
            terminated = true;
            break;
        }
    }
    V_CHECK(terminated);

    V_CHECK(d()->type > UNKNOWN);

    V_CHECK(d()->meta.timeStep() >= -1);
    V_CHECK(d()->meta.timeStep() < d()->meta.numTimesteps() || d()->meta.numTimesteps() == -1);
    V_CHECK(d()->meta.animationStep() >= -1);
    V_CHECK(d()->meta.animationStep() < d()->meta.numAnimationSteps() || d()->meta.numAnimationSteps() == -1);
    V_CHECK(d()->meta.iteration() >= -1);
    V_CHECK(d()->meta.block() >= -1);
    V_CHECK(d()->meta.block() < d()->meta.numBlocks() || d()->meta.numBlocks() == -1);
    V_CHECK(d()->meta.executionCounter() >= -1);

    return true;
}

void Object::updateInternals()
{}

int Object::ref() const
{
    return d()->ref();
}

int Object::unref() const
{
    return d()->unref();
}

int Object::refcount() const
{
    return d()->refcount();
}

bool Object::isEmpty() const
{
    return true;
}

bool Object::isEmpty()
{
    return true;
}

int ObjectData::ref() const
{
    return ShmData::ref();
}

int ObjectData::unref() const
{
    Shm::the().lockObjects();
    int ref = ShmData::unref();
    if (ref == 0) {
        ObjectTypeRegistry::getDestroyer(type)(name);
    }
    Shm::the().unlockObjects();
    return ref;
}

shm_handle_t Object::getHandle() const
{
    return Shm::the().getHandleFromObject(this);
}

Object::Type Object::getType() const
{
    return d()->type;
}

std::string Object::getName() const
{
    return (d()->name.operator std::string());
}

const Meta &Object::meta() const
{
    return d()->meta;
}

void Object::setMeta(const Meta &meta)
{
    d()->meta = meta;
}

double Object::getRealTime() const
{
    return d()->meta.realTime();
}

int Object::getTimestep() const
{
    return d()->meta.timeStep();
}

int Object::getNumTimesteps() const
{
    return d()->meta.numTimesteps();
}

int Object::getBlock() const
{
    return d()->meta.block();
}

int Object::getNumBlocks() const
{
    return d()->meta.numBlocks();
}

int Object::getIteration() const
{
    return d()->meta.iteration();
}

void Object::setIteration(const int num)
{
    d()->meta.setIteration(num);
}

int Object::getExecutionCounter() const
{
    return d()->meta.executionCounter();
}

int Object::getCreator() const
{
    return d()->meta.creator();
}

Matrix4 Object::getTransform() const
{
    return d()->meta.transform();
}

void Object::setRealTime(const double time)
{
    d()->meta.setRealTime(time);
}

void Object::setTimestep(const int time)
{
    d()->meta.setTimeStep(time);
}

void Object::setNumTimesteps(const int num)
{
    d()->meta.setNumTimesteps(num);
}

void Object::setBlock(const int blk)
{
    d()->meta.setBlock(blk);
}

void Object::setNumBlocks(const int num)
{
    d()->meta.setNumBlocks(num);
}

void Object::setExecutionCounter(const int count)
{
    d()->meta.setExecutionCounter(count);
}

void Object::setCreator(const int id)
{
    d()->meta.setCreator(id);
}

void Object::setTransform(const Matrix4 &transform)
{
    d()->meta.setTransform(transform);
}

void Object::addAttribute(const std::string &key, const std::string &value)
{
    d()->addAttribute(key, value);
}

void Object::setAttributeList(const std::string &key, const std::vector<std::string> &values)
{
    d()->setAttributeList(key, values);
}

void Object::copyAttributes(Object::const_ptr src, bool replace)
{
    if (!src)
        return;

    if (replace) {
        auto &m = d()->meta;
        auto &sm = src->meta();
        m.setBlock(sm.block());
        m.setNumBlocks(sm.numBlocks());
        m.setTimeStep(sm.timeStep());
        m.setNumTimesteps(sm.numTimesteps());
        m.setRealTime(sm.realTime());
        m.setAnimationStep(sm.animationStep());
        m.setNumAnimationSteps(sm.numAnimationSteps());
        m.setIteration(sm.iteration());
        m.setTransform(sm.transform());
    }

    d()->copyAttributes(src->d(), replace);
}

bool Object::hasAttribute(const std::string &key) const
{
    return d()->hasAttribute(key);
}

std::string Object::getAttribute(const std::string &key) const
{
    return d()->getAttribute(key);
}

std::vector<std::string> Object::getAttributes(const std::string &key) const
{
    return d()->getAttributes(key);
}

std::vector<std::string> Object::getAttributeList() const
{
    return d()->getAttributeList();
}

void Object::Data::addAttribute(const std::string &key, const std::string &value)
{
    const Key skey(key.c_str(), Shm::the().allocator());
    std::pair<AttributeMap::iterator, bool> res =
        attributes.insert(AttributeMapValueType(skey, AttributeList(Shm::the().allocator())));
    AttributeList &a = res.first->second;
    a.emplace_back(value.c_str(), Shm::the().allocator());
}

void Object::Data::setAttributeList(const std::string &key, const std::vector<std::string> &values)
{
    const Key skey(key.c_str(), Shm::the().allocator());
    std::pair<AttributeMap::iterator, bool> res =
        attributes.insert(AttributeMapValueType(skey, AttributeList(Shm::the().allocator())));
    AttributeList &a = res.first->second;
    a.clear();
    for (size_t i = 0; i < values.size(); ++i) {
        a.emplace_back(values[i].c_str(), Shm::the().allocator());
    }
}

void Object::Data::copyAttributes(const ObjectData *src, bool replace)
{
    if (replace) {
        attributes = src->attributes;
    } else {
        const AttributeMap &a = src->attributes;

        for (AttributeMap::const_iterator it = a.begin(); it != a.end(); ++it) {
            const Key &key = it->first;
            const AttributeList &values = it->second;
            std::pair<AttributeMap::iterator, bool> res = attributes.insert(AttributeMapValueType(key, values));
            if (!res.second) {
                AttributeList &dest = res.first->second;
                if (replace)
                    dest.clear();
                for (AttributeList::const_iterator ait = values.begin(); ait != values.end(); ++ait) {
                    dest.emplace_back(*ait);
                }
            }
        }
    }
}

bool Object::Data::hasAttribute(const std::string &key) const
{
    const Key skey(key.c_str(), Shm::the().allocator());
    AttributeMap::const_iterator it = attributes.find(skey);
    return it != attributes.end();
}

std::string Object::Data::getAttribute(const std::string &key) const
{
    const Key skey(key.c_str(), Shm::the().allocator());
    AttributeMap::const_iterator it = attributes.find(skey);
    if (it == attributes.end())
        return std::string();
    const AttributeList &a = it->second;
    if (a.empty())
        return std::string();
    return std::string(a.back().c_str(), a.back().length());
}

std::vector<std::string> Object::Data::getAttributes(const std::string &key) const
{
    const Key skey(key.c_str(), Shm::the().allocator());
    AttributeMap::const_iterator it = attributes.find(skey);
    if (it == attributes.end())
        return std::vector<std::string>();
    const AttributeList &a = it->second;

    std::vector<std::string> attrs;
    for (AttributeList::const_iterator i = a.begin(); i != a.end(); ++i) {
        attrs.push_back(i->c_str());
    }
    return attrs;
}

std::vector<std::string> Object::Data::getAttributeList() const
{
    std::vector<std::string> result;
    for (AttributeMap::const_iterator it = attributes.begin(); it != attributes.end(); ++it) {
        auto key = it->first;
        result.push_back(key.c_str());
    }
    return result;
}

bool Object::addAttachment(const std::string &key, Object::const_ptr obj) const
{
    return d()->addAttachment(key, obj);
}

void Object::copyAttachments(Object::const_ptr src, bool replace)
{
    d()->copyAttachments(src->d(), replace);
}

bool Object::hasAttachment(const std::string &key) const
{
    return d()->hasAttachment(key);
}

Object::const_ptr Object::getAttachment(const std::string &key) const
{
    return d()->getAttachment(key);
}

bool Object::removeAttachment(const std::string &key) const
{
    return d()->removeAttachment(key);
}

bool Object::Data::hasAttachment(const std::string &key) const
{
    attachment_mutex_lock_type lock(attachment_mutex);
    const Key skey(key.c_str(), Shm::the().allocator());
    AttachmentMap::const_iterator it = attachments.find(skey);
    return it != attachments.end();
}

Object::const_ptr ObjectData::getAttachment(const std::string &key) const
{
    attachment_mutex_lock_type lock(attachment_mutex);
    const Key skey(key.c_str(), Shm::the().allocator());
    AttachmentMap::const_iterator it = attachments.find(skey);
    if (it == attachments.end()) {
        return Object::ptr();
    }
    return Object::const_ptr(Object::create(const_cast<Object::Data *>(&*it->second)));
}

bool Object::Data::addAttachment(const std::string &key, Object::const_ptr obj)
{
    {
        attachment_mutex_lock_type lock(attachment_mutex);
        const Key skey(key.c_str(), Shm::the().allocator());
        AttachmentMap::const_iterator it = attachments.find(skey);
        if (it != attachments.end()) {
            return false;
        }
        attachments.insert(AttachmentMapValueType(skey, obj->d()));
    }

    obj->ref();

    return true;
}

void Object::Data::copyAttachments(const ObjectData *src, bool replace)
{
    attachment_mutex_lock_type lock(attachment_mutex);
    const AttachmentMap &a = src->attachments;

    for (AttachmentMap::const_iterator it = a.begin(); it != a.end(); ++it) {
        const Key &key = it->first;
        const Attachment &value = it->second;
        auto res = attachments.insert(AttachmentMapValueType(key, value));
        if (res.second) {
            value->ref();
        } else {
            if (replace) {
                const Attachment &oldVal = res.first->second;
                oldVal->unref();
                res.first->second = value;
                value->ref();
            }
        }
    }
}

bool Object::Data::removeAttachment(const std::string &key)
{
    attachment_mutex_lock_type lock(attachment_mutex);
    const Key skey(key.c_str(), Shm::the().allocator());
    AttachmentMap::iterator it = attachments.find(skey);
    if (it == attachments.end()) {
        return false;
    }

    it->second->unref();
    attachments.erase(it);

    return true;
}

void ObjectData::unresolvedReference()
{
    ++unresolvedReferences;
}

const struct ObjectTypeRegistry::FunctionTable &ObjectTypeRegistry::getType(int id)
{
    TypeMap::const_iterator it = typeMap().find(id);
    if (it == typeMap().end()) {
        std::stringstream str;
        str << "ObjectTypeRegistry: no creator for type id " << id << " (" << typeMap().size() << " total entries)";
        throw vistle::exception(str.str());
    }
    return (*it).second;
}

ObjectTypeRegistry::CreateFunc ObjectTypeRegistry::getCreator(int id)
{
    return getType(id).create;
}

ObjectTypeRegistry::DestroyFunc ObjectTypeRegistry::getDestroyer(int id)
{
    return getType(id).destroy;
}

ObjectTypeRegistry::TypeMap &ObjectTypeRegistry::typeMap()
{
    static TypeMap m;
    return m;
}

#ifdef USE_BOOST_ARCHIVE
template Object V_COREEXPORT *Object::loadObject<boost_iarchive>(boost_iarchive &ar);
template void V_COREEXPORT Object::saveObject<boost_oarchive>(boost_oarchive &ar) const;
template void V_COREEXPORT Object::serialize<boost_iarchive>(boost_iarchive &ar);
template void V_COREEXPORT Object::serialize<boost_oarchive>(boost_oarchive &ar);
template void V_COREEXPORT Object::Data::load<boost_iarchive>(boost_iarchive &ar);
template void V_COREEXPORT Object::Data::save<boost_oarchive>(boost_oarchive &ar) const;
#endif
#ifdef USE_YAS
template Object V_COREEXPORT *Object::loadObject<yas_iarchive>(yas_iarchive &ar);
template void V_COREEXPORT Object::saveObject<yas_oarchive>(yas_oarchive &ar) const;
template void V_COREEXPORT Object::serialize<yas_iarchive>(yas_iarchive &ar);
template void V_COREEXPORT Object::serialize<yas_oarchive>(yas_oarchive &ar);
template void V_COREEXPORT Object::Data::load<yas_iarchive>(yas_iarchive &ar);
template void V_COREEXPORT Object::Data::save<yas_oarchive>(yas_oarchive &ar) const;
#endif

} // namespace vistle
