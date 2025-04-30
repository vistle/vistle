#include "archive_loader.h"
#include "archives.h"
#include "archives_impl.h"
#include "shm_array_impl.h"
#include "object.h"

#include <boost/mpl/for_each.hpp>

#include <iostream>

namespace vistle {

DeepArchiveFetcher::DeepArchiveFetcher(const std::map<std::string, buffer> &objects,
                                       const std::map<std::string, buffer> &arrays,
                                       const std::map<std::string, message::CompressionMode> &compressions,
                                       const std::map<std::string, size_t> &sizes)
: m_objects(objects), m_arrays(arrays), m_compression(compressions), m_rawSize(sizes)
{}

void DeepArchiveFetcher::requestArray(const std::string &arname, int type,
                                      const ArrayCompletionHandler &completeCallback)
{
    //std::cerr << "DeepArchiveFetcher: trying array " << arname << std::endl;
    auto it = m_arrays.find(arname);
    if (it == m_arrays.end()) {
        std::cerr << "DeepArchiveFetcher: did not find array " << arname << std::endl;
        return;
    }
    message::CompressionMode comp = message::CompressionNone;
    auto itc = m_compression.find(arname);
    if (itc != m_compression.end()) {
        comp = itc->second;
    }
    size_t size = 0;
    auto its = m_rawSize.find(arname);
    if (its != m_rawSize.end()) {
        size = its->second;
    }
    buffer raw;
    if (comp != message::CompressionNone) {
        try {
            raw = message::decompressPayload(comp, it->second.size(), size, it->second.data());
        } catch (const std::exception &ex) {
            std::cerr << "DeepArchiveFetcher: failed to decompress array " << arname << ": " << ex.what() << std::endl;
            return;
        }
    }
    vecistreambuf<buffer> vb(comp == message::CompressionNone ? it->second : raw);
    try {
        iarchive ar(vb);
        ar.setFetcher(shared_from_this());
        ArrayLoader loader(arname, type, ar);
        if (loader.load()) {
            //std::cerr << "DeepArchiveFetcher: success array " << arname << std::endl;
            m_ownedArrays.emplace(loader.owner());
            completeCallback(ar.translateArrayName(arname));
        } else {
            std::cerr << "DeepArchiveFetcher: failed to load array " << arname << " of type " << type << std::endl;
        }
    } catch (std::exception &ex) {
        std::cerr << "DeepArchiveFetcher: exception " << ex.what() << " while loading array " << arname << " of type "
                  << type << std::endl;
        throw ex;
    }
}

void DeepArchiveFetcher::requestObject(const std::string &arname, const ObjectCompletionHandler &completeCallback)
{
    //std::cerr << "DeepArchiveFetcher: trying object " << arname << std::endl;
    auto it = m_objects.find(arname);
    if (it == m_objects.end()) {
        std::cerr << "DeepArchiveFetcher: did not find object " << arname << std::endl;
        return;
    }
    message::CompressionMode comp = message::CompressionNone;
    auto itc = m_compression.find(arname);
    if (itc != m_compression.end()) {
        comp = itc->second;
    }
    size_t size = 0;
    auto its = m_rawSize.find(arname);
    if (its != m_rawSize.end()) {
        size = its->second;
    }
    buffer raw;
    if (comp != message::CompressionNone) {
        try {
            raw = message::decompressPayload(comp, it->second.size(), size, it->second.data());
        } catch (const std::exception &ex) {
            std::cerr << "DeepArchiveFetcher: failed to decompress object " << arname << ": " << ex.what() << std::endl;
            return;
        }
    }
    vecistreambuf<buffer> vb(comp == message::CompressionNone ? it->second : raw);
    iarchive ar(vb);
    ar.setFetcher(shared_from_this());
    Object::ptr obj(Object::loadObject(ar));
    if (obj && obj->isComplete()) {
        //std::cerr << "DeepArchiveFetcher: success object " << arname << ", created as " << obj->getName() << std::endl;
        completeCallback(obj);
    } else {
        std::cerr << "DeepArchiveFetcher: failed to load object " << arname << std::endl;
    }
}

bool DeepArchiveFetcher::renameObjects() const
{
    return m_rename;
}

std::string DeepArchiveFetcher::translateObjectName(const std::string &name) const
{
    if (!m_rename)
        return name;

    auto it = m_transObject.find(name);
    if (it == m_transObject.end())
        return std::string();

    return it->second;
}

std::string DeepArchiveFetcher::translateArrayName(const std::string &name) const
{
    if (!m_rename)
        return name;

    auto it = m_transArray.find(name);
    if (it == m_transArray.end())
        return std::string();

    return it->second;
}

void DeepArchiveFetcher::registerObjectNameTranslation(const std::string &arname, const std::string &name)
{
    if (!m_rename)
        return;
    auto p = m_transObject.emplace(arname, name);

    if (p.second) {
        //std::cerr << "yas_iarchive: object name translation for " << arname << " was already registered" << std::endl;
    }
}

void DeepArchiveFetcher::registerArrayNameTranslation(const std::string &arname, const std::string &name)
{
    if (!m_rename)
        return;

    auto p = m_transArray.emplace(arname, name);
    if (p.second) {
        //std::cerr << "yas_iarchive: array name translation for " << arname << " was already registered" << std::endl;
    }
}

void DeepArchiveFetcher::setRenameObjects(bool rename)
{
    m_rename = rename;
}

std::map<std::string, std::string> DeepArchiveFetcher::objectTranslations() const
{
    return m_transObject;
}

std::map<std::string, std::string> DeepArchiveFetcher::arrayTranslations() const
{
    return m_transArray;
}

void DeepArchiveFetcher::setObjectTranslations(const std::map<std::string, std::string> &objs)
{
    m_transObject = objs;
}

void DeepArchiveFetcher::setArrayTranslations(const std::map<std::string, std::string> &arrs)
{
    m_transArray = arrs;
}

void DeepArchiveFetcher::releaseArrays()
{
    m_ownedArrays.clear();
}

std::ostream &operator<<(std::ostream &s, const DeepArchiveFetcher &daf)
{
    s << "Objects:";
    for (const auto &o: daf.m_objects) {
        s << " " << o.first << "("
          << ")";
    }
    s << " ";
    s << "Arrays:";
    for (const auto &a: daf.m_arrays) {
        auto it = daf.m_rawSize.find(a.first);
        size_t unc = 0;
        if (it != daf.m_rawSize.end())
            unc = it->second;
        s << " " << a.first << "(" << a.second.size() << "->" << unc << ")";
    }
    return s;
}

ArrayLoader::ArrayLoader(const std::string &name, int type, const iarchive &ar)
: m_ok(false), m_arname(name), m_type(type), m_ar(ar)
{
    m_name = m_ar.translateArrayName(m_arname);
    //std::cerr << "ArrayLoader: loading array " << m_arname << ", translates to " << m_name << std::endl;
}

template<typename T>
void ArrayLoader::operator()(T)
{
    if (shm_array<T, typename shm<T>::allocator>::typeId() != m_type) {
        return;
    }

    if (m_ok) {
        m_ok = false;
        std::cerr << "ArrayLoader: multiple type matches for data array " << m_name << std::endl;
        return;
    }

    if (!m_name.empty()) {
        if (auto arr = Shm::the().getArrayFromName<T>(m_name)) {
            std::cerr << "ArrayLoader: already have data array with name " << m_name << std::endl;
            m_unreffer.reset(new Unreffer<T>(arr));
            m_ok = true;
            return;
        }
    }

    auto &ar = const_cast<vistle::iarchive &>(m_ar);
    std::string arname;
    ar &arname;
    assert(arname == m_arname);
    m_name = ar.translateArrayName(arname);
    auto arr = ShmVector<T>((shm_name_t)m_name);
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

bool ArrayLoader::load()
{
    try {
        boost::mpl::for_each<VectorTypes>(boost::reference_wrapper<ArrayLoader>(*this));
        if (!m_ok) {
            std::cerr << "ArrayLoader: failed to restore array " << m_name << " from archive" << std::endl;
        }
    } catch (std::exception &ex) {
        m_ok = false;
        std::cerr << "ArrayLoader: failed to restore array " << m_name << " from archive - exception: " << ex.what()
                  << std::endl;
    }
    return m_ok;
}

const std::string &ArrayLoader::name() const
{
    return m_name;
}

std::shared_ptr<ArrayLoader::ArrayOwner> ArrayLoader::owner() const
{
    return m_unreffer;
}

} // namespace vistle
