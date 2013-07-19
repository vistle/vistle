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
#include <set>
#include <boost/iostreams/filtering_stream.hpp>
#include <module/module.h>
#include <core/unstr.h>
#include <core/polygons.h>

typedef size_t index_t;

class ReadFOAM: public vistle::Module
{
   const int NumPorts = 3;
 public:
      virtual bool compute();
      ReadFOAM(const std::string &shmname, int rank, int size, int moduleId);
      virtual ~ReadFOAM();

   private:
      vistle::Port *m_gridOut = nullptr, *m_boundOut;
      std::vector<vistle::Port *> m_dataOut;
      vistle::StringParameter *m_casedir = nullptr;
      vistle::FloatParameter *m_starttime, *m_stoptime;
      vistle::IntParameter *m_timeskip;
      std::vector<vistle::StringParameter *> m_fieldOut;

      std::map<double, std::string> m_timedirs; //Map of all the Time Directories
      std::map<std::string, int> m_fieldnames;
      std::map<std::string, int> m_varyingFields, m_constantFields;
      std::vector<std::string> fieldfiles;
      std::vector<std::string> m_meshfiles; // vector(points,faces,owners,neighbors)
      int m_numprocessors = 0;
      bool m_varyingGrid = false, m_varyingCoords = false;

      bool checkMeshDirectory(const std::string &meshdir, bool time);
      bool checkSubDirectory(const std::string &casedir, bool time);
      bool checkCaseDirectory(const std::string &casedir, bool compareOnly=false);

      bool readDirectory(const std::string &dir, int processor, int timestep);
      bool readConstant(const std::string &dir);
      bool readTime(const std::string &dir, int timestep);

      std::pair<vistle::UnstructuredGrid::ptr, vistle::Polygons::ptr> loadGrid(const std::string &dir);
      vistle::Object::ptr loadField(const std::string &dir, const std::string &field);
      bool loadFields(const std::string &dir, const std::map<std::string, int> &fields,
            int processor, int timestep);

      void setMeta(vistle::Object::ptr obj, int processor, int timestep) const;

      std::map<int, vistle::UnstructuredGrid::ptr> m_basegrid;
      std::map<int, vistle::Polygons::ptr> m_basebound;
};
#endif // READFOAM_H
