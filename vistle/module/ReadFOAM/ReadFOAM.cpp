/**************************************************************************\
 **                                                           (C)2013 RUS  **
 **                                                                        **
 ** Description: Read FOAM data format                                     **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** History:                                                               **
 ** May   13	    C.Kopf  	    V1.0                                   **
 *\**************************************************************************/

#include "ReadFOAM.h"
#include <core/unstr.h>
#include <core/vec.h>

//Includes copied from vistle ReadFOAM.cpp
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <set>
#include <cctype>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_no_skip.hpp>
#include <boost/spirit/include/support_istream_iterator.hpp>
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

#include <boost/filesystem.hpp>

const size_t MaxHeaderLines = 1000;

namespace bi = boost::iostreams;
namespace bs = boost::spirit;
namespace bf = boost::filesystem;
namespace classic = boost::spirit::classic;

template<typename Alloc = std::allocator<char> >
struct basic_gzip_decompressor;
typedef basic_gzip_decompressor<> gzip_decompressor;

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;


using namespace vistle;

struct HeaderInfo
{
    std::string version;
    std::string format;
    std::string fieldclass;
    std::string location;
    std::string object;
    std::string dimensions;
    std::string internalField;
    index_t lines = 0;
    bool valid;

};

BOOST_FUSION_ADAPT_STRUCT(
    HeaderInfo,
    (std::string, version)
    (std::string, format)
    (std::string, fieldclass)
    (std::string, location)
    (std::string, object)
    (std::string, dimensions)
    (std::string, internalField)
    (index_t, lines)
)

ReadFOAM::ReadFOAM(const std::string &shmname, int rank, int size, int moduleId)
: Module("ReadFoam", shmname, rank, size, moduleId)
{
   // file browser parameter
   m_casedir = addStringParameter("casedir", "OpenFOAM case directory", "");
   m_casedir->setValue("/data/OpenFOAM/PumpTurbine/transient/");

   m_starttime = addFloatParameter("starttime", "start reading at the first step after this time", 0.);
   m_stoptime = addFloatParameter("stoptime", "stop reading at the last step before this time",
         std::numeric_limits<double>::max());
   m_timeskip = addIntParameter("timeskip", "number of timesteps to skip", 0);

   // the output ports
   m_gridOut = createOutputPort("grid_out");
   m_boundOut = createOutputPort("grid_out1");

   for (int i=0; i<NumPorts; ++i) {
      {
         std::stringstream s;
         s << "data_out" << i;
         m_dataOut.push_back(createOutputPort(s.str()));
      }
      {
         std::stringstream s;
         s << "fieldname" << i;
         m_fieldOut.push_back(addStringParameter(s.str(), "name of field", "NONE"));
      }
   }
}


ReadFOAM::~ReadFOAM()       //Destructor
{
}
//Boost Spirit parser definitions


class FilteringStreamDeleter {
 public:
   FilteringStreamDeleter(bi::filtering_istream *f, std::ifstream *s)
      : filtered(f)
      , stream(s)
      {}

   void operator()(std::istream *f) {
      assert(static_cast<bi::filtering_istream *>(f) == filtered);
      delete filtered;
      delete stream;
   }

   bi::filtering_istream *filtered;
   std::ifstream *stream;
};

boost::shared_ptr<std::istream> getStreamForFile(const std::string &filename) {

   std::ifstream *s = new std::ifstream(filename, std::ios_base::in | std::ios_base::binary);
   if (!s->is_open()) {
      std::cerr << "failed to open " << filename << std::endl;
      return boost::shared_ptr<std::istream>();
   }

   bi::filtering_istream *fi = new bi::filtering_istream;
   bf::path p(filename);
   if (p.extension().string() == ".gz") {
      fi->push(bi::gzip_decompressor());
   }
   fi->push(*s);
   return boost::shared_ptr<std::istream>(fi, FilteringStreamDeleter(fi, s));
}

boost::shared_ptr<std::istream> getStreamForFile(const std::string &dir, const std::string &basename) {

   bf::path zipped(dir + "/" + basename + ".gz");
   if (bf::exists(zipped) && !bf::is_directory(zipped))
      return getStreamForFile(zipped.string());
   else
      return getStreamForFile(dir + "/" + basename);
}

bool isTimeDir(const std::string &dir) {

   //std::cerr << "checking timedir: " << dir << std::endl;

   if (dir == ".")
      return false;
#if 1
   if (dir == "0")
      return false;
#endif

   int numdots = 0;
   for (size_t i=0; i<dir.length(); ++i) {
      if (dir[i] == '.') {
         ++numdots;
         if (numdots > 1)
            return false;
      } else if (!isdigit(dir[i]))
         return false;
   }

   return true;
}

bool isProcessorDir(const std::string &dir) {

   if (dir.find("processor") != 0) {

      return false;
   }

   for (size_t i=strlen("processor"); i<dir.length(); ++i) {

      if (!isdigit(dir[i]))
         return false;
   }

   return true;
}

