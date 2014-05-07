#ifndef READFOAM_H
#define READFOAM_H
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

#include <vector>
#include <map>

#include <module/module.h>
#include <core/unstr.h>
#include <core/polygons.h>

#include "foamtoolbox.h"
#include <util/coRestraint.h>

struct GridDataContainer {

   GridDataContainer(vistle::UnstructuredGrid::ptr g
                     , vistle::Polygons::ptr p
                     , boost::shared_ptr<std::vector<vistle::Index> > o
                     , boost::shared_ptr<Boundaries> b) {
      grid=g;
      polygon=p;
      owners=o;
      boundaries=b;
   }

   vistle::UnstructuredGrid::ptr grid;
   vistle::Polygons::ptr polygon;
   boost::shared_ptr<std::vector<vistle::Index> > owners;
   boost::shared_ptr<Boundaries> boundaries;
};

class ReadFOAM: public vistle::Module
{
   static const int NumPorts = 3;
   static const int NumBoundaryPorts = 3;

 public:
      virtual bool compute();
      ReadFOAM(const std::string &shmname, int rank, int size, int moduleId);
      virtual ~ReadFOAM();

   private:
      //Parameter
      vistle::StringParameter *m_casedir, *m_patchSelection;
      vistle::FloatParameter *m_starttime, *m_stoptime;
      vistle::IntParameter *m_timeskip;
      vistle::IntParameter *m_readGrid, *m_readBoundary;
      std::vector<vistle::StringParameter *> m_fieldOut , m_boundaryOut;
      //Ports
      vistle::Port *m_gridOut, *m_boundOut;
      std::vector<vistle::Port *> m_volumeDataOut, m_boundaryDataOut;

      vistle::coRestraint m_boundaryPatches;
      CaseInfo m_case;

      std::vector<std::string> getFieldList() const;

      bool parameterChanged(vistle::Parameter *p);
      bool readDirectory(const std::string &dir, int processor, int timestep);
      bool addGhostCells(int processor, int timestep);
      bool addGhostCellsData(const std::string &dir, int processor, int timestep);
      bool addGridToPorts(int processor);
      bool addDataToPorts(int processor);
      bool readConstant(const std::string &dir);
      bool readTime(const std::string &dir, int timestep);

      GridDataContainer loadGrid(const std::string &dir);
      vistle::Object::ptr loadField(const std::string &dir, const std::string &field);
      vistle::Object::ptr loadBoundaryField(const std::string &dir, const std::string &field,
                                            const int &processor);
      bool loadFields(const std::string &dir, const std::map<std::string, int> &fields,
            int processor, int timestep);

      void setMeta(vistle::Object::ptr obj, int processor, int timestep) const;

      std::map<int, vistle::UnstructuredGrid::ptr> m_basegrid;
      std::map<int, vistle::Polygons::ptr> m_basebound;
      std::map<int, vistle::UnstructuredGrid::ptr> m_currentgrid;
      std::map<int, std::map<int, vistle::Object::ptr> > m_currentvolumedata;
      std::map<int, boost::shared_ptr<std::vector<vistle::Index> > > m_owners;
      std::map<int, boost::shared_ptr<Boundaries>> m_boundaries;
      std::map<int, std::map<int, std::vector<vistle::Index> > > m_procBoundaryVertices;
      std::map<int, std::map<int, std::vector<vistle::Index> > > m_neighborProcBoundaryVertices;
};
#endif // READFOAM_H
