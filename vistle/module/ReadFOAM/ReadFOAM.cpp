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
#include <sstream>
#include <iostream>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_no_skip.hpp>
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
#include <boost/fusion/include/std_pair.hpp>

#include "object.h"

#include "ReadFOAM.h"

namespace bi = boost::iostreams;
namespace bs = boost::spirit;

template<typename Alloc = std::allocator<char> >
struct basic_gzip_decompressor;
typedef basic_gzip_decompressor<> gzip_decompressor;

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace classic = boost::spirit::classic;

MODULE_MAIN(ReadFOAM)

class Boundary {
public:
   Boundary(const size_t f, const size_t t, const size_t n, const size_t s)
      : from(f), to(t), numFaces(n), startFace(s) {

   }

   const size_t from;
   const size_t to;
   const size_t numFaces;
   const size_t startFace;
};


class Boundaries {
public:
   Boundaries(const size_t f): from(f) { }

   bool isBoundaryFace(const size_t face) {

      std::vector<Boundary *>::iterator i;
      for (i = boundaries.begin(); i != boundaries.end(); i ++) {

         if (face >= (*i)->startFace && face < (*i)->startFace + (*i)->numFaces)
            return true;
      }
      return false;
   }

   void addBoundary(Boundary *b) {

      boundaries.push_back(b);
   }

private:
   const size_t from;
   std::vector<Boundary *> boundaries;
};

ReadFOAM::ReadFOAM(int rank, int size, int moduleID)
   : Module("ReadFOAM", rank, size, moduleID) {

   createOutputPort("grid_out");
   createOutputPort("p_out");
   addFileParameter("filename", "");
}

ReadFOAM::~ReadFOAM() {

}

template <typename Iterator>
struct skipper: qi::grammar<Iterator> {

   skipper(): skipper::base_type(start) {

      start =
         ascii::space
         | "/*" >> *(ascii::char_ - "*/") >> "*/"
         | "//" >> *(ascii::char_ - qi::eol) >> qi::eol
         | "FoamFile" >> *ascii::space >> '{' >> *(ascii::char_ - '}') >> '}'
         | "dimensions" >> *(ascii::char_ - qi::eol) >> qi::eol
         | "internalField" >> *(ascii::char_ - qi::eol) >> qi::eol
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
         qi::omit[qi::int_] >> '(' >> *term >> ')'
         ;

      term =
         '(' >> qi::float_ >> qi::float_ >> qi::float_ >> ')'
         ;
   }

   qi::rule<Iterator, std::vector<vistle::Vector>(), skipper<Iterator> > start;
   qi::rule<Iterator, vistle::Vector(), skipper<Iterator> > term;
};

template <typename Iterator>
struct FaceParser: qi::grammar<Iterator, std::vector<std::vector<size_t> >(),
                               skipper<Iterator> > {

   FaceParser(): FaceParser::base_type(start) {

      start =
         qi::omit[qi::int_]
         >> '(' >> *term >> ')'
         ;

      term =
         qi::omit[qi::int_] >>
         '(' >> qi::int_ >> qi::int_ >> qi::int_ >> qi::int_ >> ')'
         ;
   }

   qi::rule<Iterator, std::vector<std::vector<size_t> >(), skipper<Iterator> > start;
   qi::rule<Iterator, std::vector<size_t>(), skipper<Iterator> > term;
};

template <typename Iterator>
struct IntParser: qi::grammar<Iterator, std::vector<size_t>(),
                               skipper<Iterator> > {

   IntParser(): IntParser::base_type(start) {

      start =
         qi::omit[qi::int_] >> qi::omit[*ascii::space]
                                       >> '(' >> *term >> ')'
         ;

      term = qi::int_
         ;
   }

   qi::rule<Iterator, std::vector<size_t>(), skipper<Iterator> > start;
   qi::rule<Iterator, size_t(), skipper<Iterator> > term;
};

template <typename Iterator>
struct ScalarParser: qi::grammar<Iterator, std::vector<float>(),
                                 skipper<Iterator> > {

   ScalarParser(): ScalarParser::base_type(start) {

      start =
         qi::omit[qi::int_] >> qi::omit[*ascii::space]
                                       >> '(' >> *term >> ')'
         ;

      term = qi::float_
         ;
   }

   qi::rule<Iterator, std::vector<float>(), skipper<Iterator> > start;
   qi::rule<Iterator, float(), skipper<Iterator> > term;
};

