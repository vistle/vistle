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
//#include <util/coRestraint.h>

using namespace vistle;

ReadFOAM::ReadFOAM(const std::string &shmname, int rank, int size, int moduleId)
: Module("ReadFoam", shmname, rank, size, moduleId)
, m_gridOut(nullptr)
, m_boundOut(nullptr)
{
   // file browser parameter
   m_casedir = addStringParameter("casedir", "OpenFOAM case directory",
      "/mnt/raid/home/hpcchkop/OpenFoamFiles/motorBike", Parameter::Directory);
   //Time Parameters
   m_starttime = addFloatParameter("starttime", "start reading at the first step after this time", 0.);
   setParameterMinimum<Float>(m_starttime, 0.);
   m_stoptime = addFloatParameter("stoptime", "stop reading at the last step before this time",
         std::numeric_limits<double>::max());
   setParameterMinimum<Float>(m_stoptime, 0.);
   m_timeskip = addIntParameter("timeskip", "number of timesteps to skip", 0);
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
         StringParameter *p =  addStringParameter(s.str(), "name of field", "(NONE)", Parameter::Choice);
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
         StringParameter *p =  addStringParameter(s.str(), "name of field", "(NONE)", Parameter::Choice);
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

bool ReadFOAM::parameterChanged(Parameter *p)
{

   StringParameter *sp = dynamic_cast<StringParameter *>(p);
   if (sp == m_casedir) {
      sendMessage(message::Busy());
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
      std::stringstream meshdir;
      meshdir << casedir << "/constant/polyMesh"; //<< m_case.constantdir << "/polyMesh";
      sendInfo("Listing Boundary Patches!");
      Boundaries bounds = loadBoundary(meshdir.str());
      for (int i=0;i<bounds.boundaries.size();++i) {
         std::stringstream info;
         info << bounds.boundaries[i].index<< " ## " << bounds.boundaries[i].name;
         sendInfo("%s", info.str().c_str());
      }

      //fill choice parameters
      std::vector<std::string> choices = getFieldList();
      for (StringParameter *out: m_fieldOut) {
         setParameterChoices(out, choices);
      }
      for (StringParameter *out: m_boundaryOut) {
         setParameterChoices(out, choices);
      }
      sendMessage(message::Idle());
   }

   return Module::parameterChanged(p);
}

bool loadCoords(const std::string &meshdir, UnstructuredGrid::ptr grid) {

   boost::shared_ptr<std::istream> pointsIn = getStreamForFile(meshdir, "points");
   HeaderInfo pointsH = readFoamHeader(*pointsIn);
   grid->setSize(pointsH.lines);
   readFloatVectorArray(*pointsIn, grid->x().data(), grid->y().data(), grid->z().data(), pointsH.lines);

   return true;
}

std::pair<UnstructuredGrid::ptr, Polygons::ptr> ReadFOAM::loadGrid(const std::string &meshdir) {

   bool readGrid = m_readGrid->getValue();
   bool readBoundary = m_readBoundary->getValue();

   if (!readGrid && !readBoundary) {
      return std::make_pair(UnstructuredGrid::ptr(), Polygons::ptr()); //Module crashes when this is returned
   }

   Boundaries boundaries = loadBoundary(meshdir);
   DimensionInfo dim = readDimensions(meshdir);

   Polygons::ptr poly(new Polygons(0, 0, 0));
   UnstructuredGrid::ptr grid(new UnstructuredGrid(dim.cells, 0, 0));

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

      if (readBoundary) {
         auto &polys = poly->el();
         auto &conn = poly->cl();
         //Index num_bound = dim.faces - dim.internalFaces;
         //polys.reserve(num_bound); //TODO
         for (std::vector<Boundary>::const_iterator it=boundaries.boundaries.begin();
              it!=boundaries.boundaries.end();
              ++it) {
            int boundaryIndex=it->index;
            std::string selection=m_patchSelection->getValue();
            if (m_boundaryPatches(boundaryIndex) || strcmp(selection.c_str(),"all")==0) {
               for (index_t i=it->startFace; i<it->startFace + it->numFaces; ++i) {
                  polys.push_back(conn.size());
                  auto &face = faces[i];
                  for (int j=0; j<face.size(); ++j) {
                     conn.push_back(face[j]);
                  }
               }
               polys.push_back(conn.size());
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
      el[dim.cells] = connectivities.size();
   }


   loadCoords(meshdir, grid);
   if (readBoundary) {
      poly->d()->x[0] = grid->d()->x[0];
      poly->d()->x[1] = grid->d()->x[1];
      poly->d()->x[2] = grid->d()->x[2];
   }

   //std::cerr << " done!" << std::endl;

   return std::make_pair(grid, poly);
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
      addObject(m_volumeDataOut[i], obj);
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
      if (!m_case.varyingGrid) {
         auto ret = loadGrid(dir + "/polyMesh");
         UnstructuredGrid::ptr grid = ret.first;
         setMeta(grid, processor, timestep);
         Polygons::ptr poly = ret.second;
         setMeta(poly, processor, timestep);

         if (m_case.varyingCoords) {
            m_basegrid[processor] = grid;
            m_basebound[processor] = poly;
         } else {
            addObject(m_gridOut, grid);
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
         if (m_case.varyingCoords) {
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
      loadFields(dir, m_case.varyingFields, processor, timestep);
   }

   return true;
}

bool ReadFOAM::readConstant(const std::string &casedir)
{
   std::cerr << "reading constant data..." << std::endl;
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

   return true;
}

bool ReadFOAM::readTime(const std::string &casedir, int timestep) {

   std::cerr << "reading time step " << timestep << "..." << std::endl;
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

   return true;
}

bool ReadFOAM::compute()     //Compute is called when Module is executed
{
   const std::string casedir = m_casedir->getValue();
   m_boundaryPatches.add(m_patchSelection->getValue());
   m_case = getCaseInfo(casedir, m_starttime->getValue(), m_stoptime->getValue());
   if (!m_case.valid) {
      std::cerr << casedir << " is not a valid OpenFOAM case" << std::endl;
      return false;
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

   m_basegrid.clear();
   std::cerr << "ReadFoam: done" << std::endl;

   return true;
}

MODULE_MAIN(ReadFOAM)
