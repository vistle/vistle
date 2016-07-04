#include <core/object.h>
#include <core/unstr.h>
#include <core/structuredgrid.h>
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

   const GridInterface *grid = nullptr;
   UnstructuredGrid::const_ptr unstr = accept<UnstructuredGrid>("data_in");
   StructuredGridBase::const_ptr strb = accept<StructuredGridBase>("data_in");
   Object::const_ptr gridObj;
   if (unstr) {
       grid = dynamic_cast<const GridInterface *>(unstr.get());
       gridObj = unstr;
   } else if (strb) {
       grid = dynamic_cast<const GridInterface *>(strb.get());
       gridObj = boost::static_pointer_cast<const Object>(strb);
   } else {
       sendInfo("Unstructured or structured grid required");
       return true;
   }

   if (m_createCelltree->getValue()) {
      if (unstr) {
          unstr->getCelltree();
          if (!unstr->validateCelltree()) {
              std::cerr << "UnstructuredGrid celltree validation failed" << std::endl;
          }
      } else if (strb) {
          if (auto str = StructuredGrid::as(gridObj)) {
              str->getCelltree();
              str->getCelltree();
              if (!str->validateCelltree()) {
                  std::cerr << "StructuredGrid celltree validation failed" << std::endl;
              }
          }
      }
   }
   Index idx = grid->findCell(point);
   if (idx != InvalidIndex) {
      m_block->setValue(gridObj->getBlock());
      m_cell->setValue(idx);

      const auto bounds = grid->cellBounds(idx);
      const auto &min = bounds.first, &max = bounds.second;

      std::cerr << "found cell " << idx << " (block " << gridObj->getBlock() << "): bounds: min " << min.transpose() << ", max " << max.transpose() << std::endl;
   }

   return true;
}
