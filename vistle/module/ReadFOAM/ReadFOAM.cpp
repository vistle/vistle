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
#include <core/message.h>

//Includes copied from covise ReadFOAM.cpp
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <set>
#include <cctype>

#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <boost/shared_ptr.hpp>

#include "foamtoolbox.h"
#include <util/coRestraint.h>
#include <boost/serialization/vector.hpp>
#include <boost/mpi.hpp>
namespace mpi = boost::mpi;

using namespace vistle;

ReadFOAM::ReadFOAM(const std::string &shmname, int rank, int size, int moduleId)
: Module("ReadFoam", shmname, rank, size, moduleId)
, m_gridOut(nullptr)
, m_boundOut(nullptr)
{
   // file browser parameter
   m_casedir = addStringParameter("casedir", "OpenFOAM case directory",
      "/data/OpenFOAM", Parameter::Directory);
   //Time Parameters
   m_starttime = addFloatParameter("starttime", "start reading at the first step after this time", 0.);
   setParameterMinimum<Float>(m_starttime, 0.);
   m_stoptime = addFloatParameter("stoptime", "stop reading at the last step before this time",
         std::numeric_limits<double>::max());
   setParameterMinimum<Float>(m_stoptime, 0.);
   m_timeskip = addIntParameter("timeskip", "skip this many timesteps after reading one", 0);
   setParameterMinimum<Integer>(m_timeskip, 0);
   m_readGrid = addIntParameter("read_grid", "load the grid?", 1, Parameter::Boolean);

   //Mesh ports
   m_gridOut = createOutputPort("grid_out");
   m_boundOut = createOutputPort("grid_out1");

   for (int i=0; i<NumPorts; ++i) {
      {// Data Ports
         std::stringstream s;
         s << "data_out" << i;
         m_volumeDataOut.push_back(createOutputPort(s.str()));
      }
      {// Date Choice Parameters
         std::stringstream s;
         s << "Data" << i;
         auto p =  addStringParameter(s.str(), "name of field", "(NONE)", Parameter::Choice);
         std::vector<std::string> choices;
         choices.push_back("(NONE)");
         setParameterChoices(p, choices);
         m_fieldOut.push_back(p);
      }
   }
   m_readBoundary = addIntParameter("read_boundary", "load the boundary?", 1, Parameter::Boolean);
   m_patchSelection = addStringParameter("patches", "select patches","all");
   for (int i=0; i<NumBoundaryPorts; ++i) {
      {// 2d Data Ports
         std::stringstream s;
         s << "data_2d_out" << i;
         m_boundaryDataOut.push_back(createOutputPort(s.str()));
      }
      {// 2d Data Choice Parameters
         std::stringstream s;
         s << "Data2d" << i;
         auto p =  addStringParameter(s.str(), "name of field", "(NONE)", Parameter::Choice);
         std::vector<std::string> choices;
         choices.push_back("(NONE)");
         setParameterChoices(p, choices);
         m_boundaryOut.push_back(p);
      }
   }
}


ReadFOAM::~ReadFOAM()       //Destructor
{
}

std::vector<std::string> ReadFOAM::getFieldList() const {

   std::vector<std::string> choices;
   choices.push_back("(NONE)");

   if (m_case.valid) {
      for (auto &field: m_case.constantFields)
         choices.push_back(field.first);
      for (auto &field: m_case.varyingFields)
         choices.push_back(field.first);
   }

   return choices;
}

bool ReadFOAM::parameterChanged(const Parameter *p)
{
   auto sp = dynamic_cast<const StringParameter *>(p);
   if (sp == m_casedir) {
      std::string casedir = sp->getValue();

      m_case = getCaseInfo(casedir, m_starttime->getValue(), m_stoptime->getValue());
      if (!m_case.valid) {
         std::cerr << casedir << " is not a valid OpenFOAM case" << std::endl;
         return false;
      }

      std::cerr << "# processors: " << m_case.numblocks << std::endl;
      std::cerr << "# time steps: " << m_case.timedirs.size() << std::endl;
      std::cerr << "grid topology: " << (m_case.varyingGrid?"varying":"constant") << std::endl;
      std::cerr << "grid coordinates: " << (m_case.varyingCoords?"varying":"constant") << std::endl;

      //print out a list of boundary patches to Vistle Console
      if (rank() == 0) {
         std::stringstream meshdir;
         meshdir << casedir << "/constant/polyMesh"; //<< m_case.constantdir << "/polyMesh";
         Boundaries bounds = loadBoundary(meshdir.str());
         if (bounds.valid) {
            sendInfo("boundary patches:");
            for (int i=0;i<bounds.boundaries.size();++i) {
               std::stringstream info;
               info << bounds.boundaries[i].index<< " ## " << bounds.boundaries[i].name;
               sendInfo("%s", info.str().c_str());
            }
         } else {
            sendInfo("No global boundary file was found at:");
            sendInfo(meshdir.str());
         }
      }

      //fill choice parameters
      std::vector<std::string> choices = getFieldList();
      for (auto out: m_fieldOut) {
         setParameterChoices(out, choices);
      }
      for (auto out: m_boundaryOut) {
         setParameterChoices(out, choices);
      }
   }

   return Module::parameterChanged(p);
}

