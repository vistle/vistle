//-------------------------------------------------------------------------
// VISTLE OBJECT OARCHIVE H
// *
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------

#ifndef VISTLE_OBJECT_OARCHIVE_H
#define VISTLE_OBJECT_OARCHIVE_H

#include <cstddef>
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
class V_COREEXPORT ShmVectorOArchive {
public:
    struct ArrayData;
private:

    // private member variables
    std::vector<ArrayData> m_arrayDataVector;
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
    void register_type(const T * = NULL) {}

    unsigned int get_library_version() { return 0; }
    void save_binary(const void *address, std::size_t count) {}


    // the << operators
    template<class T>
    ShmVectorOArchive & operator<<(T const & t);
    template<class T>
    ShmVectorOArchive & operator<<(T * const t);
    template<class T, int N>
    ShmVectorOArchive & operator<<(const T (&t)[N]);
    template<class T>
    ShmVectorOArchive & operator<<(const boost::serialization::nvp<T> & t);
    template<class T>
    ShmVectorOArchive & operator<<(vistle::ShmVector<T> & t);

    // the & operator
    template<class T>
    ShmVectorOArchive & operator&(const T & t);

    // constructor
    ShmVectorOArchive() : m_indexHint(0) {}

    // get functions
    std::vector<ArrayData> & getVector() { return m_arrayDataVector; }
    ArrayData * getVectorEntryByNvpName(std::string name);
};


//-------------------------------------------------------------------------
// VISTLE OBJECT OARCHIVE CLASS DEFINITION
//-------------------------------------------------------------------------

// ARRAY DATA STRUCT
//-------------------------------------------------------------------------
struct ShmVectorOArchive::ArrayData {
    std::string nvpName;
    std::string arrayName;
    void * ref;

    ArrayData(std::string _nvpName, std::string _arrayName, void * _ref)
        : nvpName(_nvpName), arrayName(_arrayName), ref(_ref) {}
};

// SPECIALIZED SAVE FUNCTION: ENUMS
//-------------------------------------------------------------------------
template<class Archive>
struct ShmVectorOArchive::save_enum_type {
    template<class T>
    static void invoke(Archive &ar, const T &t){

        // do nothing

        return;
    }
};

// SPECIALIZED SAVE FUNCTION: PRIMITIVES
//-------------------------------------------------------------------------
template<class Archive>
struct ShmVectorOArchive::save_primitive {
    template<class T>
    static void invoke(Archive & ar, const T & t){

        // do nothing

        return;
    }
};

// SPECIALIZED SAVE FUNCTION
// * calls serialize on all non-primitive/non-enum types
//-------------------------------------------------------------------------
template<class Archive>
struct ShmVectorOArchive::save_only {
    template<class T>
    static void invoke(Archive & ar, const T & t){

        // call serialization delegate
        boost::serialization::serialize_adl(ar, const_cast<T &>(t), ::boost::serialization::version< T >::value);

        return;
    }
};

// UNSPECIALIZED SAVE FUNCTION
// * delegates saving functionality based on the type of the incoming variable.
//-------------------------------------------------------------------------
template<class T>
void ShmVectorOArchive::save(const T &t){
    typedef
        BOOST_DEDUCED_TYPENAME boost::mpl::eval_if<boost::is_enum< T >,
            boost::mpl::identity<save_enum_type<ShmVectorOArchive> >,
        //else
        BOOST_DEDUCED_TYPENAME boost::mpl::eval_if<
            // if its primitive
                boost::mpl::equal_to<
                    boost::serialization::implementation_level< T >,
                    boost::mpl::int_<boost::serialization::primitive_type>
                >,
                boost::mpl::identity<save_primitive<ShmVectorOArchive> >,
            // else
            boost::mpl::identity<save_only<ShmVectorOArchive> >
        > >::type typex;
    typex::invoke(*this, t);
}



// << OPERATOR: UNSPECIALIZED
//-------------------------------------------------------------------------
template<class T>
ShmVectorOArchive & ShmVectorOArchive::operator<<(T const & t) {

    save(t);

    return *this;
}

// << OPERATOR: POINTERS
//-------------------------------------------------------------------------
template<class T>
ShmVectorOArchive & ShmVectorOArchive::operator<<(T * const t) {

    if(t != nullptr) {
        *this << *t;
    }

    return *this;
}

// << OPERATOR: ARRAYS
//-------------------------------------------------------------------------
template<class T, int N>
ShmVectorOArchive & ShmVectorOArchive::operator<<(const T (&t)[N]) {

    return *this << &t;
}

// << OPERATOR: NVP
//-------------------------------------------------------------------------
template<class T>
ShmVectorOArchive & ShmVectorOArchive::operator<<(const boost::serialization::nvp<T> & t) {

    m_currNvpTag = t.name();

    return *this << t.value();
}

// << OPERATOR: SHMVECTOR
//-------------------------------------------------------------------------
template<class T>
ShmVectorOArchive & ShmVectorOArchive::operator<<(vistle::ShmVector<T> & t) {

    // save shmName to archive
    m_arrayDataVector.push_back(ArrayData(m_currNvpTag, std::string(t.name().name.data()), &t));

    return *this;
}


// & OPERATOR
//-------------------------------------------------------------------------
template<class T>
ShmVectorOArchive & ShmVectorOArchive::operator&(const T & t){

    // delegate to appropriate << operator
    return *this << t;
}


#endif /* VISTLE_OBJECT_OARCHIVE_H */
