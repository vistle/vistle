#include <core/object.h>
#include <core/unstr.h>
#include <core/message.h>
#include "TestCellSearch.h"

MODULE_MAIN(TestCellSearch)

using namespace vistle;

TestCellSearch::TestCellSearch(const std::string &shmname, const std::string &name, int moduleID)
    : Module("test celltree", shmname, name, moduleID) {

    setDefaultCacheMode(ObjectCache::CacheDeleteLate);

    createInputPort("data_in");

    m_point = addVectorParameter("point", "search point", ParamVector(0., 0., 0.));
    m_block = addIntParameter("block", "number of containing block", -1);
    m_cell = addIntParameter("cell", "number of containing cell", -1);
    m_createCelltree = addIntParameter("create_celltree", "create celltree", 0, Parameter::Boolean);
}

TestCellSearch::~TestCellSearch() {

}

bool TestCellSearch::compute() {

   const Vector point = m_point->getValue();

   UnstructuredGrid::const_ptr grid = expect<UnstructuredGrid>("data_in");
   if (!grid)
      return false;

   if (m_createCelltree->getValue()) {
      grid->getCelltree();
      if (!grid->validateCelltree()) {
         std::cerr << "celltree validation failed" << std::endl;
      }
   }
   Index idx = grid->findCell(point);
   if (idx != InvalidIndex) {
      m_block->setValue(grid->getBlock());
      m_cell->setValue(idx);

      const Scalar m = std::numeric_limits<Scalar>::max();
      Vector min(m, m, m), max(-m, -m, -m);
      auto cellstart = grid->el()[idx], cellend = grid->el()[idx+1];
      for (Index i=cellstart; i<cellend; ++i) {
         Index vert = grid->cl()[i];
         Vector v(grid->x()[vert], grid->y()[vert], grid->z()[vert]);
         for (int i=0; i<3; ++i) {
            if (v[i] < min[i])
               min[i] = v[i];
            if (v[i] > max[i])
               max[i] = v[i];
         }
      }

      std::cerr << "found cell " << idx << " (block " << grid->getBlock() << "): bounds: min " << min.transpose() << ", max " << max.transpose() << std::endl;
   }

   return true;
}
