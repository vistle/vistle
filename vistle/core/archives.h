#ifndef ARCHIVES_H
#define ARCHIVES_H

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <boost/mpl/vector.hpp>

namespace vistle {

typedef boost::mpl::vector<
   boost::archive::xml_iarchive,
   boost::archive::text_iarchive,
   boost::archive::binary_iarchive
      > InputArchives;

typedef boost::mpl::vector<
   boost::archive::xml_oarchive,
   boost::archive::text_oarchive,
   boost::archive::binary_oarchive
      > OutputArchives;

}

#endif
