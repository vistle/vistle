#include <boost/algorithm/string/predicate.hpp>

#include <core/object.h>
#include <core/vec.h>
#include <core/polygons.h>
#include <core/lines.h>
#include <core/points.h>
#include <core/unstr.h>
#include <core/uniformgrid.h>
#include <core/rectilineargrid.h>
#include <core/structuredgrid.h>

#include "ReadIagTecplot.h"
#include "topo.h"
#include "mesh.h"

#include <util/filesystem.h>

MODULE_MAIN(ReadIagTecplot)

using namespace vistle;

bool ReadIagTecplot::examine(const Parameter *param) {

    if (param != nullptr && param != m_filename)
        return true;

    const std::string filename = m_filename->getValue();
    size_t numZones = 0;
    try {
        TecplotFile tecplot(filename);
        numZones = tecplot.NumZones();
    } catch(...) {
        std::cerr << "failed to create TecplotFile for " << filename << std::endl;
        setPartitions(0);
        return false;
    }

    setPartitions(std::min(size_t(size()*32), numZones));

    return true;
}

bool ReadIagTecplot::read(Reader::Token &token, int timestep, int block)
{
   auto unstr = std::make_shared<UnstructuredGrid>(0,0,0);
   auto &x = unstr->x();
   auto &y = unstr->y();
   auto &z = unstr->z();
   auto &el = unstr->el();
   auto &cl = unstr->cl();
   auto &tl = unstr->tl();

   Vec<Scalar>::ptr p,r;
   Vec<Scalar,3>::ptr n,u,v;

   typedef Vec<Scalar>::array array;

   array *pp = nullptr;
   if (m_p->isConnected()) {
       p = std::make_shared<Vec<Scalar>>(0);
       pp = &p->x();
   }
   array *rr = nullptr;
   if (m_r->isConnected()) {
       r = std::make_shared<Vec<Scalar>>(0);
       rr = &r->x();
   }
   array *nx = nullptr, *ny = nullptr, *nz = nullptr;
   if (m_n->isConnected()) {
       n = std::make_shared<Vec<Scalar,3>>(0);
       nx = &n->x();
       ny = &n->y();
       nz = &n->z();
   }
   array *ux = nullptr, *uy = nullptr, *uz = nullptr;
   if (m_u->isConnected()) {
       u = std::make_shared<Vec<Scalar,3>>(0);
       ux = &u->x();
       uy = &u->y();
       uz = &u->z();
   }
   array *vx = nullptr, *vy = nullptr, *vz = nullptr;
   if (m_v->isConnected()) {
       v = std::make_shared<Vec<Scalar,3>>(0);
       vx = &v->x();
       vy = &v->y();
       vz = &v->z();
   }

   const int npart = std::max(1,numPartitions());

   TecplotFile tecplot(m_filename->getValue());

   size_t numZones = tecplot.NumZones();
   size_t numZonesBlock = (numZones + npart - 1)/npart;
   size_t begin = block>=0 ? numZonesBlock*block : 0;
   size_t end = block>=0 ? std::min(numZones, numZonesBlock*(block+1)) : numZones;
   std::cerr << "reading zones "  << begin << " to " << end-1 << std::endl;

   for (size_t i=0; i<begin && i<numZones; ++i) {
#if 1
       tecplot.SkipZone(i);
#else
       auto mesh = tecplot.ReadZone(i);
       delete mesh;
#endif
   }

   Index baseVertex = 0;
   for (size_t i=begin; i<end; ++i) {
       auto mesh = tecplot.ReadZone(i);
       if (auto hexmesh = dynamic_cast<VolumeMesh<HexaederTopo> *>(mesh)) {
#if 1
           hexmesh->SetupVolume();
           std::cerr << "hexmesh: #points=" << hexmesh->getNumPoints() << ", #cells=" << hexmesh->getNumCells() << std::endl;

           std::map<int, Index> vertMap;
           for (int c=0; c<hexmesh->getNumCells(); ++c) {
               const auto &cell = mesh->getState(c);
               const auto &hex = static_cast<const HexaederTopo &>(cell);
               bool allNodes = true;
               for (int n=0; n<8; ++n) {
                   int v = hex.mNodes[n];
                   if (v < 0) {
                       allNodes = false;
                       break;
                   }
               }
               if (!allNodes)
                   continue;
               for (int n=0; n<8; ++n) {
                   int v = hex.mNodes[n];
                   auto p = vertMap.emplace(v, baseVertex+vertMap.size());
                   if (p.second) {
                       auto &ps = mesh->getPointState(v);
                       x.push_back(ps.X());
                       y.push_back(ps.Y());
                       z.push_back(ps.Z());

                       if (pp) {
                           pp->push_back(ps.mP);
                       }
                       if (rr) {
                           rr->push_back(ps.mR);
                       }
                       if (nx && ny && nz) {
                           nx->push_back(ps.mN.X());
                           ny->push_back(ps.mN.Y());
                           nz->push_back(ps.mN.Z());
                       }
                       if (ux && uy && uz) {
                           ux->push_back(ps.mU.X());
                           uy->push_back(ps.mU.Y());
                           uz->push_back(ps.mU.Z());
                       }
                       if (vx && vy && vz) {
                           vx->push_back(ps.mV.X());
                           vy->push_back(ps.mV.Y());
                           vz->push_back(ps.mV.Z());
                       }
                   }
                   Index &vv = p.first->second;
                   cl.push_back(vv);
               }
               tl.push_back(UnstructuredGrid::HEXAHEDRON);
               el.push_back(cl.size());
           }
           baseVertex += vertMap.size();
#endif
       } else if (auto tetmesh = dynamic_cast<VolumeMesh<HexaederTopo> *>(mesh)) {
           std::cerr << "tetmesh: #points=" << tetmesh->getNumPoints() << ", #cells=" << tetmesh->getNumCells() << std::endl;
       } else if (auto trisurf = dynamic_cast<SurfaceMesh<TriangleTopo> *>(mesh)) {
           std::cerr << "trisurf: #points=" << trisurf->getNumPoints() << ", #cells=" << trisurf->getNumCells() << std::endl;
       } else if (auto quadsurf = dynamic_cast<SurfaceMesh<QuadrangleTopo> *>(mesh)) {
           std::cerr << "quadsurf: #points=" << quadsurf->getNumPoints() << ", #cells=" << quadsurf->getNumCells() << std::endl;
       }
       delete mesh;
   }

   token.addObject(m_grid, unstr);
   if (p) {
       p->addAttribute("_species", "p");
       p->setGrid(unstr);
       token.addObject(m_p, p);
   }
   if (r) {
       r->addAttribute("_species", "r");
       r->setGrid(unstr);
       token.addObject(m_r, r);
   }
   if (n) {
       n->addAttribute("_species", "n");
       n->setGrid(unstr);
       token.addObject(m_n, n);
   }
   if (u) {
       u->addAttribute("_species", "u");
       u->setGrid(unstr);
       token.addObject(m_u, u);
   }
   if (v) {
       v->addAttribute("_species", "v");
       v->setGrid(unstr);
       token.addObject(m_v, v);
   }

   return true;

}