bool loadCoords(const std::string &meshdir, Coords::ptr grid) {

   boost::shared_ptr<std::istream> pointsIn = getStreamForFile(meshdir, "points");
   if (!pointsIn)
      return false;
   HeaderInfo pointsH = readFoamHeader(*pointsIn);
   grid->setSize(pointsH.lines);
   readFloatVectorArray(*pointsIn, grid->x().data(), grid->y().data(), grid->z().data(), pointsH.lines);

   return true;
}

GridDataContainer ReadFOAM::loadGrid(const std::string &meshdir) {

   bool readGrid = m_readGrid->getValue();
   bool readBoundary = m_readBoundary->getValue();

   DimensionInfo dim = readDimensions(meshdir);

   UnstructuredGrid::ptr grid(new UnstructuredGrid(0, 0, 0));
   Polygons::ptr poly(new Polygons(0, 0, 0));
   boost::shared_ptr<std::vector<Index> > owners(new std::vector<Index>());
   boost::shared_ptr<Boundaries> boundaries(new Boundaries());
   GridDataContainer result(grid,poly,owners,boundaries);
   (*boundaries) = loadBoundary(meshdir);
   if (!readGrid && !readBoundary) {
      return result;
   }

   //read mesh files
   boost::shared_ptr<std::istream> ownersIn = getStreamForFile(meshdir, "owner");
   if (!ownersIn)
      return result;
   HeaderInfo ownerH = readFoamHeader(*ownersIn);
   owners->resize(ownerH.lines);
   readIndexArray(*ownersIn, (*owners).data(), (*owners).size());

   {

      boost::shared_ptr<std::istream> facesIn = getStreamForFile(meshdir, "faces");
      if (!facesIn)
         return result;
      HeaderInfo facesH = readFoamHeader(*facesIn);
      std::vector<std::vector<Index>> faces(facesH.lines);
      readIndexListArray(*facesIn, faces.data(), faces.size());

      boost::shared_ptr<std::istream> neighborsIn = getStreamForFile(meshdir, "neighbour");
      if (!neighborsIn)
         return result;
      HeaderInfo neighbourH = readFoamHeader(*neighborsIn);
      if (neighbourH.lines != dim.internalFaces) {
         std::cerr << "inconsistency: #internalFaces != #neighbours" << std::endl;
      }
      std::vector<Index> neighbour(neighbourH.lines);
      readIndexArray(*neighborsIn, neighbour.data(), neighbour.size());

      //Boundary Polygon
      if (readBoundary) {
         auto &polys = poly->el();
         auto &conn = poly->cl();
         Index num_bound = 0;
         for (const auto &b: (*boundaries).boundaries) {
            int boundaryIndex=b.index;
            if (m_boundaryPatches(boundaryIndex)) {
               num_bound+=b.numFaces;
            }
         }
         polys.reserve(num_bound+1);
         for (const auto &b: (*boundaries).boundaries) {
            int boundaryIndex=b.index;
            if (m_boundaryPatches(boundaryIndex) && b.numFaces>0) {
               for (index_t i=b.startFace; i<b.startFace + b.numFaces; ++i) {
                  auto &face = faces[i];
                  for (int j=0; j<face.size(); ++j) {
                     conn.push_back(face[j]);
                  }
                  polys.push_back(conn.size());
               }
            }
         }
      }

      //Vertices lists for GhostCell creation -
      //each node creates lists of the outer vertices that are shared with other domains
      //with either its own or the neighbouring domain's face-numbering (clockwise or ccw)
      //so that two domains have the same list for a mutual border
      //therefore m_procBoundaryVertices[0][1] == m_procBoundaryVertices[1][0]
      for (const auto &b: (*boundaries).procboundaries) {
         std::vector<Index> outerVertices;
         int myProc=b.myProc;
         int neighborProc=b.neighborProc;
         if (myProc < neighborProc) {
            //create with own numbering
            for (int i=b.startFace; i<b.startFace+b.numFaces; ++i) {
               auto face=faces[i];
               for (int j=0; j<face.size(); ++j) {
                  outerVertices.push_back(face[j]);
               }
            }
         } else {
            //create with neighbour numbering (reverse direction)
            for (int i=b.startFace; i<b.startFace+b.numFaces; ++i) {
               auto face=faces[i];
               outerVertices.push_back(face[0]);
               for (int j=face.size()-1; j>0; --j) {
                  outerVertices.push_back(face[j]);
               }
            }
         }
         m_procBoundaryVertices[myProc][neighborProc] = outerVertices;
      }

      //Grid
      if (readGrid) {
         grid->el().resize(dim.cells+1);
         grid->tl().resize(dim.cells);
         //Create CellFaceMap
         std::vector<std::vector<Index>> cellfacemap(dim.cells);
         for (Index face = 0; face < (*owners).size(); ++face) {
            cellfacemap[(*owners)[face]].push_back(face);
         }
         for (Index face = 0; face < neighbour.size(); ++face) {
            cellfacemap[neighbour[face]].push_back(face);
         }
         auto types = grid->tl().data();
         Index num_conn = 0;
         //Check Shape of Cells and add fill Type_List
         for (index_t i=0; i<dim.cells; i++) {
            const std::vector<Index> &cellfaces=cellfacemap[i];
            const std::vector<index_t> cellvertices = getVerticesForCell(cellfaces, faces);
            bool onlySimpleFaces=true; //Simple Face = Triangle or Rectangle
            for (index_t j=0; j<cellfaces.size(); ++j) {
               if (faces[cellfaces[j]].size()<3 || faces[cellfaces[j]].size()>4) {
                  onlySimpleFaces=false;
                  break;
               }
            }
            const Index num_faces = cellfaces.size();
            Index num_verts = cellvertices.size();
            if (num_faces==6 && num_verts==8 && onlySimpleFaces) {
               types[i]=UnstructuredGrid::HEXAHEDRON;
            } else if (num_faces==5 && num_verts==6 && onlySimpleFaces) {
               types[i]=UnstructuredGrid::PRISM;
            } else if (num_faces==5 && num_verts==5 && onlySimpleFaces) {
               types[i]=UnstructuredGrid::PYRAMID;
            } else if (num_faces==4 && num_verts==4 && onlySimpleFaces) {
               types[i]=UnstructuredGrid::TETRAHEDRON;
            } else {
               types[i]=UnstructuredGrid::POLYHEDRON;
               num_verts=0;
               for (Index j=0; j<cellfaces.size(); ++j) {
                  num_verts += faces[cellfaces[j]].size() + 1;
               }
            }
            num_conn += num_verts;
         }
         //save data cell by cell to element, connectivity and type list
         auto el = grid->el().data();
         auto &connectivities = grid->cl();
         auto inserter = std::back_inserter(connectivities);
         connectivities.reserve(num_conn);
         for(index_t i=0;  i<dim.cells; i++) {
            //element list
            el[i] = connectivities.size();
            //connectivity list
            const auto &cellfaces=cellfacemap[i];//get all faces of current cell
            switch (types[i]) {
               case UnstructuredGrid::HEXAHEDRON: {
                  index_t ia=cellfaces[0];//use the first face as starting face
                  std::vector<index_t> a=faces[ia];

                  if (!isPointingInwards(ia,i,dim.internalFaces,(*owners),neighbour)) {
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
                  while (faces[ia].size()>3) {//find first face with 3 vertices to use as starting face
                     ia=cellfaces[it++];
                  }

                  std::vector<index_t> a=faces[ia];

                  if(!isPointingInwards(ia,i,dim.internalFaces,(*owners),neighbour)) {
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
                  while (faces[ia].size()<4) {//find the rectangular face to use as starting face
                     ia=cellfaces[it++];
                  }

                  std::vector<index_t> a=faces[ia];

                  if(!isPointingInwards(ia,i,dim.internalFaces,(*owners),neighbour)) {
                     std::reverse(a.begin(), a.end());
                  }

                  std::copy(a.begin(), a.end(), inserter);
                  connectivities.push_back(findVertexAlongEdge(a[0],ia,cellfaces,faces));
               }
               break;

               case UnstructuredGrid::TETRAHEDRON: {
                  index_t ia=cellfaces[0];//use first face as starting face
                  std::vector<index_t> a=faces[ia];

                  if(!isPointingInwards(ia,i,dim.internalFaces,(*owners),neighbour)) {
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

                     if(!isPointingInwards(ia,i,dim.internalFaces,(*owners),neighbour)) {
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
         el[dim.cells] = connectivities.size();
      }
   }

   if (readGrid) {
      loadCoords(meshdir, grid);

      if (readBoundary) {//if grid has been read alaready -> re-use coordinate lists for the boundary-plygon
         poly->d()->x[0] = grid->d()->x[0];
         poly->d()->x[1] = grid->d()->x[1];
         poly->d()->x[2] = grid->d()->x[2];
      }
   } else {
      loadCoords(meshdir, poly);
   }
   return result;
}

Object::ptr ReadFOAM::loadField(const std::string &meshdir, const std::string &field) {

   boost::shared_ptr<std::istream> stream = getStreamForFile(meshdir, field);
   if (!stream) {
      std::cerr << "failed to open " << meshdir << "/" << field << std::endl;
      return Object::ptr();
   }
   HeaderInfo header = readFoamHeader(*stream);
   if (header.fieldclass == "volScalarField") {
      Vec<Scalar>::ptr s(new Vec<Scalar>(header.lines));
      readFloatArray(*stream, s->x().data(), s->x().size());
      return s;
   } else if (header.fieldclass == "volVectorField") {
      Vec<Scalar, 3>::ptr v(new Vec<Scalar, 3>(header.lines));
      readFloatVectorArray(*stream, v->x().data(), v->y().data(), v->z().data(), v->x().size());
      return v;
   }

   std::cerr << "cannot interpret " << meshdir << "/" << field << std::endl;
   return Object::ptr();
}

Object::ptr ReadFOAM::loadBoundaryField(const std::string &meshdir, const std::string &field,
                                        const int &processor) {
   auto &boundaries = *m_boundaries[processor];
   auto owners = m_owners[processor]->data();
   std::vector<index_t> dataMapping;
   //Create the dataMapping Vector
   for (const auto &b: boundaries.boundaries) {
      Index boundaryIndex=b.index;
      if (m_boundaryPatches(boundaryIndex)) {
         for (index_t i=b.startFace; i<b.startFace + b.numFaces; ++i) {
            dataMapping.push_back(owners[i]);
         }
      }
   }

   boost::shared_ptr<std::istream> stream = getStreamForFile(meshdir, field);
   if (!stream) {
      std::cerr << "failed to open " << meshdir << "/" << field << std::endl;
      return Object::ptr();
   }
   HeaderInfo header = readFoamHeader(*stream);
   if (header.fieldclass == "volScalarField") {
      std::vector<scalar_t> fullX(header.lines);
      readFloatArray(*stream, fullX.data(), header.lines);

      Vec<Scalar>::ptr s(new Vec<Scalar>(dataMapping.size()));
      auto x = s->x().data();
      for (index_t i=0;i<dataMapping.size();++i) {
         x[i] = fullX[dataMapping[i]];
      }

      return s;

   } else if (header.fieldclass == "volVectorField") {
      std::vector<scalar_t> fullX(header.lines),fullY(header.lines),fullZ(header.lines);
      readFloatVectorArray(*stream,fullX.data(),fullY.data(),fullZ.data(),header.lines);

      Vec<Scalar, 3>::ptr v(new Vec<Scalar, 3>(dataMapping.size()));
      auto x = v->x().data();
      auto y = v->y().data();
      auto z = v->z().data();
      for (index_t i=0;i<dataMapping.size();++i) {
         x[i] = fullX[dataMapping[i]];
         y[i] = fullY[dataMapping[i]];
         z[i] = fullZ[dataMapping[i]];
      }
      return v;
   }

   std::cerr << "cannot interpret " << meshdir << "/" << field << std::endl;
   return Object::ptr();
}

void ReadFOAM::setMeta(Object::ptr obj, int processor, int timestep) const {

   if (obj) {
      obj->setTimestep(timestep);
      obj->setNumTimesteps(m_case.timedirs.size());
      obj->setBlock(processor);
      obj->setNumBlocks(m_case.numblocks == 0 ? 1 : m_case.numblocks);

      if (timestep >= 0) {
         int i = 0;
         for (auto &ts: m_case.timedirs) {
            if (i == timestep) {
               obj->setRealTime(ts.first);
               break;
            }
            ++i;
         }
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
      m_currentvolumedata[processor][i]= obj;
   }

   for (int i=0; i<NumBoundaryPorts; ++i) {
      std::string field = m_boundaryOut[i]->getValue();
      auto it = fields.find(field);
      if (it == fields.end())
         continue;
      Object::ptr obj = loadBoundaryField(meshdir, field, processor);
      setMeta(obj, processor, timestep);
      addObject(m_boundaryDataOut[i], obj);
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
      dir += "/" + m_case.constantdir;
      if (!m_case.varyingGrid){
         auto ret = loadGrid(dir + "/polyMesh");
         UnstructuredGrid::ptr grid = ret.grid;
         setMeta(grid, processor, timestep);
         Polygons::ptr poly = ret.polygon;
         setMeta(poly, processor, timestep);
         m_owners[processor] = ret.owners;
         m_boundaries[processor] = ret.boundaries;

         if (m_case.varyingCoords) {
            m_basegrid[processor] = grid;
            m_currentgrid[processor] = grid;
            m_basebound[processor] = poly;
         } else {
            m_currentgrid[processor] = grid;
            addObject(m_boundOut, poly);
         }
      }
      loadFields(dir, m_case.constantFields, processor, timestep);
   } else {
      int i = 0;
      int skipfactor = m_timeskip->getValue()+1;
      for (auto &ts: m_case.timedirs) {
         if (i == timestep*skipfactor) {
            dir += "/" + ts.second;
            break;
         }
         ++i;
      }
      if (i == m_case.timedirs.size()) {
         std::cerr << "no directory for timestep " << timestep << " found" << std::endl;
         return false;
      }
      if (m_case.varyingGrid || m_case.varyingCoords) {
         UnstructuredGrid::ptr grid;
         Polygons::ptr poly;
         if (!m_case.varyingGrid) {
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
            grid = ret.grid;
            poly = ret.polygon;
            m_owners[processor] = ret.owners;
            m_boundaries[processor] = ret.boundaries;
         }
         setMeta(grid, processor, timestep);
         setMeta(poly, processor, timestep);
         m_currentgrid[processor] = grid;
         addObject(m_boundOut, poly);
      }
      loadFields(dir, m_case.varyingFields, processor, timestep);
   }

   return true;
}

int tag(int p, int n, int i=0) { //MPI needs a unique ID for each send/receive request, this function creates unique ids for each processor pairing
   return p*10000+n*100+i;
}

bool ReadFOAM::buildGhostCells(int processor, GhostMode mode) {
   auto &boundaries = *m_boundaries[processor];
   auto &owners = *m_owners[processor];

   UnstructuredGrid::ptr grid = m_currentgrid[processor];
   auto &el = grid->el();
   auto &cl = grid->cl();
   auto &tl = grid->tl();
   auto &x = grid->x();
   auto &y = grid->y();
   auto &z = grid->z();

   for (const auto &b :boundaries.procboundaries) {
      Index neighborProc=b.neighborProc;
      boost::shared_ptr<GhostCells> out(new GhostCells()); //object that will be sent to neighbor processor
      m_GhostCellsOut[processor][neighborProc] = out;
      std::vector<Index> &procBoundaryVertices = m_procBoundaryVertices[processor][neighborProc];

      std::vector<Index> &elOut = out->el;
      std::vector<SIndex> &clOut = out->cl;
      std::vector<Index> &tlOut = out->tl;
      std::vector<Scalar> &pointsOutX = out->x;
      std::vector<Scalar> &pointsOutY = out->y;
      std::vector<Scalar> &pointsOutZ = out->z;

      if (mode == ALL || mode == BASE) { //create ghost cell topology and send vertice-coordinates
         elOut.reserve(b.numFaces + 1);
         elOut.push_back(0);
         tlOut.reserve(b.numFaces);
         Index conncount=0;
         for (Index i=0;i<b.numFaces;++i) {
            Index cell=owners[b.startFace + i];
            Index elementStart = el[cell];
            Index elementEnd = el[cell + 1];
            for (Index j=elementStart; j<elementEnd; ++j) {
               SIndex point = cl[j];
               clOut.push_back(point);
               ++conncount;
            }
            elOut.push_back(conncount);
            tlOut.push_back(tl[cell]);
         }

         //Create Vertices Mapping
         std::map<Index, SIndex> verticesMapping;
         //shared vertices (coords do not have to be sent) -> mapped to negative values
         SIndex c=-1;
         for (const Index &v: procBoundaryVertices) {
            if (verticesMapping.emplace(v,c).second) {
               --c;
            }
         }
         //vertices with coordinates that have to be sent -> mapped to positive values
         c=0;
         for (const SIndex &v: clOut) {
            if (verticesMapping.emplace(v,c).second) {
               ++c;
            }
         }

         //Change connectivity list to use the mapped values
         for (SIndex &v: clOut) {
            v = verticesMapping[v];
         }

         //save the vertices mapping for later use if mode==BASE
         if (mode == BASE) {
            m_verticesMappings[processor][neighborProc]=verticesMapping;
         }

         //create and fill Coordinate Vectors that have to be sent
         pointsOutX.resize(c);
         pointsOutY.resize(c);
         pointsOutZ.resize(c);

         for (auto &v: verticesMapping) {
            Index f=v.first;
            SIndex s=v.second;
            if (s > -1) {
               pointsOutX[s] = x[f];
               pointsOutY[s] = y[f];
               pointsOutZ[s] = z[f];
            }
         }
      } else if (mode == COORDS){ //only send the unknown vertices and assume that the base-topology is already known from a previous timestep
                                  //buildGhostCells needs to be called with mode parameter BASE once before this will work
                                  //and applyGhostCells has to be called with mode parameter COORDS when receiving only coordinates
         std::map<Index, SIndex> verticesMapping = m_verticesMappings[processor][neighborProc];
         SIndex c=0;
         for (auto &v: verticesMapping) {
            SIndex s=v.second;
            if (s > -1) {
               ++c;
            }
         }
         pointsOutX.resize(c);
         pointsOutY.resize(c);
         pointsOutZ.resize(c);

         for (auto &v: verticesMapping) {
            Index f=v.first;
            SIndex s=v.second;
            if (s > -1) {
               pointsOutX[s] = x[f];
               pointsOutY[s] = y[f];
               pointsOutZ[s] = z[f];
            }
         }
      }
   }

   //build requests for Ghost Cells
   mpi::communicator world;
   for (const auto &b :boundaries.procboundaries) {
      Index neighborProc=b.neighborProc;
      int myRank=rank();
      int neighborRank = neighborProc % size();
      boost::shared_ptr<GhostCells> out = m_GhostCellsOut[processor][neighborProc];
      if (myRank != neighborRank) {
         m_requests[myRank].push_back(world.isend(neighborRank, tag(processor,neighborProc), *out));
         boost::shared_ptr<GhostCells> in(new GhostCells());
         m_GhostCellsIn[processor][neighborProc] = in;
         m_requests[myRank].push_back(world.irecv(neighborRank, tag(neighborProc,processor), *in));
      } else {
         m_GhostCellsIn[processor][neighborProc] = out;
      }
   }
   return true;
}

bool ReadFOAM::buildGhostCellData(int processor) {
   auto &boundaries = *m_boundaries[processor];
   auto &owners = *m_owners[processor];
   for (const auto &b :boundaries.procboundaries) {
      Index neighborProc=b.neighborProc;
      for (int i = 0; i < NumPorts; ++i) {
         auto f=m_currentvolumedata[processor].find(i);
         if (f == m_currentvolumedata[processor].end()) {
            continue;
         }
         Object::ptr obj = f->second;
         Vec<Scalar, 1>::ptr v1 = Vec<Scalar, 1>::as(obj);
         Vec<Scalar, 3>::ptr v3 = Vec<Scalar, 3>::as(obj);
         if (!v1 && !v3) {
            continue;
            std::cerr << "Could not send Data - unsupported Data-Object Type" << std::endl;
         }
         if (v1) {
            boost::shared_ptr<GhostData> dataOut(new GhostData(1));
            m_GhostDataOut[processor][neighborProc][i] = dataOut;
            auto &d=v1->x(0);
            for (Index i=0;i<b.numFaces;++i) {
               Index cell=owners[b.startFace + i];
               (*dataOut).x[0].push_back(d[cell]);
            }
         } else if (v3) {
            boost::shared_ptr<GhostData> dataOut(new GhostData(3));
            m_GhostDataOut[processor][neighborProc][i] = dataOut;
            for (Index j=0; j<3; ++j) {
               auto &d=v3->x(j);
               for (Index i=0;i<b.numFaces;++i) {
                  Index cell=owners[b.startFace + i];
                  (*dataOut).x[j].push_back(d[cell]);
               }
            }
         }
      }
   }

   //build requests for Ghost Data
   mpi::communicator world;
   for (const auto &b :boundaries.procboundaries) {
      Index neighborProc=b.neighborProc;
      int myRank=rank();
      int neighborRank = neighborProc % size();

      std::map<int, boost::shared_ptr<GhostData> > &m = m_GhostDataOut[processor][neighborProc];
      for (Index i=0; i<NumPorts; ++i) {
         if (m.find(i) != m.end()) {
            boost::shared_ptr<GhostData> dataOut = m[i];
            if (myRank != neighborRank) {
               m_requests[myRank].push_back(world.isend(neighborRank, tag(processor,neighborProc,i+1), *dataOut));
               boost::shared_ptr<GhostData> dataIn(new GhostData((*dataOut).dim));
               m_GhostDataIn[processor][neighborProc][i] = dataIn;
               m_requests[myRank].push_back(world.irecv(neighborRank, tag(neighborProc,processor,i+1), *dataIn));
            } else {
               m_GhostDataIn[processor][neighborProc][i] = dataOut;
            }
         }
      }
   }
   return true;
}

bool ReadFOAM::processAllRequests() {
   std::vector<mpi::request> r=m_requests[rank()];
   mpi::wait_all(r.begin(),r.end());
   m_requests.clear();
   m_GhostCellsOut.clear();
   m_GhostDataOut.clear();
   return true;
}

bool ReadFOAM::applyGhostCells(int processor, GhostMode mode) {
   auto &boundaries = *m_boundaries[processor];
   UnstructuredGrid::ptr grid = m_currentgrid[processor];
   auto &el = grid->el();
   auto &cl = grid->cl();
   auto &tl = grid->tl();
   auto &x = grid->x();
   auto &y = grid->y();
   auto &z = grid->z();

   for (const auto &b :boundaries.procboundaries) {
       Index neighborProc=b.neighborProc;
       std::vector<Index> &procBoundaryVertices = m_procBoundaryVertices[processor][neighborProc];
       std::vector<Index> sharedVerticesMapping;
       for (const Index &v: procBoundaryVertices) {
          if(std::find(sharedVerticesMapping.begin(), sharedVerticesMapping.end(), v) == sharedVerticesMapping.end()) {
              sharedVerticesMapping.push_back(v);
          }
       }
       boost::shared_ptr<GhostCells> in = m_GhostCellsIn[processor][neighborProc];
       std::vector<Index> &elIn = in->el;
       std::vector<SIndex> &clIn = in->cl;
       std::vector<Index> &tlIn = in->tl;
       std::vector<Scalar> &pointsInX = in->x;
       std::vector<Scalar> &pointsInY = in->y;
       std::vector<Scalar> &pointsInZ = in->z;
       Index pointsSize=x.size();

       if (mode == ALL) {
          for (int cell = 0; cell < tlIn.size();++cell) {//append new topology to old grid
             Index elementStart = elIn[cell];
             Index elementEnd = elIn[cell + 1];
             for (Index i = elementStart; i < elementEnd; ++i) {
                SIndex point = clIn[i];
                if (point < 0) {//if point<0 then vertice is already known and can be looked up in sharedVerticesMapping
                   point=sharedVerticesMapping[(point*-1)-1];
                } else {//else the vertice is unknown and its coordinates will be appended to the old coord-lists so we point to an index beyond the current size
                   point+=pointsSize;
                }
                cl.push_back(point);
             }
             el.push_back(cl.size());
             tl.push_back(tlIn[cell]|UnstructuredGrid::GHOST_BIT);
          }

          for (int i=0; i<pointsInX.size(); ++i) {//append new coordinates to old coordinate-lists
             x.push_back(pointsInX[i]);
             y.push_back(pointsInY[i]);
             z.push_back(pointsInZ[i]);
          }

       } else { //mode == COORDS
          for (int i=0; i<pointsInX.size(); ++i) { //base topology is already known and only unknown vertices have to be applied again
             x.push_back(pointsInX[i]);
             y.push_back(pointsInY[i]);
             z.push_back(pointsInZ[i]);
          }
       }
   }
   m_GhostCellsIn[processor].clear();
   return true;
}

bool ReadFOAM::applyGhostCellsData(int processor) {
   auto &boundaries = *m_boundaries[processor];

   for (const auto &b :boundaries.procboundaries) {
      Index neighborProc=b.neighborProc;
      for (int i = 0; i < NumPorts; ++i) {
         auto f=m_currentvolumedata[processor].find(i);
         if (f == m_currentvolumedata[processor].end()) {
            continue;
         }
         Object::ptr obj = f->second;
         Vec<Scalar, 1>::ptr v1 = Vec<Scalar, 1>::as(obj);
         Vec<Scalar, 3>::ptr v3 = Vec<Scalar, 3>::as(obj);
         if (!v1 && !v3) {
            continue;
            std::cerr << "Could not apply Data - unsupported Data-Object Type" << std::endl;
         }
         //append ghost cell data to old data objects
         if (v1) {
            boost::shared_ptr<GhostData> dataIn = m_GhostDataIn[processor][neighborProc][i];
            auto &d=v1->x(0);
            std::vector<Scalar> &x=(*dataIn).x[0];
            for (Index i=0;i<b.numFaces;++i) {
               d.push_back(x[i]);
            }
         } else if (v3) {
            boost::shared_ptr<GhostData> dataIn = m_GhostDataIn[processor][neighborProc][i];
            for (Index j=0; j<3; ++j) {
               auto &d=v3->x(j);
               std::vector<Scalar> &x=(*dataIn).x[j];
               for (Index i=0;i<b.numFaces;++i) {
                  d.push_back(x[i]);
               }
            }
         }
      }
   }
   m_GhostDataIn[processor].clear();
   return true;
}

bool ReadFOAM::addGridToPorts(int processor) {
   addObject(m_gridOut, m_currentgrid[processor]);
   return true;
}

bool ReadFOAM::addVolumeDataToPorts(int processor) {
   for (auto &data: m_currentvolumedata[processor]) {
      int portnum=data.first;
      addObject(m_volumeDataOut[portnum], m_currentvolumedata[processor][portnum]);
   }
   return true;
}

bool ReadFOAM::readConstant(const std::string &casedir)
{
   bool readGrid = m_readGrid->getValue();
   if (m_case.numblocks > 0) {
      for (int i=0; i<m_case.numblocks; ++i) {
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

   mpi::communicator c;
   c.barrier();

   if (!m_case.varyingCoords && !m_case.varyingGrid && readGrid) {
      if (m_case.numblocks > 0) {
         for (int i=0; i<m_case.numblocks; ++i) {
            if (i % size() == rank()) {
               if (!buildGhostCells(i,ALL))
                  return false;
            }
         }

         c.barrier();
         processAllRequests();

         for (int i=0; i<m_case.numblocks; ++i) {
            if (i % size() == rank()) {
               if (!applyGhostCells(i,ALL))
                  return false;
            }
            addGridToPorts(i);
         }

      } else {
         addGridToPorts(-1);
      }

   } else if (m_case.varyingCoords && readGrid) {
      if (m_case.numblocks > 0) {
         for (int i=0; i<m_case.numblocks; ++i) {
            if (i % size() == rank()) {
               if (!buildGhostCells(i,BASE))
                  return false;
            }
         }

         c.barrier();
         processAllRequests();

         for (int i=0; i<m_case.numblocks; ++i) {
            if (i % size() == rank()) {
               if (!applyGhostCells(i,ALL))
                  return false;
            }
         }
      }
   }

   m_currentgrid.clear();
   m_currentvolumedata.clear();
   return true;
}

bool ReadFOAM::readTime(const std::string &casedir, int timestep) {
   bool readGrid = m_readGrid->getValue();
   if (m_case.numblocks > 0) {
      for (int i=0; i<m_case.numblocks; ++i) {
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

   mpi::communicator c;
   c.barrier();

   if ((m_case.varyingCoords || m_case.varyingGrid) && readGrid) {
      if (m_case.numblocks > 0) {
         for (int i=0; i<m_case.numblocks; ++i) {
            if (i % size() == rank()) {
               if (m_case.varyingGrid) {
                  if (!buildGhostCells(i,ALL))
                     return false;
               } else {
                  if (!buildGhostCells(i,COORDS))
                     return false;
               }
            }
         }

         c.barrier();
         processAllRequests();

         for (int i=0; i<m_case.numblocks; ++i) {
            if (i % size() == rank()) {
               if (m_case.varyingGrid) {
                  if (!applyGhostCells(i,ALL))
                     return false;
               } else {
                  if (!applyGhostCells(i,COORDS))
                     return false;
               }
            }
            addGridToPorts(i);
         }
      } else {
         addGridToPorts(-1);
      }
   }



   if (m_case.numblocks > 0 && readGrid) {
      for (int i=0; i<m_case.numblocks; ++i) {
         if (i % size() == rank()) {
            if (!buildGhostCellData(i))
               return false;
         }
      }

      c.barrier();
      processAllRequests();

      for (int i=0; i<m_case.numblocks; ++i) {
         if (i % size() == rank()) {
            if (!applyGhostCellsData(i))
               return false;
         }
         addVolumeDataToPorts(i);
      }
   } else {
      addVolumeDataToPorts(-1);
   }

   m_currentgrid.clear();
   m_currentvolumedata.clear();
   return true;
}

bool ReadFOAM::compute()     //Compute is called when Module is executed
{
   if (rank() == 0)
      std::cout << time(0) << " starting" << std::endl;
   const std::string casedir = m_casedir->getValue();
   m_boundaryPatches.add(m_patchSelection->getValue());
   m_case = getCaseInfo(casedir, m_starttime->getValue(), m_stoptime->getValue());
   if (!m_case.valid) {
      std::cerr << casedir << " is not a valid OpenFOAM case" << std::endl;
      return true;
   }
   std::cerr << "# processors: " << m_case.numblocks << std::endl;
   std::cerr << "# time steps: " << m_case.timedirs.size() << std::endl;
   std::cerr << "grid topology: " << (m_case.varyingGrid?"varying":"constant") << std::endl;
   std::cerr << "grid coordinates: " << (m_case.varyingCoords?"varying":"constant") << std::endl;

   readConstant(casedir);
   int skipfactor = m_timeskip->getValue()+1;
   for (int timestep=0; timestep<m_case.timedirs.size()/skipfactor; ++timestep) {
      readTime(casedir, timestep);
   }

   if (rank() == 0)
      std::cout << time(0) << " done" << std::endl;
   return true;
}

MODULE_MAIN(ReadFOAM)
