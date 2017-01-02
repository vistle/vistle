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
m_lines(new Lines(Object::Initialized)),
m_ids(new Vec<Index>(Object::Initialized)),
m_steps(new Vec<Index>(Object::Initialized)),
m_times(new Vec<Scalar>(Object::Initialized)),
m_dists(new Vec<Scalar>(Object::Initialized)),
m_vx(nullptr),
m_vy(nullptr),
m_vz(nullptr),
m_p(nullptr)
{
   m_ivec.emplace_back(new Vec<Scalar, 3>(Object::Initialized));

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
      m_iscal.emplace_back(new Vec<Scalar>(Object::Initialized));
      m_scamap = m_scafld->guessMapping();
      if (m_scamap == DataBase::Unspecified)
          m_scamap = DataBase::Vertex;
   }
}

BlockData::~BlockData(){}

void BlockData::setMeta(const vistle::Meta &meta) {

   m_lines->setMeta(meta);
   if (m_ids)
      m_ids->setMeta(meta);
   if (m_steps)
      m_steps->setMeta(meta);
   if (m_times)
       m_times->setMeta(meta);
   if (m_dists)
       m_dists->setMeta(meta);
   for (auto &v: m_ivec) {
       if (v)
          v->setMeta(meta);
   }
   for (auto &s: m_iscal) {
       if (s)
          s->setMeta(meta);
   }
}

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

Lines::ptr BlockData::getLines(){
    return m_lines;
}

std::vector<Vec<Scalar, 3>::ptr> BlockData::getIplVec(){
    return m_ivec;
}

std::vector<Vec<Scalar>::ptr> BlockData::getIplScal(){
    return m_iscal;
}

Vec<Index>::ptr BlockData::ids() const {
   return m_ids;
}

Vec<Index>::ptr BlockData::steps() const {
   return m_steps;
}

Vec<Scalar>::ptr BlockData::times() const {
   return m_times;
}

Vec<Scalar>::ptr BlockData::distances() const {
   return m_dists;
}

void BlockData::addLines(Index id, const std::vector<Vector3> &points,
             const std::vector<Vector3> &velocities,
             const std::vector<Scalar> &pressures,
             const std::vector<Index> &steps,
             const std::vector<Scalar> &times,
             const std::vector<Scalar> &dists) {

    std::lock_guard<std::mutex> locker(m_mutex);

//#pragma omp critical
    {
       Index numpoints = points.size();
       assert(numpoints == velocities.size());
       assert(!m_p || pressures.size()==numpoints);

       size_t newSize = m_lines->x().size()+numpoints;
       m_lines->x().reserve(newSize);
       m_lines->y().reserve(newSize);
       m_lines->z().reserve(newSize);
       m_ivec[0]->x().reserve(newSize);
       m_ivec[0]->y().reserve(newSize);
       m_ivec[0]->z().reserve(newSize);
       m_ids->x().reserve(newSize);
       m_steps->x().reserve(newSize);
       m_times->x().reserve(newSize);
       m_dists->x().reserve(newSize);
       if (m_p) {
           m_iscal[0]->x().reserve(newSize);
       }

       for(Index i=0; i<numpoints; i++) {

          m_lines->x().push_back(points[i](0));
                m_lines->y().push_back(points[i](1));
                m_lines->z().push_back(points[i](2));
                Index numcorn = m_lines->getNumCorners();
          m_lines->cl().push_back(numcorn);

          m_ivec[0]->x().push_back(velocities[i](0));
                m_ivec[0]->y().push_back(velocities[i](1));
                m_ivec[0]->z().push_back(velocities[i](2));

                if (m_p) {
             m_iscal[0]->x().push_back(pressures[i]);
          }

          m_ids->x().push_back(id);
          m_steps->x().push_back(steps[i]);
          m_times->x().push_back(times[i]);
          m_dists->x().push_back(dists[i]);
       }
       Index numcorn = m_lines->getNumCorners();
       m_lines->el().push_back(numcorn);
    }
}
