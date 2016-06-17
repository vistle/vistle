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

//-------------------------------------------------------------------------
// VISTLE OBJECT OARCHIVE CLASS DECLARATION
//-------------------------------------------------------------------------
class vistleObject_oarchive {
public:
    // typedefs
    typedef boost::mpl::bool_<true> is_saving;
    typedef boost::mpl::bool_<false> is_loading;
    typedef std::vector<std::pair<std::string, const void *>> vistleObjectArchiveVector;

    // empty functions
    template<class T> void register_type() {}
    void save_binary(void *address, std::size_t count) {}

    // archive & operator functions
    template<class T> vistleObject_oarchive & operator<<(const T & t);
    template<class T> vistleObject_oarchive & operator<<(const vistle::ShmVector<T> & t);
    template<class T> vistleObject_oarchive & operator&(const T & t);

    // get/set functions
    vistleObjectArchiveVector & getVector() { return m_archiveVector; }

private:
    // private member varibles
    vistleObjectArchiveVector m_archiveVector;
};

// GENERAL OPERATOR <<
//-------------------------------------------------------------------------
template<class T>
vistleObject_oarchive & vistleObject_oarchive::operator<<(const T & t){
    const void * tPtr = reinterpret_cast<const void *>(&t);
    std::string tName = "unknown";


    m_archiveVector.push_back(std::make_pair(tName, tPtr));

   return *this;
}

// SHMVECTOR OPERATOR <<
//-------------------------------------------------------------------------
template<class T>
vistleObject_oarchive & vistleObject_oarchive::operator<<(const vistle::ShmVector<T> & t) {
    const void * tPtr = reinterpret_cast<const void *>(&t);
    std::string tName = "ShmVector";

    m_archiveVector.push_back(std::make_pair(tName, tPtr));

    return *this;
}

// GENERAL OPERATOR &
//-------------------------------------------------------------------------
template<class T>
vistleObject_oarchive & vistleObject_oarchive::operator&(const T & t) {
    return *this << t;

}



#endif /* VISTLE_OBJECT_OARCHIVE_H */
