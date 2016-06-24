//-------------------------------------------------------------------------
// VISTLE OBJECT OARCHIVE H
// *
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------

#ifndef VISTLE_OBJECT_OARCHIVE_H
#define VISTLE_OBJECT_OARCHIVE_H

#include <cstddef>
#include <stack>
#include <core/shm.h>

#include <boost/config.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/version.hpp>

#include <typeinfo>
#include <typeindex>
#include <type_traits>

#include "export.h"
#include "shm.h"

//-------------------------------------------------------------------------
// VISTLE OBJECT OARCHIVE CLASS DECLARATION
//-------------------------------------------------------------------------
class V_COREEXPORT PointerOArchive {
public:
    // temporary debug variables
    unsigned nvpCount;
    unsigned enumCount;
    unsigned primitiveCount;
    unsigned onlyCount;
    unsigned shmCount;

private:
    // archive vector entry struct
    struct ArchivedEntry {
        std::string name;
        std::string parentObjectName;
        const void * value;
        unsigned size;
        unsigned heirarchyDepth;
        bool isPointer;
        std::type_index typeInfo;

        ArchivedEntry() : typeInfo(typeid(value)) {}
        ArchivedEntry(std::string _name, std::string _parentObjectName, const void * _value, unsigned _size, unsigned _heirarchyDepth, bool _isPointer, std::type_index _typeInfo)
            : name(_name), parentObjectName(_parentObjectName), value(_value), size(_size), heirarchyDepth(_heirarchyDepth), isPointer(_isPointer), typeInfo(_typeInfo) {}
    };

    // typedefs
    typedef std::vector<ArchivedEntry> ArchiveVector;
    typedef std::stack<std::string> HeirarchyStack;

    // private member variables
    ArchiveVector m_archiveVector;
    HeirarchyStack m_heirarchyStack;
    ArchivedEntry m_currentEntry;

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
    void register_type(const T * = NULL) {}

    unsigned int get_library_version() { return 0; }
    void save_binary(const void *address, std::size_t count) {}


    // the << operators
    template<class T>
    PointerOArchive & operator<<(T const & t);
    template<class T>
    PointerOArchive & operator<<(T * const t);
    template<class T, int N>
    PointerOArchive & operator<<(const T (&t)[N]);
    template<class T>
    PointerOArchive & operator<<(const boost::serialization::nvp<T> & t);
    template<class T>
    PointerOArchive & operator<<(const vistle::ShmVector<T> & t);
    PointerOArchive & operator<<(const std::string & t);

    // the & operator
    template<class T>
    PointerOArchive & operator&(const T & t);

    // constructor - currently only in use due to debuf functions
    PointerOArchive() {nvpCount = 0; primitiveCount = 0; enumCount = 0; onlyCount = 0; shmCount = 0;}
    PointerOArchive(std::ostream &) {nvpCount = 0; primitiveCount = 0; enumCount = 0; onlyCount = 0; shmCount = 0;}


    // get functions
    ArchiveVector & vector() { return m_archiveVector; }

};


//-------------------------------------------------------------------------
// VISTLE OBJECT OARCHIVE CLASS DEFINITION
//-------------------------------------------------------------------------

// SPECIALIZED SAVE FUNCTION: ENUMS
//-------------------------------------------------------------------------
template<class Archive>
struct PointerOArchive::save_enum_type {
    template<class T>
    static void invoke(Archive &ar, const T &t){
        ar.enumCount++;

        // store entry to vector
        ar.m_currentEntry.parentObjectName = ar.m_heirarchyStack.size() > 0 ? ar.m_heirarchyStack.top() : "/";
        ar.m_currentEntry.value = (const void *) &t;
        ar.m_currentEntry.heirarchyDepth = ar.m_heirarchyStack.size();
        ar.m_currentEntry.typeInfo = typeid(t);

        ar.m_archiveVector.push_back(ar.m_currentEntry);


        return;
    }
};