bool ReadFOAM::checkMeshDirectory(const std::string &meshdir, bool time) {

   //std::cerr << "checking meshdir " << meshdir << std::endl;
   bf::path p(meshdir);
   if (!bf::is_directory(p)) {
      std::cerr << meshdir << " is not a directory" << std::endl;
      return false;
   }

   bool havePoints = false;
   std::map<std::string, std::string> meshfiles;
   for (auto it = bf::directory_iterator(p);
         it != bf::directory_iterator();
         ++it) {
      bf::path ent(*it);
      std::string stem = ent.stem().string();
      std::string ext = ent.extension().string();
      if (stem == "points" || stem == "faces" || stem == "owner" || stem == "neighbour") {
         if (bf::is_directory(*it) || (!ext.empty() && ext != ".gz")) {
            std::cerr << "ignoring " << *it << std::endl;
         } else {
            meshfiles[stem] = bf::path(*it).string();
            if (stem == "points")
               havePoints = true;
         }
      }
   }

   if (meshfiles.size() == 4) {
      if (time) {
         m_varyingGrid = true;
      }
      return true;
   }
   if (meshfiles.size() == 1 && time && havePoints) {
      m_varyingCoords = true;
      return true;
   }

   std::cerr << "did not find all of points, faces, owner and neighbour files" << std::endl;
   return false;
}

bool ReadFOAM::checkSubDirectory(const std::string &timedir, bool time) {

   bf::path dir(timedir);
   if (!bf::exists(dir)) {
      std::cerr << "timestep directory " << timedir << " does not exist" << std::endl;
      return false;
   }
   if (!bf::is_directory(timedir)) {
      std::cerr << "timestep directory " << timedir << " is not a directory" << std::endl;
      return false;
   }

   for (auto it = bf::directory_iterator(dir);
         it != bf::directory_iterator();
         ++it) {
      bf::path p(*it);
      if (bf::is_directory(*it)) {
         std::string name = p.filename().string();
         if (name == "polyMesh") {
            if (!checkMeshDirectory(p.string(), time))
               return false;
         }
      } else {
         std::string stem = p.stem().string();
         ++m_fieldnames[stem];
         if (time)
            ++m_varyingFields[stem];
         else
            ++m_constantFields[stem];
      }
   }


   return true;
}

bool ReadFOAM::checkCaseDirectory(const std::string &casedir, bool compare) {

   if (!compare) {
      m_timedirs.clear();
      m_varyingFields.clear();
      m_constantFields.clear();
      m_fieldnames.clear();
      m_meshfiles.clear();
      m_varyingGrid = false;
      m_varyingCoords = false;
   }

   std::cerr << "reading casedir: " << casedir << std::endl;

   bf::path dir(casedir);
   if (!bf::exists(dir)) {
      std::cerr << "case directory " << casedir << " does not exist" << std::endl;
      return false;
   }
   if (!bf::is_directory(casedir)) {
      std::cerr << "case directory " << casedir << " is not a directory" << std::endl;
      return false;
   }

   int num_processors = 0;
   for (auto it = bf::directory_iterator(dir);
         it != bf::directory_iterator();
         ++it) {
      if (bf::is_directory(*it)) {
         if (isProcessorDir(bf::basename(it->path())))
            ++num_processors;
      }
   }

   if (!compare && num_processors>0) {
      m_numprocessors = num_processors;
   }

   if (compare && num_processors > 0) {
      std::cerr << "found processor subdirectory in processor directory" << std::endl;
      return false;
   }
   
   if (num_processors > 0) {
      bool result = checkCaseDirectory(casedir+"/processor0");
      if (!result) {
         std::cerr << "failed to read case directory for processor 0" << std::endl;
         return false;
      }

      for (int i=1; i<num_processors; ++i) {
         std::stringstream s;
         s << casedir << "/processor" << i;
         bool result = checkCaseDirectory(s.str(), true);
         if (!result)
            return false;
      }

      return true;
   }

   int num_timesteps = 0;
   for (auto it = bf::directory_iterator(dir);
         it != bf::directory_iterator();
         ++it) {
      if (bf::is_directory(*it)) {
         std::string bn = it->path().filename().string();
         if (isTimeDir(bn)) {
            double t = atof(bn.c_str());
            if (t >= m_starttime->getValue() && t <= m_stoptime->getValue()) {
               ++num_timesteps;
               if (compare) {
                  if (m_timedirs.find(t) == m_timedirs.end()) {
                     std::cerr << "timestep " << bn << " not available on all processors" << std::endl;
                     return false;
                  }
               } else {
                  m_timedirs[t]=bn;
               }
            }
         }
      }
   }

   bool first = true;
   for (auto it = bf::directory_iterator(dir);
         it != bf::directory_iterator();
         ++it) {
      if (bf::is_directory(*it)) {
         std::string bn = it->path().filename().string();
         if (isTimeDir(bn)) {
            bool result = checkSubDirectory(it->path().string(), true);
            if (!result)
               return false;
            first = false;
         } else if (bn == "constant") {
            bool result = checkSubDirectory(it->path().string(), false);
            if (!result)
               return false;
         }
      }
   }

   if (compare && num_timesteps != m_timedirs.size()) {
      std::cerr << "not all timesteps available on all processors" << std::endl;
      return false;
   }

   if (!compare) {
      std::cerr << " " << "casedir: " << casedir << " " << std::endl
         << "Number of processors: " << num_processors << std::endl
         << "Number of time directories found: " << m_timedirs.size()  << std::endl
         << "Number of fields: " << m_fieldnames.size() << std::endl;
         //<< "Meshfiles found: " << meshfiles.size()<< std::endl;

      int np = num_processors > 0 ? num_processors : 1;
      for (auto field: m_fieldnames) {
         std::cerr << "   " << field.first << ": " << field.second;
         if (np * m_timedirs.size() != field.second) {
            m_fieldnames.erase(field.first);
         }
      }
      std::cerr << std::endl;
   }

   return true;
}

