//-------------------------------------------------------------------------
// WRITE HDF5 H
// *
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------

#ifndef WRITEHDF5_H
#define WRITEHDF5_H

#include <string>
#include <unordered_map>

#include <module/module.h>

#include <core/vec.h>
#include <core/unstr.h>
#include <core/pointeroarchive.h>

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
   void compute_store(PointerOArchive & archive, vistle::Object::const_ptr data);
   void util_checkStatus(herr_t status);
   void debug_printArchive(PointerOArchive & archive);

   // private member variables
   vistle::StringParameter *m_fileName;
   unsigned m_numPorts;

   bool m_isRootNode;

   hid_t m_fileId;
   hid_t m_filePropertyListId;
   hid_t m_groupId_index;
   hid_t m_groupId_grid;
   hid_t m_groupId_data;


   // private member constants
   const int M_ROOT_NODE = 0;

   const std::unordered_map <std::type_index, hid_t> m_nativeTypeMap = {
         { typeid(int), H5T_NATIVE_INT },
         { typeid(unsigned int), H5T_NATIVE_UINT },

         { typeid(char), H5T_NATIVE_CHAR },
         { typeid(unsigned char), H5T_NATIVE_UCHAR },

         { typeid(short), H5T_NATIVE_SHORT },
         { typeid(unsigned short), H5T_NATIVE_USHORT },

         { typeid(long), H5T_NATIVE_LONG },
         { typeid(unsigned long), H5T_NATIVE_ULONG },
         { typeid(long long), H5T_NATIVE_LLONG },
         { typeid(unsigned long long), H5T_NATIVE_ULLONG },

         { typeid(float), H5T_NATIVE_FLOAT },
         { typeid(double), H5T_NATIVE_DOUBLE },
         { typeid(long double), H5T_NATIVE_LDOUBLE }

          // to implement? these are other accepted types
          //        H5T_NATIVE_B8
          //        H5T_NATIVE_B16
          //        H5T_NATIVE_B32
          //        H5T_NATIVE_B64

          //        H5T_NATIVE_OPAQUE
          //        H5T_NATIVE_HADDR
          //        H5T_NATIVE_HSIZE
          //        H5T_NATIVE_HSSIZE
          //        H5T_NATIVE_HERR
          //        H5T_NATIVE_HBOOL

   };

};

#endif /* WRITEHDF5_H */