template <typename Iterator>
struct VectorParser: qi::grammar<Iterator, std::vector<std::vector<float> >(),
                                 skipper<Iterator> > {

   VectorParser(): VectorParser::base_type(start) {

      start =
         qi::omit[qi::int_] >> qi::omit[*ascii::space]
                                       >> '(' >> *term >> ')'
         ;

      term =
         '(' >> +qi::float_ >> ')'
         ;
   }

   qi::rule<Iterator, std::vector<std::vector<float> >(), skipper<Iterator> > start;
   qi::rule<Iterator, std::vector<float>(), skipper<Iterator> > term;
};

template <typename Iterator>
struct BoundaryParser
   : qi::grammar<Iterator, std::map<std::string, std::map<std::string, std::string> >(),
                 skipper<Iterator> >
{
   BoundaryParser()
      : BoundaryParser::base_type(start)
   {
      start = qi::omit[qi::int_] >> '(' >> +mapmap >> ')';
      mapmap = +(qi::char_ - '{') >> '{' >> entrymap >> '}';
      entrymap = +pair;
      pair  = qi::lexeme[+qi::char_("a-zA-Z")] >> +(qi::char_ - ';') >> ';';
   }

   qi::rule<Iterator, std::map<std::string, std::map<std::string, std::string> >(),
            skipper<Iterator> > start;

   qi::rule<Iterator, std::pair<std::string, std::map<std::string, std::string> >(),
            skipper<Iterator> > mapmap;

   qi::rule<Iterator, std::map<std::string, std::string>(),
            skipper<Iterator> > entrymap;

   qi::rule<Iterator, std::pair<std::string, std::string>(),
            skipper<Iterator> > pair;
};


size_t getFirstFace(size_t cell,
              const std::vector<std::vector<size_t> > & faces,
              const std::map<size_t, std::vector<size_t> > & cellFaceMapping,
              const std::map<size_t, std::vector<size_t> > & cellFaceNeighbors,
              std::vector<size_t> & faceIndices) {

   std::map<size_t, std::vector<size_t> >::const_iterator cellFaces =
      cellFaceMapping.find(cell);

   if (cellFaces != cellFaceMapping.end()) {

      std::vector<size_t>::const_iterator i = cellFaces->second.begin();
      if (i != cellFaces->second.end()) {

         std::vector<size_t>::const_iterator face;
         for (face = faces[*i].begin(); face != faces[*i].end(); face ++)
            faceIndices.push_back(*face);

         return *i;
      }
   }

   return -1;
}

int compareFaces(const std::vector<size_t> & a,
                 const std::vector<size_t> & b) {

   int num = 0;
   std::vector<size_t>::const_iterator ai, bi;

   for (ai = a.begin(); ai != a.end(); ai ++)
      for (bi = b.begin(); bi != b.end(); bi ++)
         if (*ai == *bi)
            num ++;

   return num;
}

bool inFace(const size_t vertexA, const size_t vertexB,
            const std::vector<size_t> & faceIndices) {

   bool containsA = false;
   bool containsB = false;

   std::vector<size_t>::const_iterator i;
   for (i = faceIndices.begin(); i != faceIndices.end(); i ++) {

      if (*i == vertexA)
         containsA = true;

      if (*i == vertexB)
         containsB = true;
   }

   return (containsA && containsB);
}

size_t getOppositeVertexInFace(const size_t vertex, const size_t cell,
                     const std::vector<std::vector<size_t> > & faces,
                     const std::vector<size_t> & faceIndices,
                     const std::map<size_t, std::vector<size_t> > & cellFaceMapping,
                     const std::map<size_t, std::vector<size_t> > & cellFaceNeighbors) {

   std::set<size_t> shared;
   std::vector<size_t> cellFaces;
   std::map<size_t, std::vector<size_t> >::const_iterator ci =
      cellFaceMapping.find(cell);
   if (ci != cellFaceMapping.end())
      std::copy(ci->second.begin(), ci->second.end(), std::back_inserter(cellFaces));
   ci = cellFaceNeighbors.find(cell);
   if (ci != cellFaceNeighbors.end())
      std::copy(ci->second.begin(), ci->second.end(), std::back_inserter(cellFaces));

   std::vector<size_t>::const_iterator i;
   for (i = faceIndices.begin(); i != faceIndices.end(); i ++) {

      std::vector<size_t>::const_iterator f;
      for (f = cellFaces.begin(); f != cellFaces.end(); f ++) {

         if (inFace(vertex, *i, faces[*f])) {
            if (shared.find(*i) == shared.end())
               shared.insert(*i);
            else
               return *i;
         }
      }
   }

   return -1;
}

