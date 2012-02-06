#define __STDC_FORMAT_MACROS
#include <stdio.h>
#include <inttypes.h>

#include <sstream>
#include <iomanip>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <fstream>
#include <iostream>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_multi_pass.hpp>
#include <boost/spirit/include/classic_position_iterator.hpp>

#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/iterator.hpp>
#include <boost/fusion/include/iterator.hpp>
#include <boost/fusion/container/vector/vector_fwd.hpp>
#include <boost/fusion/include/vector_fwd.hpp>

#include "object.h"

#include "ReadFOAM.h"

template<typename Alloc = std::allocator<char> >
struct basic_gzip_decompressor;
typedef basic_gzip_decompressor<> gzip_decompressor;

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace classic = boost::spirit::classic;

MODULE_MAIN(ReadFOAM)

ReadFOAM::ReadFOAM(int rank, int size, int moduleID)
   : Module("ReadFOAM", rank, size, moduleID) {

   createOutputPort("grid_out");
   addFileParameter("filename", "");
}

ReadFOAM::~ReadFOAM() {

}

template <typename Iterator>
struct skipper: qi::grammar<Iterator> {

   skipper(): skipper::base_type(start) {
      qi::char_type char_;
      ascii::space_type space;

      start =
         space
         | "/*" >> *(char_ - "*/") >> "*/"
         | "//" >> *(ascii::char_ - qi::eol) >> qi::eol
         | "FoamFile" >> *ascii::space >> '{' >> *(ascii::char_ - '}') >> '}'
         ;
   }

   qi::rule<Iterator> start;
};

BOOST_FUSION_ADAPT_STRUCT(vistle::Vector,
                          (float, x)
                          (float, y)
                          (float, z))

template <typename Iterator>
struct PointParser: qi::grammar<Iterator, std::vector<vistle::Vector>(),
                                skipper<Iterator> > {

   PointParser(): PointParser::base_type(start) {

      start =
         boost::spirit::omit[qi::int_] >> '(' >> *term >> ')'
         ;

      term =
         '(' >> qi::float_ >> qi::float_ >> qi::float_ >> ')'
         ;
   }

   qi::rule<Iterator, std::vector<vistle::Vector>(), skipper<Iterator> > start;
   qi::rule<Iterator, vistle::Vector(), skipper<Iterator> > term;
};

template <typename Iterator>
struct FaceParser: qi::grammar<Iterator, std::vector<std::vector<int> >(),
                               skipper<Iterator> > {

   FaceParser(): FaceParser::base_type(start) {

      start =
         boost::spirit::omit[qi::int_]
         >> '(' >> *term >> ')'
         ;

      term =
         boost::spirit::omit[qi::int_] >>
         '(' >> qi::int_ >> qi::int_ >> qi::int_ >> qi::int_ >> ')'
         ;
   }

   qi::rule<Iterator, std::vector<std::vector<int> >(), skipper<Iterator> > start;
   qi::rule<Iterator, std::vector<int>(), skipper<Iterator> > term;
};

template <typename Iterator>
struct IntParser: qi::grammar<Iterator, std::vector<int>(),
                               skipper<Iterator> > {

   IntParser(): IntParser::base_type(start) {

      start =
         boost::spirit::omit[qi::int_] >> boost::spirit::omit[*ascii::space]
                                       >> '(' >> *term >> ')'
         ;

      term = qi::int_
         ;
   }

   qi::rule<Iterator, std::vector<int>(), skipper<Iterator> > start;
   qi::rule<Iterator, int(), skipper<Iterator> > term;
};

