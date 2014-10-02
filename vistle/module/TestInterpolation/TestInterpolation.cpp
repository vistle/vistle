#include <core/object.h>
#include <core/unstr.h>
#include <core/message.h>
#include "TestInterpolation.h"

MODULE_MAIN(TestInterpolation)

using namespace vistle;

TestInterpolation::TestInterpolation(const std::string &shmname, int rank, int size, int moduleID)
    : Module("test interpolation", shmname, rank, size, moduleID) {

    setDefaultCacheMode(ObjectCache::CacheAll);

    createInputPort("data_in");

    m_count = addIntParameter("count", "number of randoim points to generate per block", 100);
    m_createCelltree = addIntParameter("create_celltree", "create celltree", 0, Parameter::Boolean);
}

TestInterpolation::~TestInterpolation() {

}

namespace {
   Vector randpoint(const Vector &min, const Vector &max) {
      Vector point;
      for (int c=0; c<3; ++c) {
         point[c] = drand48() * (max[c]-min[c]) + min[c];
      }
      return point;
   }
}

bool TestInterpolation::compute() {

   const Index count = getIntParameter("count");

   while (hasObject("data_in")) {

      UnstructuredGrid::const_ptr grid = UnstructuredGrid::as(takeFirstObject("data_in"));
      if (!grid)
         continue;

      if (m_createCelltree->getValue()) {
         grid->getCelltree();
         if (!grid->validateCelltree()) {
            std::cerr << "celltree validation failed" << std::endl;
         }
      }

      auto bounds = grid->getBounds();
      Vector min = bounds.first, max = bounds.second;
      const Scalar *x = grid->x().data();
      const Scalar *y = grid->y().data();
      const Scalar *z = grid->z().data();

      Index numChecked = 0;
      Scalar squaredError = 0;
      for (Index i=0; i<count; ++i) {
         Vector point(randpoint(min, max));
         Index idx = grid->findCell(point);
         if (idx != InvalidIndex) {
            ++numChecked;
            UnstructuredGrid::Interpolator interpol = grid->getInterpolator(idx, point);
            Vector p = interpol(x, y, z);
            Scalar d2 = (point-p).squaredNorm();
            if (d2 > 0.01) {
               std::cerr << "point: " << point.transpose() << ", recons: " << p.transpose() << std::endl;
            }
            squaredError += d2;
         }
      }
      std::cerr << "block " << grid->getBlock() << ", bounds: min " << min.transpose() << ", max " << max.transpose() << ", checked: " << numChecked << ", avg error: " << squaredError/numChecked << std::endl;
   }

   return true;
}