std::string getFoamHeader(std::istream &stream)
{
   std::string header;
   for (size_t i=0; i<MaxHeaderLines; ++i) {
      std::string line;
      std::getline(stream, line);
      if (line == "(")
         break;
      header.append(line);
      header.append("\n");
   }
   return header;
}

//Skipper for skipping FOAM Headers and unimportant data
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

//Skipper for skipping FOAM Headers and unimportant data
template <typename Iterator>
struct headerSkipper: qi::grammar<Iterator> {

    headerSkipper(): headerSkipper::base_type(start) {

        start =
            ascii::space
            | "/*" >> *(ascii::char_ - "*/") >> "*/"
            | "//" >> *(ascii::char_ - qi::eol) >> qi::eol
            | "FoamFile" >> *ascii::space >> '{' >> *(ascii::char_ - qi::eol) >> qi::eol
            | "note" >> *(ascii::char_ - qi::eol) >> qi::eol
            | '}'
            ;
    }

qi::rule<Iterator> start;
};

template <typename Iterator>
struct headerParser: qi::grammar<Iterator, HeaderInfo(), headerSkipper<Iterator> > {

    headerParser(): headerParser::base_type(start) {

        start =
            version
            >> format
            >> fieldclass
            >> location
            >> object
            >> -dimensions
            >> -internalField
            >> lines
        ;

        version = "version" >> +(ascii::char_ - ';') >> ';'
        ;
        format = "format" >> +(ascii::char_ - ';') >> ';'
        ;
        fieldclass = "class" >> +(ascii::char_ - ';') >> ';'
        ;
        location = "location" >> qi::lit('"') >> +(ascii::char_ - '"') >> '"' >> ';'
        ;
        object = "object" >> +(ascii::char_ - ';') >> ';'
        ;
        dimensions = "dimensions" >> qi::lexeme['[' >> +(ascii::char_ - ']') >>']' >> ';']
        ;
        internalField ="internalField" >> qi::lexeme[+(ascii::char_ - qi::eol) >> qi::eol]
        ;
        lines = qi::int_
        ;
    }

    qi::rule<Iterator, HeaderInfo(), headerSkipper<Iterator> > start;
    qi::rule<Iterator, std::string(), headerSkipper<Iterator> > version;
    qi::rule<Iterator, std::string(), headerSkipper<Iterator> > format;
    qi::rule<Iterator, std::string(), headerSkipper<Iterator> > fieldclass;
    qi::rule<Iterator, std::string(), headerSkipper<Iterator> > location;
    qi::rule<Iterator, std::string(), headerSkipper<Iterator> > object;
    qi::rule<Iterator, std::string(), headerSkipper<Iterator> > dimensions;
    qi::rule<Iterator, std::string(), headerSkipper<Iterator> > internalField;
    qi::rule<Iterator, int(), headerSkipper<Iterator> > lines;
};

HeaderInfo readFoamHeader(std::istream &stream)
{
    struct headerParser<std::string::iterator> headerParser;
    struct headerSkipper<std::string::iterator> headerSkipper;

    std::string header = getFoamHeader(stream);
    HeaderInfo info;

    info.valid = qi::phrase_parse(header.begin(),header.end(),
                             headerParser, headerSkipper, info);

    if (!info.valid) {
       std::cerr << "parsing FOAM header failed" << std::endl;

       std::cerr << "================================================" << std::endl;
       std::cerr << header << std::endl;
       std::cerr << "================================================" << std::endl;
    }

    return info;
}


//Skipper for skipping everything but the dimensions in the owners file Header
template <typename Iterator>
struct dimSkipper: qi::grammar<Iterator> {

