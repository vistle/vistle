#include "BlockData.h"
#include "Integrator.h"
#include <core/vec.h>
#include <mutex>


using namespace vistle;

BlockData::BlockData(Index i,
          Object::const_ptr grid,
          Vec<Scalar, 3>::const_ptr vdata,
          Vec<Scalar>::const_ptr pdata):
m_grid(grid),
m_gridInterface(m_grid->getInterface<GridInterface>()),
m_vecfld(vdata),
m_scafld(pdata),
m_vecmap(DataBase::Vertex),
m_scamap(DataBase::Vertex),
m_vx(nullptr),
m_vy(nullptr),
m_vz(nullptr),
m_p(nullptr)
{
   if (m_vecfld) {
      m_vx = &m_vecfld->x()[0];
      m_vy = &m_vecfld->y()[0];
      m_vz = &m_vecfld->z()[0];

      m_vecmap = m_vecfld->guessMapping();
      if (m_vecmap == DataBase::Unspecified)
          m_vecmap = DataBase::Vertex;
   }

   if (m_scafld) {
      m_p = &m_scafld->x()[0];
      m_scamap = m_scafld->guessMapping();
      if (m_scamap == DataBase::Unspecified)
          m_scamap = DataBase::Vertex;
   }
}

BlockData::~BlockData(){}

const GridInterface *BlockData::getGrid() {
    return m_gridInterface;
}

Vec<Scalar, 3>::const_ptr BlockData::getVecFld(){
    return m_vecfld;
}

DataBase::Mapping BlockData::getVecMapping() const {
    return m_vecmap;
}

Vec<Scalar>::const_ptr BlockData::getScalFld(){
    return m_scafld;
}

DataBase::Mapping BlockData::getScalMapping() const {
    return m_scamap;
}