size_t getOppositeFace(size_t cell, size_t face,
              const std::vector<std::vector<size_t> > & faces,
              const std::map<size_t, std::vector<size_t> > & cellFaceMapping,
              const std::map<size_t, std::vector<size_t> > & cellFaceNeighbors,
                     std::vector<size_t> & faceIndices,
                     std::vector<size_t> & oppositeFaceIndices) {

   std::vector<size_t> cellFaces;
   std::map<size_t, std::vector<size_t> >::const_iterator ci =
      cellFaceMapping.find(cell);
   if (ci != cellFaceMapping.end())
      std::copy(ci->second.begin(), ci->second.end(), std::back_inserter(cellFaces));
   ci = cellFaceNeighbors.find(cell);
   if (ci != cellFaceNeighbors.end())
      std::copy(ci->second.begin(), ci->second.end(), std::back_inserter(cellFaces));

   std::vector<size_t>::const_iterator i;
   for (i = cellFaces.begin(); i != cellFaces.end(); i ++) {

      int num = compareFaces(faceIndices, faces[*i]);
      if (num == 0) {
         for (size_t index = 0; index < faceIndices.size(); index ++) {
            size_t v = getOppositeVertexInFace(faceIndices[index], cell, faces,
                                               faces[*i], cellFaceMapping,
                                               cellFaceNeighbors);
            oppositeFaceIndices.push_back(v);
         }
         return *i;
      }
   }

   return -1;
}

void ReadFOAM::parseBoundary(const std::string & casedir, const int partition) {

   std::stringstream name;
   name << casedir << "/processor" << partition
        << "/constant/polyMesh/boundary";

   std::ifstream file(name.str().c_str(),
                      std::ios_base::in | std::ios_base::binary);

   bi::filtering_istream in;
   in.push(file);

   typedef std::istreambuf_iterator<char> base_iterator_type;
   typedef bs::multi_pass<base_iterator_type> forward_iterator_type;
   typedef classic::position_iterator2<forward_iterator_type> pos_iterator_type;
   forward_iterator_type fwd_begin =
      bs::make_default_multi_pass(base_iterator_type(in));
   forward_iterator_type fwd_end;
   pos_iterator_type pos_begin(fwd_begin, fwd_end, name.str());
   pos_iterator_type pos_end;

   struct skipper<pos_iterator_type> skipper;

   BoundaryParser<pos_iterator_type> p;
   std::map<std::string, std::map<std::string, std::string> > boundaries;

   bool r = qi::phrase_parse(pos_begin, pos_end,
                             p, skipper, boundaries);

   std::cout << "r: " << r << std::endl;

   std::map<std::string, std::map<std::string, std::string> >::iterator top;
   for (top = boundaries.begin(); top != boundaries.end(); top ++) {

      std::cout << top->first << ":" << std::endl;
      std::map<std::string, std::string>::iterator i;
      for (i = top->second.begin(); i != top->second.end(); i ++)
         std::cout << "    " << i->first << " => " << i->second << std::endl;
   }
}

