//-------------------------------------------------------------------------
// FIND OBJECT REFERENCE OARCHIVE H
// *
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------

#ifndef FIND_OBJECT_REFERENCE_OARCHIVE_H
#define FIND_OBJECT_REFERENCE_OARCHIVE_H

#include "archives_config.h"

#ifdef USE_INTROSPECTION_ARCHIVE

#include <cstddef>

#include <boost/config.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/version.hpp>

#include <typeinfo>
#include <typeindex>
#include <type_traits>

#include "export.h"
#include "object.h"
#include "shm.h"

namespace vistle {


// FORWARD DECLARATIONS
//-------------------------------------------------------------------------
template<class T>
class shm_obj_ref;

//-------------------------------------------------------------------------
// FIND OBJECT REFERENCE OARCHIVE CLASS DECLARATION
//-------------------------------------------------------------------------
class V_COREEXPORT FindObjectReferenceOArchive {
public:
    enum ReferenceType { ShmVector, ObjectReference };

    struct ReferenceData;

private:
    // private member variables
    std::vector<ReferenceData> m_referenceDataVector;
    std::string m_currNvpTag;
    unsigned m_indexHint;

    // specialized save static structs
    template<class Archive>
    struct save_enum_type;

    template<class Archive>
    struct save_primitive;

    template<class Archive>
    struct save_only;

    // unspecialized save static structs
    template<class T>
    void save(const T &t);

public:
    // Implement requirements for archive concept
    typedef boost::mpl::bool_<false> is_loading;
    typedef boost::mpl::bool_<true> is_saving;

    template<class T>
    void register_type(const T * = NULL)
    {}

    unsigned int get_library_version() { return 0; }
    void save_binary(const void *address, std::size_t count) {}


    // the << operators
    template<class T>
    FindObjectReferenceOArchive &operator<<(T const &t);
    template<class T>
    FindObjectReferenceOArchive &operator<<(T *const t);
    template<class T, int N>
    FindObjectReferenceOArchive &operator<<(const T (&t)[N]);
    template<class T>
    FindObjectReferenceOArchive &operator<<(const boost::serialization::nvp<T> &t);
    template<class T>
    FindObjectReferenceOArchive &operator<<(vistle::ShmVector<T> &t);
    template<class T>
    FindObjectReferenceOArchive &operator<<(vistle::shm_obj_ref<T> &t);

    // the & operator
    template<class T>
    FindObjectReferenceOArchive &operator&(const T &t);

    // constructor
    FindObjectReferenceOArchive(): m_indexHint(0) {}

    // get functions
    std::vector<ReferenceData> &getVector() { return m_referenceDataVector; }
    ReferenceData *getVectorEntryByNvpName(std::string name);

    // public member variables
    static const std::string nullObjectReferenceName;

    template<class T>
    void saveArray(const vistle::ShmVector<T> &)
    {}

    template<class T>
    void saveObject(const vistle::shm_obj_ref<T> &)
    {}
};


//-------------------------------------------------------------------------
// FIND OBJECT REFERENCE OARCHIVE CLASS DEFINITION
//-------------------------------------------------------------------------

// ARRAY DATA STRUCT
//-------------------------------------------------------------------------
struct FindObjectReferenceOArchive::ReferenceData {
    std::string nvpName;
    std::string referenceName;
    FindObjectReferenceOArchive::ReferenceType referenceType;
    void *ref;

