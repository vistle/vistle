#ifndef VISTLE_CORE_OBJECT_IMPL_H
#define VISTLE_CORE_OBJECT_IMPL_H

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
    Object::Data *objData = nullptr;
    auto lambda = [&obj, &objData, &ar]() {
        std::string arname;
        ar &V_NAME(ar, "object_name", arname);
        std::string name = ar.translateObjectName(arname);
        int type;
        ar &V_NAME(ar, "object_type", type);
        if (!name.empty())
            objData = Shm::the().getObjectDataFromName(name);
        if (!ar.currentObject())
            ar.setCurrentObject(objData);
        if (objData && objData->isComplete()) {
            obj = Object::create(objData);
        } else if (objData) {
            std::cerr << "Object::loadObject: have " << name << ", but incomplete, refcount=" << objData->refcount()
                      << std::endl;
            obj = Object::create(objData);
        } else {
            auto funcs = ObjectTypeRegistry::getType(type);
            obj = funcs.createEmpty(name);
            objData = obj->d();
            assert(objData);
            if (!ar.currentObject())
                ar.setCurrentObject(objData);
            name = obj->getName();
            ar.registerObjectNameTranslation(arname, name);
        }
        assert(obj->refcount() >= 1);
    };
    try {
        Shm::the().atomicFunc(lambda);
        // lock so that only one thread restores object from archive
        ObjectData::mutex_lock_type guard(obj->d()->object_mutex);
        if (!objData->isComplete() || objData->meta.creator() == -1) {
            objData->meta.setRestoring(true);
            obj->loadFromArchive(ar);
            objData->meta.setRestoring(false);
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
#endif
    } catch (std::exception &ex) {
        std::cerr << "exception during object loading: " << ex.what() << std::endl;
    } catch (...) {
        throw;
    }
    if (obj) {
        assert(objData == obj->d());
        assert(obj->isComplete() || ar.currentObject() == obj->d());
        if (obj->isComplete()) {
            obj->refresh();
            assert(obj->check(std::cerr));
            if (ar.objectCompletionHandler())
                ar.objectCompletionHandler()();
        } else {
            //std::cerr << "LOADED " << obj->d()->name << " (" << obj->d()->type << "): " << obj->d()->unresolvedReferences << " unresolved references" << std::endl;
        }
    }
    return obj;
}

} // namespace vistle

#endif