   dimSkipper(): dimSkipper::base_type(start) {

      start =
         ascii::space
         | "/*" >> *(ascii::char_ - "*/") >> "*/"
         | "//" >> *(ascii::char_ - qi::eol) >> qi::eol
         | "version" >> *(ascii::char_ - qi::eol) >> qi::eol
         | "location" >> *(ascii::char_)
         | ~ascii::char_("0-9")
         ;
   }

   qi::rule<Iterator> start;
};

//Parser that Reads the size of the domain (nCells, nPoints, nFaces, nInternalFaces) from the owners header (use together with dimSkipper)
template <typename Iterator>
struct dimParser: qi::grammar<Iterator, std::vector<index_t>(),
                               dimSkipper<Iterator> > {

   dimParser(): dimParser::base_type(start) {

       start = term >> term >> term >> term
         ;

      term = qi::int_
         ;
   }

   qi::rule<Iterator, std::vector<index_t>(), dimSkipper<Iterator> > start;
   qi::rule<Iterator, index_t(),dimSkipper<Iterator> > term;
};

class Boundary {
 public:
   Boundary(const std::string &name, const Index s, const Index num, const std::string &t)
      : name(name)
      , startFace(s)
      , numFaces(num)
      , type(t)
      {
      }

   const std::string name;
   const Index startFace;
   const Index numFaces;
   const std::string type;
};


class Boundaries {
 public:
   Boundaries() {}

   bool isBoundaryFace(const Index face) {

      for (auto i = boundaries.begin(); i != boundaries.end(); i ++) {
         if (i->type == "processor")
            continue;
         if (face >= i->startFace && face < i->startFace + i->numFaces)
            return true;
      }
      return false;
   }

   bool isProcessorBoundaryFace(const Index face) {

      for (auto i = boundaries.begin(); i != boundaries.end(); i ++) {
         if (i->type != "processor")
            continue;
         if (face >= i->startFace && face < i->startFace + i->numFaces)
            return true;
      }
      return false;
   }

   void addBoundary(const Boundary &b) {

      boundaries.push_back(b);
   }