    ReferenceData(std::string _nvpName, std::string _referenceName,
                  FindObjectReferenceOArchive::ReferenceType _referenceType, void *_ref)
    : nvpName(_nvpName), referenceName(_referenceName), referenceType(_referenceType), ref(_ref)
    {}
};

// SPECIALIZED SAVE FUNCTION: ENUMS
//-------------------------------------------------------------------------
template<class Archive>
struct FindObjectReferenceOArchive::save_enum_type {
    template<class T>
    static void invoke(Archive &ar, const T &t)
    {
        // do nothing

        return;
    }
};

// SPECIALIZED SAVE FUNCTION: PRIMITIVES
//-------------------------------------------------------------------------
template<class Archive>
struct FindObjectReferenceOArchive::save_primitive {
    template<class T>
    static void invoke(Archive &ar, const T &t)
    {
        // do nothing

        return;
    }
};

// SPECIALIZED SAVE FUNCTION
// * calls serialize on all non-primitive/non-enum types
//-------------------------------------------------------------------------
template<class Archive>
struct FindObjectReferenceOArchive::save_only {
    template<class T>
    static void invoke(Archive &ar, const T &t)
    {
        // call serialization delegate
        boost::serialization::serialize_adl(ar, const_cast<T &>(t), ::boost::serialization::version<T>::value);

        return;
    }
};

// UNSPECIALIZED SAVE FUNCTION
// * delegates saving functionality based on the type of the incoming variable.
//-------------------------------------------------------------------------
template<class T>
void FindObjectReferenceOArchive::save(const T &t)
{
    typedef BOOST_DEDUCED_TYPENAME
        boost::mpl::eval_if<boost::is_enum<T>, boost::mpl::identity<save_enum_type<FindObjectReferenceOArchive>>,
                            //else
                            BOOST_DEDUCED_TYPENAME boost::mpl::eval_if<
                                // if its primitive
                                boost::mpl::equal_to<boost::serialization::implementation_level<T>,
                                                     boost::mpl::int_<boost::serialization::primitive_type>>,
                                boost::mpl::identity<save_primitive<FindObjectReferenceOArchive>>,
                                // else
                                boost::mpl::identity<save_only<FindObjectReferenceOArchive>>>>::type typex;
    typex::invoke(*this, t);
}


// << OPERATOR: UNSPECIALIZED
//-------------------------------------------------------------------------
template<class T>
FindObjectReferenceOArchive &FindObjectReferenceOArchive::operator<<(T const &t)
{
    save(t);

    return *this;
}

// << OPERATOR: POINTERS
//-------------------------------------------------------------------------
template<class T>
FindObjectReferenceOArchive &FindObjectReferenceOArchive::operator<<(T *const t)
{
    if (t != nullptr) {
        *this << *t;
    }

    return *this;
}

// << OPERATOR: ARRAYS
//-------------------------------------------------------------------------
template<class T, int N>
FindObjectReferenceOArchive &FindObjectReferenceOArchive::operator<<(const T (&t)[N])
{
    return *this << &t;
}

// << OPERATOR: NVP
//-------------------------------------------------------------------------
template<class T>
FindObjectReferenceOArchive &FindObjectReferenceOArchive::operator<<(const boost::serialization::nvp<T> &t)
{
    m_currNvpTag = t.name();

    return *this << t.value();
}

// << OPERATOR: SHMVECTOR
//-------------------------------------------------------------------------
template<class T>
FindObjectReferenceOArchive &FindObjectReferenceOArchive::operator<<(vistle::ShmVector<T> &t)
{
    // check for object validity
    assert(t);

    // save shmName to archive
    m_referenceDataVector.push_back(
        ReferenceData(m_currNvpTag, std::string(t.name().name.data()), ReferenceType::ShmVector, &t));

    return *this;
}

// << OPERATOR: SHM OBJECT REFERENCE
//-------------------------------------------------------------------------
template<class T>
FindObjectReferenceOArchive &FindObjectReferenceOArchive::operator<<(vistle::shm_obj_ref<T> &t)
{
    // save shmName to archive
    if (auto obj = t.getObject()) {
        m_referenceDataVector.push_back(
            ReferenceData(m_currNvpTag, obj->getName(), ReferenceType::ObjectReference, &t));
    } else {
        m_referenceDataVector.push_back(
            ReferenceData(m_currNvpTag, nullObjectReferenceName, ReferenceType::ObjectReference, &t));
    }

    return *this;
}


// & OPERATOR
//-------------------------------------------------------------------------
template<class T>
FindObjectReferenceOArchive &FindObjectReferenceOArchive::operator&(const T &t)
{
    // delegate to appropriate << operator
    return *this << t;
}

} // namespace vistle


#endif /* USE_INTROSPECTION_ARCHIVE */
#endif /* FIND_OBJECT_REFERENCE_OARCHIVE_H */
