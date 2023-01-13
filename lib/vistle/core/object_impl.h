#ifndef OBJECT_IMPL_H
#define OBJECT_IMPL_H

#include "serialize.h"
#include "archives_config.h"

#ifdef USE_BOOST_ARCHIVE
#include <boost/archive/basic_archive.hpp>

namespace boost {
namespace serialization {

template<>
V_COREEXPORT void access::destroy(const vistle::shm<char>::string *t);

template<>
V_COREEXPORT void access::construct(vistle::shm<char>::string *t);

template<>
V_COREEXPORT void access::destroy(const vistle::Object::Data::AttributeList *t);

template<>
V_COREEXPORT void access::construct(vistle::Object::Data::AttributeList *t);

template<>
V_COREEXPORT void access::destroy(const vistle::Object::Data::AttributeMapValueType *t);

template<>
V_COREEXPORT void access::construct(vistle::Object::Data::AttributeMapValueType *t);

} // namespace serialization
} // namespace boost
#endif


namespace vistle {

template<class Archive>
void Object::serialize(Archive &ar)
{
    d()->serialize<Archive>(ar);
}

template<class Archive>
void Object::Data::save(Archive &ar) const
{
    ar &V_NAME(ar, "meta", meta);
    StdAttributeMap attrMap;
    auto attrs = getAttributeList();
    for (auto a: attrs) {
        attrMap[a] = getAttributes(a);
    }
    ar &V_NAME(ar, "attributes", attrMap);
}

template<class Archive>
void Object::Data::load(Archive &ar)
{
    ar &V_NAME(ar, "meta", meta);
    attributes.clear();
    StdAttributeMap attrMap;
    ar &V_NAME(ar, "attributes", attrMap);
    for (auto &kv: attrMap) {
        auto &a = kv.first;
        auto &vallist = kv.second;
        setAttributeList(a, vallist);
    }
}

template<class Archive>
void Object::saveObject(Archive &ar) const
{
    ar &V_NAME(ar, "object_name", this->getName());
    ar &V_NAME(ar, "object_type", (int)this->getType());
    this->saveToArchive(ar);
}

template<class Archive>
Object *Object::loadObject(Archive &ar)
{
    Object *obj = nullptr;
    try {
        std::string arname;
        ar &V_NAME(ar, "object_name", arname);
        std::string name = ar.translateObjectName(arname);
        int type;
        ar &V_NAME(ar, "object_type", type);
        Shm::the().lockObjects();
        Object::Data *objData = nullptr;
        if (!name.empty())
            objData = Shm::the().getObjectDataFromName(name);
        if (objData && objData->isComplete()) {
            objData->ref();
            Shm::the().unlockObjects();
            obj = Object::create(objData);
            if (!ar.currentObject())
                ar.setCurrentObject(objData);
            objData->unref();
            assert(obj->refcount() >= 1);
        } else {
            if (objData) {
                std::cerr << "Object::loadObject: have " << name << ", but incomplete, refcount=" << objData->refcount()
                          << std::endl;
                obj = Object::create(objData);
            } else {
                auto funcs = ObjectTypeRegistry::getType(type);
                obj = funcs.createEmpty(name);
                objData = obj->d();
            }
            assert(objData);
            Shm::the().unlockObjects();
            name = obj->getName();
            ar.registerObjectNameTranslation(arname, name);
            ObjectData::attachment_mutex_lock_type guard(obj->d()->attachment_mutex);
            if (!objData->isComplete() || objData->meta.creator() == -1) {
                obj->loadFromArchive(ar);
            }
            assert(obj->refcount() >= 1);
        }
#ifdef USE_BOOST_ARCHIVE
    } catch (const boost::archive::archive_exception &ex) {
        std::cerr << "Boost.Archive exception: " << ex.what() << std::endl;
        if (ex.code == boost::archive::archive_exception::unsupported_version) {
            std::cerr << "***" << std::endl;
            std::cerr << "*** received Boost archive in unsupported format, supported is up to "
                      << boost::archive::BOOST_ARCHIVE_VERSION() << std::endl;
            std::cerr << "***" << std::endl;
        }
        return obj;
#endif
    } catch (std::exception &ex) {
        std::cerr << "exception during object loading: " << ex.what() << std::endl;
        return obj;
    } catch (...) {
        throw;
    }
    assert(obj->isComplete() || ar.currentObject() == obj->d());
    if (obj->d()->unresolvedReferences == 0) {
        obj->refresh();
        assert(obj->check());
        if (ar.objectCompletionHandler())
            ar.objectCompletionHandler()();
    } else {
        //std::cerr << "LOADED " << obj->d()->name << " (" << obj->d()->type << "): " << obj->d()->unresolvedReferences << " unresolved references" << std::endl;
    }
    return obj;
}

} // namespace vistle

#endif
