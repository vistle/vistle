//-------------------------------------------------------------------------
// WRITE HDF5 H
// *
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------

#ifndef WRITEHDF5_H
#define WRITEHDF5_H

#include <string>

#include <module/module.h>

#include <core/vec.h>
#include <core/unstr.h>

#include "hdf5.h"

//-------------------------------------------------------------------------
// WRITE HDF5 CLASS DECLARATION
//-------------------------------------------------------------------------
class WriteHDF5 : public vistle::Module {
 public:
    friend class boost::serialization::access;

   WriteHDF5(const std::string &shmname, const std::string &name, int moduleID);
   ~WriteHDF5();

 private:

   // overriden functions
   virtual bool parameterChanged(const vistle::Parameter * p);
   virtual bool prepare();
   virtual bool compute();
   virtual bool reduce(int timestep);

   // private helper functions
   void compute_store(vistle::UnstructuredGrid::const_ptr dataGrid, vistle::DataBase::const_ptr data);

   // private member variables
   vistle::StringParameter *m_fileName;
   unsigned m_numPorts;

   bool m_isRootNode;

   hid_t m_fileId;
   hid_t m_filePropertyListId;

   // private member constants
   const int M_ROOT_NODE = 0;

};

#endif /* WRITEHDF5_H */
