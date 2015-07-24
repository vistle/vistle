#ifndef ARCHIVES_H
#define ARCHIVES_H

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <boost/archive/detail/oserializer.hpp>
#include <boost/archive/detail/iserializer.hpp>

#include <boost/mpl/vector.hpp>

namespace vistle {

class oarchive: public boost::archive::binary_oarchive_impl<oarchive, std::ostream::char_type, std::ostream::traits_type> {

    typedef boost::archive::binary_oarchive_impl<oarchive, std::ostream::char_type, std::ostream::traits_type> Base;
public:
    oarchive(std::ostream &os, unsigned int flags=0)
       : boost::archive::binary_oarchive_impl<oarchive, std::ostream::char_type, std::ostream::traits_type>(os, flags)
    {}
    oarchive(std::streambuf &bsb, unsigned int flags=0)
       : boost::archive::binary_oarchive_impl<oarchive, std::ostream::char_type, std::ostream::traits_type>(bsb, flags)
    {}

};

class iarchive: public boost::archive::binary_iarchive_impl<iarchive, std::istream::char_type, std::istream::traits_type> {

    typedef boost::archive::binary_iarchive_impl<iarchive, std::istream::char_type, std::istream::traits_type> Base;
public:
    iarchive(std::istream &is, unsigned int flags=0);
    iarchive(std::streambuf &bsb, unsigned int flags=0);

    void setSource(int hub, int rank);
    int sourceHub() const;
    int sourceRank() const;

private:
    int m_hub, m_rank;
};

typedef boost::mpl::vector<
   iarchive
      > InputArchives;

typedef boost::mpl::vector<
   oarchive
      > OutputArchives;

} // namespace vistle

BOOST_SERIALIZATION_REGISTER_ARCHIVE(vistle::oarchive)
BOOST_SERIALIZATION_USE_ARRAY_OPTIMIZATION(vistle::oarchive)
BOOST_SERIALIZATION_REGISTER_ARCHIVE(vistle::iarchive)
BOOST_SERIALIZATION_USE_ARRAY_OPTIMIZATION(vistle::iarchive)

#endif
