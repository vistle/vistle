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

#include <typeinfo>
#include <typeindex>
#include <type_traits>

//-------------------------------------------------------------------------
// VISTLE OBJECT OARCHIVE CLASS DECLARATION
//-------------------------------------------------------------------------
class VistleObjectOArchive {
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
        const void * value;
        std::type_index typeInfo;
    };

    // typedefs
    typedef std::vector<ArchivedEntry> ArchiveVector;

    // private member variables
    ArchiveVector m_archiveVector;
    std::string m_currentName;
    bool m_hasName;

    // specific save class: enum type - archive concept requirements
    template<class Archive>
    struct save_enum_type {
        template<class T>
        static void invoke(Archive &ar, const T &t){
            ar.enumCount++;

            if (ar.m_hasName) {
                ar.m_archiveVector.push_back({ar.m_currentName, (const void *) &t, typeid(t)});
            }

            return;
        }
    };

    // specific save class: primitive type - archive concept requirements
    template<class Archive>
    struct save_primitive {
        template<class T>
        static void invoke(Archive & ar, const T & t){
            ar.primitiveCount++;

            if (ar.m_hasName) {
                ar.m_archiveVector.push_back({ar.m_currentName, (const void *) &t, typeid(t)});
            }

            return;
        }
    };

    // specific save class: only type - archive concept requirements
    template<class Archive>
    struct save_only {
        template<class T>
        static void invoke(Archive & ar, const T & t){
            ar.onlyCount++;

            if (ar.m_hasName) {
                ar.m_archiveVector.push_back({ar.m_currentName, (const void *) &t, typeid(t)});
            }

            boost::serialization::serialize_adl(ar, const_cast<T &>(t), ::boost::serialization::version< T >::value);

            return;
        }
    };

    // unspecified save method - archive concept requirement
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
    VistleObjectOArchive & operator<<(T const & t);
    template<class T>
    VistleObjectOArchive & operator<<(T * const t);
    template<class T, int N>
    VistleObjectOArchive & operator<<(const T (&t)[N]);
    template<class T>
    VistleObjectOArchive & operator<<(const boost::serialization::nvp<T> & t);

    // the & operator
    template<class T>
    VistleObjectOArchive & operator&(const T & t);

    // constructor - currently only in use due to debuf functions
    VistleObjectOArchive() {nvpCount = 0; primitiveCount = 0; enumCount = 0; onlyCount = 0; shmCount = 0;}

    // get functions
    ArchiveVector & getVector() { return m_archiveVector; }

};

//-------------------------------------------------------------------------
// VISTLE OBJECT OARCHIVE CLASS DEFINITION
//-------------------------------------------------------------------------

// UNSPECIFIED SAVE FUNCTION
// * delegates saving functionality based on the type of the incoming variable.
//-------------------------------------------------------------------------
template<class T>
void VistleObjectOArchive::save(const T &t){
    typedef
        BOOST_DEDUCED_TYPENAME boost::mpl::eval_if<boost::is_enum< T >,
            boost::mpl::identity<save_enum_type<VistleObjectOArchive> >,
        //else
        BOOST_DEDUCED_TYPENAME boost::mpl::eval_if<
            // if its primitive
                boost::mpl::equal_to<
                    boost::serialization::implementation_level< T >,
                    boost::mpl::int_<boost::serialization::primitive_type>
                >,
                boost::mpl::identity<save_primitive<VistleObjectOArchive> >,
            // else
            boost::mpl::identity<save_only<VistleObjectOArchive> >
        > >::type typex;
    typex::invoke(*this, t);
}

// << OPERATOR: UNSPECIALISED
//-------------------------------------------------------------------------
template<class T>
VistleObjectOArchive & VistleObjectOArchive::operator<<(T const & t){
    save(t);
    return * this;
}

// << OPERATOR: POINTERS
//-------------------------------------------------------------------------
template<class T>
VistleObjectOArchive & VistleObjectOArchive::operator<<(T * const t){

    if(t != nullptr) {
        *this << * t;
    }

    return * this;
}

// << OPERATOR: ARRAYS
//-------------------------------------------------------------------------
template<class T, int N>
VistleObjectOArchive & VistleObjectOArchive::operator<<(const T (&t)[N]){
    return *this << boost::serialization::make_array(
        static_cast<const T *>(&t[0]),
        N
    );
}

// << OPERATOR: NVP
//-------------------------------------------------------------------------
template<class T>
VistleObjectOArchive & VistleObjectOArchive::operator<<(const boost::serialization::nvp<T> & t){

    nvpCount++;

    m_currentName = t.name();
    m_hasName = true;

    * this << t.const_value();
    return * this;
}

// & OPERATOR
//-------------------------------------------------------------------------
template<class T>
VistleObjectOArchive & VistleObjectOArchive::operator&(const T & t){

    m_hasName = false;
    return * this << t;
}



#endif /* VISTLE_OBJECT_OARCHIVE_H */