// SPECIALIZED SAVE FUNCTION: PRIMITIVES
//-------------------------------------------------------------------------
template<class Archive>
struct PointerOArchive::save_primitive {
    template<class T>
    static void invoke(Archive & ar, const T & t){
        ar.primitiveCount++;

        // store entry to vector
        ar.m_currentEntry.parentObjectName = ar.m_heirarchyStack.size() > 0 ? ar.m_heirarchyStack.top() : "/";
        ar.m_currentEntry.value = (const void *) &t;
        ar.m_currentEntry.heirarchyDepth = ar.m_heirarchyStack.size();
        ar.m_currentEntry.typeInfo = typeid(t);

        ar.m_archiveVector.push_back(ar.m_currentEntry);


        return;
    }
};

// SPECIALIZED SAVE FUNCTION
// * calls serialize on all non-primitive/non-enum types
//-------------------------------------------------------------------------
template<class Archive>
struct PointerOArchive::save_only {
    template<class T>
    static void invoke(Archive & ar, const T & t){
        ar.onlyCount++;

        // store entry to vector
        ar.m_currentEntry.parentObjectName = ar.m_heirarchyStack.size() > 0 ? ar.m_heirarchyStack.top() : "/";
        ar.m_currentEntry.value = (const void *) &t;
        ar.m_currentEntry.heirarchyDepth = ar.m_heirarchyStack.size();
        ar.m_currentEntry.typeInfo = typeid(t);

        ar.m_archiveVector.push_back(ar.m_currentEntry);


        // push to stack
        ar.m_heirarchyStack.push(ar.m_currentEntry.name);

        // call serialization delegate
        boost::serialization::serialize_adl(ar, const_cast<T &>(t), ::boost::serialization::version< T >::value);

        // restore stack
        ar.m_heirarchyStack.pop();

        return;
    }
};

// UNSPECIALIZED SAVE FUNCTION
// * delegates saving functionality based on the type of the incoming variable.
//-------------------------------------------------------------------------
template<class T>
void PointerOArchive::save(const T &t){
    typedef
        BOOST_DEDUCED_TYPENAME boost::mpl::eval_if<boost::is_enum< T >,
            boost::mpl::identity<save_enum_type<PointerOArchive> >,
        //else
        BOOST_DEDUCED_TYPENAME boost::mpl::eval_if<
            // if its primitive
                boost::mpl::equal_to<
                    boost::serialization::implementation_level< T >,
                    boost::mpl::int_<boost::serialization::primitive_type>
                >,
                boost::mpl::identity<save_primitive<PointerOArchive> >,
            // else
            boost::mpl::identity<save_only<PointerOArchive> >
        > >::type typex;
    typex::invoke(*this, t);
}



// << OPERATOR: UNSPECIALISED
//-------------------------------------------------------------------------
template<class T>
PointerOArchive & PointerOArchive::operator<<(T const & t){

    save(t);

    return *this;
}

// << OPERATOR: POINTERS
//-------------------------------------------------------------------------
template<class T>
PointerOArchive & PointerOArchive::operator<<(T * const t){

    if(t != nullptr) {

        m_currentEntry.isPointer = true;

        *this << * t;
    }

    return *this;
}

// << OPERATOR: ARRAYS
//-------------------------------------------------------------------------
template<class T, int N>
PointerOArchive & PointerOArchive::operator<<(const T (&t)[N]){

    m_currentEntry.size = N;

    return *this << &t;
}

// << OPERATOR: NVP
//-------------------------------------------------------------------------
template<class T>
PointerOArchive & PointerOArchive::operator<<(const boost::serialization::nvp<T> & t){
    nvpCount++;

    m_currentEntry.name = t.name();

    return *this << t.const_value();
}

// << OPERATOR: SHMVECTOR
//-------------------------------------------------------------------------
template<class T>
PointerOArchive & PointerOArchive::operator<<(const vistle::ShmVector<T> & t) {
    shmCount++;

    m_currentEntry.size = t->size();

    return *this << t->data();
}


// & OPERATOR
//-------------------------------------------------------------------------
template<class T>
PointerOArchive & PointerOArchive::operator&(const T & t){

    // initialize member variables
    m_currentEntry.isPointer = false;
    m_currentEntry.size = 1;
    m_currentEntry.name = "";

    // delegate to appropriate << operator
    return *this << t;
}


#endif /* VISTLE_OBJECT_OARCHIVE_H */