ReadIagTecplot::ReadIagTecplot(const std::string &name, int moduleID, mpi::communicator comm)
   : Reader("read IAG Tecplot data (hexahedra only)", name, moduleID, comm)
{

   m_filename = addStringParameter("filename", "name of Tecplot file", "", Parameter::ExistingFilename);

   m_grid = createOutputPort("grid_out");
   m_p = createOutputPort("p");
   m_r = createOutputPort("r");
   m_n = createOutputPort("n");
   m_u = createOutputPort("u");
   m_v = createOutputPort("v");
#if 0
   for (int i=0; i<NumPorts; ++i) {
      std::stringstream spara;
      spara << "cell_field_" << i;
      m_cellDataChoice[i] = addStringParameter(spara.str(), "cell data field", "", Parameter::Choice);
   }
#endif


#if 0
   for (int i=0; i<NumPorts; ++i) {
      std::stringstream spara;
      spara << "point_field_" << i;
      m_pointDataChoice[i] = addStringParameter(spara.str(), "point data field", "", Parameter::Choice);

      std::stringstream sport;
      sport << "point_data" << i;
      m_pointPort[i] = createOutputPort(sport.str(), "vertex data");
   }
   for (int i=0; i<NumPorts; ++i) {
      std::stringstream spara;
      spara << "cell_field_" << i;
      m_cellDataChoice[i] = addStringParameter(spara.str(), "cell data field", "", Parameter::Choice);

      std::stringstream sport;
      sport << "cell_data" << i;
      m_cellPort[i] = createOutputPort(sport.str(), "cell data");
   }
#endif

   //setParallelizationMode(Serial);
   setParallelizationMode(ParallelizeTimeAndBlocks);

   observeParameter(m_filename);
}

ReadIagTecplot::~ReadIagTecplot() {

}

#if 0
bool ReadIagTecplot::changeParameter(const vistle::Parameter *p) {
   if (p == m_filename) {
      const std::string filename = m_filename->getValue();
      try {
          m_tecplot.reset(new TecplotFile(filename));
      } catch(...) {
          std::cerr << "failed to create TecplotFile for " << filename << std::endl;
      }
      setChoices();
   }

   return Module::changeParameter(p);
}

bool ReadIagTecplot::prepare() {

   if (!m_tecplot) {
       if (rank() == 0)
           sendInfo("no Tecplot file open, tried %s", m_filename->getValue().c_str());
       return true;
   }

   size_t numZones = m_tecplot->NumZones();
   std::cerr << "reading " << numZones << " zones" << std::endl;
   //numZones = std::min(size_t(100), numZones);
   for (size_t i=0; i<numZones; ++i) {
       auto mesh = m_tecplot->ReadZone(i);
       if (auto hexmesh = dynamic_cast<VolumeMesh<HexaederTopo> *>(mesh)) {
           std::cerr << "hexmesh: #points=" << hexmesh->getNumPoints() << ", #cells=" << hexmesh->getNumCells() << std::endl;
#if 1
           for (int c=0; c<hexmesh->getNumCells(); ++c) {
               const auto &cell = mesh->getState(c);
               const auto &hex = static_cast<const HexaederTopo &>(cell);
               std::cerr << "hexeader: isCell=" << hex.isCell() << std::endl;
           }
#endif
       } else if (auto tetmesh = dynamic_cast<VolumeMesh<HexaederTopo> *>(mesh)) {
           std::cerr << "tetmesh: #points=" << tetmesh->getNumPoints() << ", #cells=" << tetmesh->getNumCells() << std::endl;
       } else if (auto trisurf = dynamic_cast<SurfaceMesh<TriangleTopo> *>(mesh)) {
           std::cerr << "trisurf: #points=" << trisurf->getNumPoints() << ", #cells=" << trisurf->getNumCells() << std::endl;
       } else if (auto quadsurf = dynamic_cast<SurfaceMesh<QuadrangleTopo> *>(mesh)) {
           std::cerr << "quadsurf: #points=" << quadsurf->getNumPoints() << ", #cells=" << quadsurf->getNumCells() << std::endl;
       }
       delete mesh;
   }

   return true;
}
#endif
