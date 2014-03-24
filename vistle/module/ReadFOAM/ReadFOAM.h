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
      vistle::StringParameter *m_casedir, *m_patches;
      vistle::FloatParameter *m_starttime, *m_stoptime;
      vistle::IntParameter *m_timeskip;
      std::vector<vistle::StringParameter *> m_fieldOut;
      std::vector<vistle::StringParameter *> m_field2dOut;
      //Ports
      vistle::Port *m_gridOut, *m_boundOut;
      std::vector<vistle::Port *> m_dataOut, m_data2dOut;

      CaseInfo m_case;

      std::vector<std::string> fieldChoices() const;

      bool parameterChanged(vistle::Parameter *p);
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