 private:
   std::vector<Boundary> boundaries;
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



Boundaries loadBoundary(const std::string &meshdir) {

   auto stream = getStreamForFile(meshdir, "boundary");

   typedef std::istreambuf_iterator<char> base_iterator_type;
   typedef bs::multi_pass<base_iterator_type> forward_iterator_type;
   typedef classic::position_iterator2<forward_iterator_type> pos_iterator_type;
   forward_iterator_type fwd_begin =
      bs::make_default_multi_pass(base_iterator_type(*stream));
   forward_iterator_type fwd_end;
   pos_iterator_type pos_begin(fwd_begin, fwd_end, meshdir + "/boundary");
   pos_iterator_type pos_end;

   struct skipper<pos_iterator_type> skipper;

   BoundaryParser<pos_iterator_type> p;
   std::map<std::string, std::map<std::string, std::string> > boundaries;

   bool r = qi::phrase_parse(pos_begin, pos_end,
         p, skipper, boundaries);

   //std::cout << "r: " << r << std::endl;

   Boundaries bounds;

   std::map<std::string, std::map<std::string, std::string> >::iterator top;
   for (top = boundaries.begin(); top != boundaries.end(); top ++) {

      std::string name = top->first;
#if 0
      std::cout << name << ":" << std::endl;
      std::map<std::string, std::string>::iterator i;
      for (i = top->second.begin(); i != top->second.end(); i ++) {
         std::cout << "    " << i->first << " => " << i->second << std::endl;
      }
#endif
      const auto &cur = top->second;
      auto nFaces = cur.find("nFaces");
      auto startFace = cur.find("startFace");
      auto type = cur.find("type");
      if (nFaces != cur.end() && startFace != cur.end() && type != cur.end()) {
         std::string t = type->second;
         Index n = atol(nFaces->second.c_str());
         Index s = atol(startFace->second.c_str());
         bounds.addBoundary(Boundary(name, s, n, t));
      }
   }

   return bounds;
}

template<typename T>
std::istream &operator>>(std::istream &stream, std::vector<T> &vec) {

   size_t n;
   stream >> n;
   stream.ignore(std::numeric_limits<std::streamsize>::max(),'(');
   vec.reserve(n);
   for (size_t i=0; i<n; ++i) {
      T val;
      stream >> val;
      vec.push_back(val);
   }
   stream.ignore(std::numeric_limits<std::streamsize>::max(),')');
   return stream;
}

template<typename T>
bool readVectorArray(std::istream &stream, T *x, T *y, T *z, const size_t lines) {

   for (size_t i=0; i<lines; ++i) {
      stream.ignore(std::numeric_limits<std::streamsize>::max(), '(');
      stream >> x[i] >> y[i] >> z[i];
      stream.ignore(std::numeric_limits<std::streamsize>::max(), ')');
   }

   return true;
}

template<typename T>
bool readArray(std::istream &stream, T *p, const size_t lines) {

   for (size_t i=0; i<lines; ++i) {
      stream >> p[i];
   }

   return true;
}

bool readIndexArray(std::istream &stream, Index *p, const size_t lines) {
   return readArray<Index>(stream, p, lines);
}

bool readIndexListArray(std::istream &stream, std::vector<Index> *p, const size_t lines) {
   return readArray<std::vector<Index>>(stream, p, lines);
}

struct DimensionInfo {
   Index points = 0;
   Index cells = 0;
   Index faces = 0;
   Index internalFaces = 0;
};

DimensionInfo readDimensions(const std::string &meshdir) {

   struct dimParser<std::string::iterator> dimParser;
   struct dimSkipper<std::string::iterator> dimSkipper;
   boost::shared_ptr<std::istream> fileIn = getStreamForFile(meshdir, "owner");
   if (!fileIn) {
      std::cerr << "failed to open " << meshdir + "/polyMesh/owner for reading dimensions" << std::endl;
   }
   std::string header = getFoamHeader(*fileIn);

   std::vector<Index> dims;
   bool r = qi::phrase_parse(header.begin(), header.end(), dimParser, dimSkipper, dims);
   std::cerr << " " << "Dimension parsing successful: " << r << "  ## dimensions loaded: " << dims.size()<< " of 4" << std::endl;
   DimensionInfo info;
   info.points = dims[0];
   info.cells = dims[1];
   info.faces = dims[2];
   info.internalFaces = dims[3];
   return info;
}

Index findVertexAlongEdge(const index_t & point,
      const index_t & homeface,
      const std::vector<index_t> & cellfaces,
      const std::vector<std::vector<index_t> > & faces) {

   std::vector<index_t> pointfaces;
   for (index_t i=0;i<cellfaces.size();i++) {
      if(cellfaces[i]!=homeface) {
         const std::vector<index_t> &face=faces[cellfaces[i]];
         for (index_t j=0;j<face.size(); j++) {
            if (face[j]==point) {
               pointfaces.push_back(cellfaces[i]);
               break;
            }
         }
      }
   }
   const std::vector<index_t> &a=faces[pointfaces[0]];
   const std::vector<index_t> &b=faces[pointfaces[1]];
   for (index_t i=0;i<a.size();i++) {
      for (index_t j=0;j<b.size();j++) {
         if(a[i]==b[j] && a[i]!=point) {
            return a[i];
         }
      }
   }

   return -1;
}

bool isPointingInwards(index_t &face,
      index_t &cell,
      index_t &ninternalFaces,
      std::vector<index_t> &owners,
      std::vector<index_t> &neighbors) {

   //check if the normal vector of the cell is pointing inwards
   //(in openFOAM it always points into the cell with the higher index)
   if (face>=ninternalFaces) {//if ia is bigger than the number of internal faces
      return true;                    //then a is a boundary face and normal vector goes inwards by default
   } else {
      index_t j,o,n;
      o=owners[face];
      n=neighbors[face];
      if (o==cell) {j=n;}
      else {j=o;} //now i is the index of current cell and j is index of other cell sharing the same face
      if (cell>j) {return true;} //if index of active cell is higher than index of "next door" cell
      else {return false;}    //then normal vector points inwards else outwards
   }
}

std::vector<index_t> getVerticesForCell(const std::vector<index_t> &cellfaces,
      const std::vector<std::vector<index_t>> &faces) {

   std::vector<index_t> cellvertices;
   for(Index i=0;i<cellfaces.size();i++) {
      for(Index j=0;j<faces[cellfaces[i]].size();j++) {
         cellvertices.push_back(faces[cellfaces[i]][j]);
      }
   }
   std::sort(cellvertices.begin(),cellvertices.end());
   cellvertices.erase(std::unique(cellvertices.begin(), cellvertices.end()), cellvertices.end());
   return cellvertices;
}

bool loadCoords(const std::string &meshdir, UnstructuredGrid::ptr grid) {

   boost::shared_ptr<std::istream> pointsIn = getStreamForFile(meshdir, "points");
   HeaderInfo pointsH = readFoamHeader(*pointsIn);
   grid->setSize(pointsH.lines);
   readVectorArray(*pointsIn, grid->x().data(), grid->y().data(), grid->z().data(), pointsH.lines);

   return true;
}


std::pair<UnstructuredGrid::ptr, Polygons::ptr> ReadFOAM::loadGrid(const std::string &meshdir) {

   Boundaries boundaries = loadBoundary(meshdir);

   DimensionInfo dim = readDimensions(meshdir);
   UnstructuredGrid::ptr grid(new UnstructuredGrid(dim.cells, 0, 0));

   Polygons::ptr poly(new Polygons(0, 0, 0));

   {
      boost::shared_ptr<std::istream> ownersIn = getStreamForFile(meshdir, "owner");
      HeaderInfo ownerH = readFoamHeader(*ownersIn);
      std::vector<Index> owners(ownerH.lines);
      readIndexArray(*ownersIn, owners.data(), owners.size());

      boost::shared_ptr<std::istream> facesIn = getStreamForFile(meshdir, "faces");
      HeaderInfo facesH = readFoamHeader(*facesIn);
      std::vector<std::vector<Index>> faces(facesH.lines);
      readIndexListArray(*facesIn, faces.data(), faces.size());

      boost::shared_ptr<std::istream> neighborsIn = getStreamForFile(meshdir, "neighbour");
      HeaderInfo neighbourH = readFoamHeader(*neighborsIn);
      if (neighbourH.lines != dim.internalFaces) {
         std::cerr << "inconsistency: #internalFaces != #neighbours" << std::endl;
      }
      std::vector<Index> neighbour(neighbourH.lines);
      readIndexArray(*neighborsIn, neighbour.data(), neighbour.size());



      //Load mesh dimensions
      std::cerr << "#points: " << dim.points << ", "
         << "#cells: " << dim.cells << ", "
         << "#faces: " << dim.faces << ", "
         << "#internal faces: " << dim.internalFaces << std::endl;

      auto &polys = poly->el();
      auto &conn = poly->cl();
      Index num_bound = dim.faces - dim.internalFaces;
      polys.reserve(num_bound);
      for (Index i=dim.internalFaces; i<dim.faces; ++i) {
         if (!boundaries.isProcessorBoundaryFace(i)) {
            polys.push_back(conn.size());
            auto &face = faces[i];
            for (int j=0; j<face.size(); ++j) {
               conn.push_back(face[j]);
            }
         }
      }

      //Create CellFaceMap
      //std::cerr << " " << "Creating cell to face Map ... " << std::flush;
      std::vector<std::vector<Index>> cellfacemap(dim.cells);
      for (Index face = 0; face < owners.size(); ++face) {
         cellfacemap[owners[face]].push_back(face);
      }
      for (Index face = 0; face < neighbour.size(); ++face) {
         cellfacemap[neighbour[face]].push_back(face);
      }
      //std::cerr << "done! ## size: " << cellfacemap.size() << std::endl;

      //std::cerr << "Setting Typelist and adding connectivities ... " << std::flush;

      auto types = grid->tl().data();

      Index num_conn = 0;
      Index num_hex=0, num_tet=0, num_prism=0, num_pyr=0, num_poly=0;
      for (index_t i=0; i<dim.cells; i++) {
         const std::vector<Index> &cellfaces=cellfacemap[i];
         const std::vector<index_t> cellvertices = getVerticesForCell(cellfaces, faces);
         bool onlyViableFaces=true;
         for (index_t j=0; j<cellfaces.size(); ++j) {
            if (faces[cellfaces[j]].size()<3 || faces[cellfaces[j]].size()>4) {
               onlyViableFaces=false;
               break;
            }
         }
         const Index num_faces = cellfaces.size();
         Index num_verts = cellvertices.size();
         if (num_faces==6 && num_verts==8 && onlyViableFaces) {
            types[i]=UnstructuredGrid::HEXAHEDRON;
            ++num_hex;
         } else if (num_faces==5 && num_verts==6 && onlyViableFaces) {
            types[i]=UnstructuredGrid::PRISM;
            ++num_prism;
         } else if (num_faces==5 && num_verts==5 && onlyViableFaces) {
            types[i]=UnstructuredGrid::PYRAMID;
            ++num_pyr;
         } else if (num_faces==4 && num_verts==4 && onlyViableFaces) {
            types[i]=UnstructuredGrid::TETRAHEDRON;
            ++num_tet;
         } else {
            ++num_poly;
            types[i]=UnstructuredGrid::POLYHEDRON;
            num_verts=0;
            for (Index j=0; j<cellfaces.size(); ++j) {
               num_verts += faces[cellfaces[j]].size() + 1;
            }
         }
         num_conn += num_verts;
      }
      //std::cerr << "done!" << std::endl;

      std::cerr << "#hexa: " << num_hex << ", "
         << "#prism: " << num_prism << ", "
         << "#pyramid: " << num_pyr << ", "
         << "#tetra: " << num_tet << ", "
         << "#poly: " << num_poly << std::endl;

      //std::cerr << " " << "Connectivities: "<<num_conn << std::endl;

      // save data cell by cell to element, connectivity and type list
      //std::cerr << " " << "Saving elements, connectivites and types to covise object ..."<<std::flush;

      //go cell by cell (element by element)
      auto el = grid->el().data();
      auto &connectivities = grid->cl();
      auto inserter = std::back_inserter(connectivities);
      connectivities.reserve(num_conn);
      for(index_t i=0;  i<dim.cells; i++) {
         //std::cerr << i << std::endl;
         //element list
         el[i] = connectivities.size();
         //connectivity list
         const auto &cellfaces=cellfacemap[i];//get all faces of current cell
         switch (types[i]) {
            case UnstructuredGrid::HEXAHEDRON: {
               index_t ia=cellfaces[0];//Pick the first index in the vector as index of the random starting face
               std::vector<index_t> a=faces[ia];//find face that corresponds to index ia

               if (!isPointingInwards(ia,i,dim.internalFaces,owners,neighbour)) {
                  std::reverse(a.begin(), a.end());
               }

               std::copy(a.begin(), a.end(), inserter);
               connectivities.push_back(findVertexAlongEdge(a[0],ia,cellfaces,faces));
               connectivities.push_back(findVertexAlongEdge(a[1],ia,cellfaces,faces));
               connectivities.push_back(findVertexAlongEdge(a[2],ia,cellfaces,faces));
               connectivities.push_back(findVertexAlongEdge(a[3],ia,cellfaces,faces));
            }
            break;

            case UnstructuredGrid::PRISM: {
               index_t it=1;
               index_t ia=cellfaces[0];
               while (faces[ia].size()>3) {
                  ia=cellfaces[it++];
               }

               std::vector<index_t> a=faces[ia];

               if(!isPointingInwards(ia,i,dim.internalFaces,owners,neighbour)) {
                  std::reverse(a.begin(), a.end());
               }

               std::copy(a.begin(), a.end(), inserter);
               connectivities.push_back(findVertexAlongEdge(a[0],ia,cellfaces,faces));
               connectivities.push_back(findVertexAlongEdge(a[1],ia,cellfaces,faces));
               connectivities.push_back(findVertexAlongEdge(a[2],ia,cellfaces,faces));
            }
            break;

            case UnstructuredGrid::PYRAMID: {
               index_t it=1;
               index_t ia=cellfaces[0];
               while (faces[ia].size()<4) {
                  ia=cellfaces[it++];
               }

               std::vector<index_t> a=faces[ia];

               if(!isPointingInwards(ia,i,dim.internalFaces,owners,neighbour)) {
                  std::reverse(a.begin(), a.end());
               }

               std::copy(a.begin(), a.end(), inserter);
               connectivities.push_back(findVertexAlongEdge(a[0],ia,cellfaces,faces));
            }
            break;

            case UnstructuredGrid::TETRAHEDRON: {
               index_t ia=cellfaces[0];
               std::vector<index_t> a=faces[ia];

               if(!isPointingInwards(ia,i,dim.internalFaces,owners,neighbour)) {
                  std::reverse(a.begin(), a.end());
               }

               std::copy(a.begin(), a.end(), inserter);
               connectivities.push_back(findVertexAlongEdge(a[0],ia,cellfaces,faces));
            }
            break;

            case UnstructuredGrid::POLYHEDRON: {
               for (index_t j=0;j<cellfaces.size();j++) {
                  index_t ia=cellfaces[j];
                  std::vector<index_t> a=faces[ia];

                  if(!isPointingInwards(ia,i,dim.internalFaces,owners,neighbour)) {
                     std::reverse(a.begin(), a.end());
                  }

                  for (index_t k=0; k<=a.size(); ++k) {
                     connectivities.push_back(a[k % a.size()]);
                  }
               }
            }
            break;
         }
      }
   }

   loadCoords(meshdir, grid);
   poly->d()->x[0] = grid->d()->x[0];
   poly->d()->x[1] = grid->d()->x[1];
   poly->d()->x[2] = grid->d()->x[2];

   std::cerr << " done!" << std::endl;

   return std::make_pair(grid, poly);
}

Object::ptr ReadFOAM::loadField(const std::string &meshdir, const std::string &field) {

   boost::shared_ptr<std::istream> stream = getStreamForFile(meshdir, field);
   HeaderInfo header = readFoamHeader(*stream);
   if (header.fieldclass == "volScalarField") {
      Vec<Scalar>::ptr s(new Vec<Scalar>(header.lines));
      readArray<Scalar>(*stream, s->x().data(), s->x().size());
      return s;
   } else if (header.fieldclass == "volVectorField") {
      Vec<Scalar, 3>::ptr v(new Vec<Scalar, 3>(header.lines));
      readVectorArray(*stream, v->x().data(), v->y().data(), v->z().data(), v->x().size());
      return v;
   }
   
   std::cerr << "cannot interpret " << meshdir << "/" << field << std::endl;
   return Object::ptr();
}

void ReadFOAM::setMeta(Object::ptr obj, int processor, int timestep) const {

   obj->setTimestep(timestep);
   obj->setNumTimesteps(m_timedirs.size());
   obj->setBlock(processor);
   obj->setNumBlocks(m_numprocessors == 0 ? 1 : m_numprocessors);

   if (timestep >= 0) {
      int i = 0;
      for (auto &ts: m_timedirs) {
         if (i == timestep) {
            obj->setRealTime(ts.first);
            break;
         }
         ++i;
      }
   }
}

bool ReadFOAM::loadFields(const std::string &meshdir, const std::map<std::string, int> &fields, int processor, int timestep) {

   for (int i=0; i<NumPorts; ++i) {
      std::string field = m_fieldOut[i]->getValue();
      auto it = fields.find(field);
      if (it == fields.end())
         continue;
      Object::ptr obj = loadField(meshdir, field);
      setMeta(obj, processor, timestep);
      addObject(m_dataOut[i], obj);
   }

   return true;
}




bool ReadFOAM::readDirectory(const std::string &casedir, int processor, int timestep) {

   std::string dir = casedir;

   if (processor >= 0) {
      std::stringstream s;
      s << "/processor" << processor;
      dir += s.str();
   }

   if (timestep < 0) {
      dir += "/constant";
      if (!m_varyingGrid) {
         auto ret = loadGrid(dir + "/polyMesh");
         UnstructuredGrid::ptr grid = ret.first;
         setMeta(grid, processor, timestep);
         Polygons::ptr poly = ret.second;
         setMeta(poly, processor, timestep);

         if (m_varyingCoords) {
            m_basegrid[processor] = grid;
            m_basebound[processor] = poly;
         } else {
            addObject(m_gridOut, grid);
            addObject(m_boundOut, poly);
         }
      }
      loadFields(dir, m_constantFields, processor, timestep);
   } else {
      int i = 0;
      int skipfactor = m_timeskip->getValue()+1;
      for (auto &ts: m_timedirs) {
         if (i == timestep*skipfactor) {
            dir += "/" + ts.second;
            break;
         }
         ++i;
      }
      if (i == m_timedirs.size()) {
         std::cerr << "no directory for timestep " << timestep << " found" << std::endl;
         return false;
      }
      if (m_varyingGrid || m_varyingCoords) {
         UnstructuredGrid::ptr grid;
         Polygons::ptr poly;
         if (m_varyingCoords) {
            {
               grid.reset(new UnstructuredGrid(0, 0, 0));
               UnstructuredGrid::Data *od = m_basegrid[processor]->d();
               UnstructuredGrid::Data *nd = grid->d();
               nd->tl = od->tl;
               nd->el = od->el;
               nd->cl = od->cl;
            }
            loadCoords(dir + "/polyMesh", grid);
            {
               poly.reset(new Polygons(0, 0, 0));
               Polygons::Data *od = m_basebound[processor]->d();
               Polygons::Data *nd = poly->d();
               nd->el = od->el;
               nd->cl = od->cl;
               for (int i=0; i<3; ++i)
                  poly->d()->x[i] = grid->d()->x[i];
            }
         } else {
            auto ret = loadGrid(dir + "/polyMesh");
            grid = ret.first;
            poly = ret.second;
         }
         setMeta(grid, processor, timestep);
         addObject(m_gridOut, grid);
         setMeta(poly, processor, timestep);
         addObject(m_boundOut, poly);
      }
      loadFields(dir, m_varyingFields, processor, timestep);
   }

   return true;
}

bool ReadFOAM::readConstant(const std::string &casedir)
{
   std::cerr << "reading constant data..." << std::endl;
   if (m_numprocessors > 0) {
      for (int i=0; i<m_numprocessors; ++i) {
         if (i % size() == rank()) {
            if (!readDirectory(casedir, i, -1))
               return false;
         }
      }
   } else {
      if (rank() == 0) {
         if (!readDirectory(casedir, -1, -1))
            return false;
      }
   }

   return true;
}

bool ReadFOAM::readTime(const std::string &casedir, int timestep) {

   std::cerr << "reading time step " << timestep << "..." << std::endl;
   if (m_numprocessors > 0) {
      for (int i=0; i<m_numprocessors; ++i) {
         if (i % size() == rank()) {
            if (!readDirectory(casedir, i, timestep))
               return false;
         }
      }
   } else {
      if (rank() == 0) {
         if (!readDirectory(casedir, -1, timestep))
            return false;
      }
   }

   return true;
}

bool ReadFOAM::compute()     //Compute is called when Module is executed
{
   const std::string casedir = m_casedir->getValue();
   if (!checkCaseDirectory(casedir)) {
      std::cerr << casedir << " is not a valid OpenFOAM case" << std::endl;
      return false;
   }

   std::cerr << "# processors: " << m_numprocessors << std::endl;
   std::cerr << "# time steps: " << m_timedirs.size() << std::endl;
   std::cerr << "grid topology: " << (m_varyingGrid?"varying":"constant") << std::endl;
   std::cerr << "grid coordinates: " << (m_varyingCoords?"varying":"constant") << std::endl;

   readConstant(casedir);
   int skipfactor = m_timeskip->getValue()+1;
   for (int timestep=0; timestep<m_timedirs.size()/skipfactor; ++timestep) {
      readTime(casedir, timestep);
   }

   m_basegrid.clear();
   std::cerr << "ReadFoam: done" << std::endl;

   return true;
}

MODULE_MAIN(ReadFOAM)