vistle::Object * ReadFOAM::load(const std::string & casedir) {

   std::string pointsName("/data/OpenFOAM/PumpTurbine/transient/processor0/constant/polyMesh/points.gz");
   std::ifstream pointsFile(pointsName.c_str(),
                            std::ios_base::in | std::ios_base::binary);

   std::string facesName("/data/OpenFOAM/PumpTurbine/transient/processor0/constant/polyMesh/faces.gz");
   std::ifstream facesFile(facesName.c_str(),
                           std::ios_base::in | std::ios_base::binary);

   std::string ownersName("/data/OpenFOAM/PumpTurbine/transient/processor0/constant/polyMesh/owner.gz");
   std::ifstream ownersFile(ownersName.c_str(),
                            std::ios_base::in | std::ios_base::binary);

   std::string neighborsName("/data/OpenFOAM/PumpTurbine/transient/processor0/constant/polyMesh/neighbour.gz");
   std::ifstream neighborsFile(neighborsName.c_str(),
                               std::ios_base::in | std::ios_base::binary);

   boost::iostreams::filtering_istream pointsIn;
   pointsIn.push(boost::iostreams::gzip_decompressor());
   pointsIn.push(pointsFile);

   boost::iostreams::filtering_istream facesIn;
   facesIn.push(boost::iostreams::gzip_decompressor());
   facesIn.push(facesFile);

   boost::iostreams::filtering_istream ownersIn;
   ownersIn.push(boost::iostreams::gzip_decompressor());
   ownersIn.push(ownersFile);

   boost::iostreams::filtering_istream neighborsIn;
   neighborsIn.push(boost::iostreams::gzip_decompressor());
   neighborsIn.push(neighborsFile);

   std::vector<vistle::Vector> points;
   std::vector<std::vector<int> > faces;
   std::vector<int> owners;
   std::vector<int> neighbors;

   typedef std::istreambuf_iterator<char> base_iterator_type;
   typedef boost::spirit::multi_pass<base_iterator_type> forward_iterator_type;
   typedef classic::position_iterator2<forward_iterator_type> pos_iterator_type;

   struct skipper<pos_iterator_type> skipper;
   struct PointParser<pos_iterator_type> pointParser;
   struct FaceParser<pos_iterator_type> faceParser;
   struct IntParser<pos_iterator_type> ownersParser;
   struct IntParser<pos_iterator_type> neighborsParser;

   try {

      forward_iterator_type fwd_begin =
         boost::spirit::make_default_multi_pass(base_iterator_type(pointsIn));
      forward_iterator_type fwd_end;

      pos_iterator_type pos_begin(fwd_begin, fwd_end, pointsName);
      pos_iterator_type pos_end;

      bool r = qi::phrase_parse(pos_begin, pos_end,
                                pointParser, skipper, points);
      std::cout << "r: " << r << " points: " << points.size() << std::endl;

      fwd_begin =
         boost::spirit::make_default_multi_pass(base_iterator_type(facesIn));

      pos_begin = pos_iterator_type(fwd_begin, fwd_end, facesName);
      r = qi::phrase_parse(pos_begin, pos_end,
                                faceParser, skipper, faces);
      std::cout << "r: " << r << " faces: " << faces.size() << std::endl;

      fwd_begin =
         boost::spirit::make_default_multi_pass(base_iterator_type(ownersIn));

      pos_begin = pos_iterator_type(fwd_begin, fwd_end, ownersName);
      r = qi::phrase_parse(pos_begin, pos_end,
                           ownersParser, skipper, owners);
      std::cout << "r: " << r << " owners: " << owners.size() << std::endl;

      fwd_begin =
         boost::spirit::make_default_multi_pass(base_iterator_type(neighborsIn));

      pos_begin = pos_iterator_type(fwd_begin, fwd_end, neighborsName);
      r = qi::phrase_parse(pos_begin, pos_end,
                           neighborsParser, skipper, neighbors);
      std::cout << "r: " << r << " neighbors: " << neighbors.size() << std::endl;

      /*
      if (r) {
         std::vector<vistle::Vector>::iterator i;

         for (i = points.begin(); i != points.end(); i ++)
            std::cout << "(" << i->x << ", " << i->y << ", " << i->z << ")"
                      << std::endl;
      }
      */

   } catch(const qi::expectation_failure<pos_iterator_type>& e) {

      const classic::file_position_base<std::string>& pos =
         e.first.get_position();
      std::cout <<
         "parse error at file " << pos.file <<
         " line " << pos.line << " column " << pos.column << std::endl <<
         "'" << e.first.get_currentline() << "'" << std::endl <<
         std::setw(pos.column) << " " << "^- here" << std::endl;
   }

   return NULL;
}

bool ReadFOAM::compute() {

   vistle::Object * object = load(getFileParameter("filename"));
   (void) object;

   return true;
}
