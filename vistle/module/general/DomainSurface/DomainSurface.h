#ifndef DOMAINSURFACE_H
#define DOMAINSURFACE_H

#include <module/module.h>
#include <core/unstr.h>
#include <core/polygons.h>

class DomainSurface: public vistle::Module {

 public:
   DomainSurface(const std::string &name, int moduleID, mpi::communicator comm);
   ~DomainSurface();

private:
   vistle::UnstructuredGrid::const_ptr m_grid_in;
   vistle::Polygons::ptr m_grid_out;
   std::map<vistle::Index,vistle::Index> m_verticesMapping;
   virtual bool compute();
   bool createSurface();
   //bool checkNormal(vistle::Index v1, vistle::Index v2, vistle::Index v3, vistle::Scalar x_center, vistle::Scalar y_center, vistle::Scalar z_center);
};

#endif