std::vector<std::pair<std::string, vistle::Object *> >
ReadFOAM::load(const std::string & casedir, const size_t partition) {

   std::vector<std::pair<std::string, vistle::Object *> > objects;

   std::stringstream pointsName;
   pointsName << casedir << "/processor" << partition << "/0.3238435/polyMesh/points.gz";
   std::ifstream pointsFile(pointsName.str().c_str(),
                            std::ios_base::in | std::ios_base::binary);

   std::stringstream facesName;
   facesName << casedir << "/processor" << partition << "/constant/polyMesh/faces.gz";
   std::ifstream facesFile(facesName.str().c_str(),
                           std::ios_base::in | std::ios_base::binary);

   std::stringstream ownersName;
   ownersName << casedir << "/processor" << partition << "/constant/polyMesh/owner.gz";
   std::ifstream ownersFile(ownersName.str().c_str(),
                            std::ios_base::in | std::ios_base::binary);

   std::stringstream neighborsName;
   neighborsName << casedir << "/processor" << partition << "/constant/polyMesh/neighbour.gz";
   std::ifstream neighborsFile(neighborsName.str().c_str(),
                               std::ios_base::in | std::ios_base::binary);

   std::stringstream pressureName;
   pressureName << casedir << "/processor" << partition << "/0.3238435/p.gz";
   std::ifstream pressureFile(pressureName.str().c_str(),
                              std::ios_base::in | std::ios_base::binary);

   bi::filtering_istream pointsIn;
   pointsIn.push(bi::gzip_decompressor());
   pointsIn.push(pointsFile);

   bi::filtering_istream facesIn;
   facesIn.push(bi::gzip_decompressor());
   facesIn.push(facesFile);

   bi::filtering_istream ownersIn;
   ownersIn.push(bi::gzip_decompressor());
   ownersIn.push(ownersFile);

   bi::filtering_istream neighborsIn;
   neighborsIn.push(bi::gzip_decompressor());
   neighborsIn.push(neighborsFile);

   bi::filtering_istream pressureIn;
   pressureIn.push(bi::gzip_decompressor());
   pressureIn.push(pressureFile);

   std::vector<vistle::Vector> points;
   std::vector<std::vector<size_t> > faces;
   std::vector<size_t> owners;
   std::vector<size_t> neighbors;

   std::vector<float> pressure;

   typedef std::istreambuf_iterator<char> base_iterator_type;
   typedef bs::multi_pass<base_iterator_type> forward_iterator_type;
   typedef classic::position_iterator2<forward_iterator_type> pos_iterator_type;

   struct skipper<pos_iterator_type> skipper;
   struct PointParser<pos_iterator_type> pointParser;
   struct FaceParser<pos_iterator_type> faceParser;
   struct IntParser<pos_iterator_type> ownersParser;
   struct IntParser<pos_iterator_type> neighborsParser;

   struct ScalarParser<pos_iterator_type> pressureParser;

   try {
      forward_iterator_type fwd_begin =
         bs::make_default_multi_pass(base_iterator_type(pointsIn));
      forward_iterator_type fwd_end;

      pos_iterator_type pos_begin(fwd_begin, fwd_end, pointsName.str());
      pos_iterator_type pos_end;

      bool r = qi::phrase_parse(pos_begin, pos_end,
                                pointParser, skipper, points);
      std::cout << "r: " << r << " points: " << points.size() << std::endl;

      // faces
      fwd_begin =
         bs::make_default_multi_pass(base_iterator_type(facesIn));
      pos_begin = pos_iterator_type(fwd_begin, fwd_end, facesName.str());
      r = qi::phrase_parse(pos_begin, pos_end,
                                faceParser, skipper, faces);
      std::cout << "r: " << r << " faces: " << faces.size() << std::endl;

      // owners
      fwd_begin =
         bs::make_default_multi_pass(base_iterator_type(ownersIn));
      pos_begin = pos_iterator_type(fwd_begin, fwd_end, ownersName.str());
      r = qi::phrase_parse(pos_begin, pos_end,
                           ownersParser, skipper, owners);
      std::cout << "r: " << r << " owners: " << owners.size() << std::endl;

      // neighbors
      fwd_begin =
         bs::make_default_multi_pass(base_iterator_type(neighborsIn));
      pos_begin = pos_iterator_type(fwd_begin, fwd_end, neighborsName.str());
      r = qi::phrase_parse(pos_begin, pos_end,
                           neighborsParser, skipper, neighbors);
      std::cout << "r: " << r << " neighbors: " << neighbors.size() << std::endl;

      // pressure
      fwd_begin =
         bs::make_default_multi_pass(base_iterator_type(pressureIn));
      pos_begin = pos_iterator_type(fwd_begin, fwd_end, pressureName.str());
      r = qi::phrase_parse(pos_begin, pos_end, pressureParser, skipper, pressure);
      std::cout << "r: " << r << " pressure: " << pressure.size() << std::endl;


      std::map<size_t, std::vector<size_t> > cellFaceMapping;
      for (size_t face = 0; face < owners.size(); face ++) {

         std::map<size_t, std::vector<size_t> >::iterator i =
            cellFaceMapping.find(owners[face]);
         if (i == cellFaceMapping.end()) {
            std::vector<size_t> a;
            a.push_back(face);
            cellFaceMapping[owners[face]] = a;
         } else
            i->second.push_back(face);
      }

      std::map<size_t, std::vector<size_t> > cellFaceNeighbors;
      for (size_t face = 0; face < neighbors.size(); face ++) {

         std::map<size_t, std::vector<size_t> >::iterator i =
            cellFaceNeighbors.find(neighbors[face]);
         if (i == cellFaceNeighbors.end()) {
            std::vector<size_t> a;
            a.push_back(face);
            cellFaceNeighbors[neighbors[face]] = a;
         } else
            i->second.push_back(face);
      }

      vistle::UnstructuredGrid *usg = vistle::UnstructuredGrid::create();
      usg->setBlock(partition);
      usg->setTimestep(0);

      vistle::Vec<float> *pres = vistle::Vec<float>::create(points.size());
      pres->setBlock(partition);
      pres->setTimestep(0);

      for (size_t p = 0; p < points.size(); p ++) {

         usg->x->push_back(points[p].x);
         usg->y->push_back(points[p].y);
         usg->z->push_back(points[p].z);
      }

      float *vertexPressure = new float[points.size()]();
      char *numVertexPressure = new char[points.size()]();

      for (size_t cell = 0; cell < cellFaceMapping.size(); cell ++) {

         std::vector<size_t> faceIndices;
         std::vector<size_t> oppositeFaceIndices;
         size_t face = getFirstFace(cell, faces, cellFaceMapping,
                                    cellFaceNeighbors, faceIndices);

         getOppositeFace(cell, face, faces, cellFaceMapping,
                         cellFaceNeighbors, faceIndices,
                         oppositeFaceIndices);

         usg->tl->push_back(vistle::UnstructuredGrid::HEXAHEDRON);
         usg->el->push_back(usg->cl->size());
         for (size_t index = 0; index < faceIndices.size(); index ++) {
            usg->cl->push_back(faceIndices[index]);
            vertexPressure[faceIndices[index]] += pressure[cell];
            numVertexPressure[faceIndices[index]]++;
         }
         for (size_t index = 0; index < oppositeFaceIndices.size(); index ++) {
            usg->cl->push_back(oppositeFaceIndices[index]);
            vertexPressure[oppositeFaceIndices[index]] += pressure[cell];
            numVertexPressure[oppositeFaceIndices[index]]++;
         }
      }

      float *pressureField = &((*pres->x)[0]);
      for (size_t p = 0; p < points.size(); p ++)
         if (numVertexPressure[p] > 0)
            pressureField[p] = vertexPressure[p] / numVertexPressure[p];

      objects.push_back(std::make_pair("grid_out", usg));
      objects.push_back(std::make_pair("p_out", pres));
      return objects;

   } catch (const qi::expectation_failure<pos_iterator_type>& e) {

      const classic::file_position_base<std::string>& pos =
         e.first.get_position();
      std::cout <<
         "parse error at file " << pos.file <<
         " line " << pos.line << " column " << pos.column << std::endl <<
         "'" << e.first.get_currentline() << "'" << std::endl <<
         std::setw(pos.column) << " " << "^- here" << std::endl;
   }

   return objects;
}

bool ReadFOAM::compute() {

   for (int partition = 0; partition < 32; partition ++) {
      parseBoundary(getFileParameter("filename"), partition);
   }

   for (int partition = 0; partition < 32; partition ++) {
      if (partition % size == rank) {

         std::vector<std::pair<std::string, vistle::Object *> > objects =
            load(getFileParameter("filename"), partition);

         std::vector<std::pair<std::string, vistle::Object *> >::iterator i;
         for (i = objects.begin(); i != objects.end(); i ++)
            addObject(i->first, i->second);
      }
   }

   return true;
}
