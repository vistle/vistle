//
//This code is used for both IsoCut and IsoSurface!
//

#include <iostream>
#include <cmath>
#include <ctime>
#include <boost/mpi/collectives.hpp>
#include <core/message.h>
#include <core/object.h>
#include <core/unstr.h>
#include <core/rectilineargrid.h>
#include <core/vec.h>
#include "IsoSurface.h"
#include "Leveller.h"

MODULE_MAIN(IsoSurface)

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(PointOrValue,
                                    (Point)
                                    (Value)
)

IsoSurface::IsoSurface(const std::string &shmname, const std::string &name, int moduleID)
   : Module(
#ifdef CUTTINGSURFACE
"cut through grids"
#else
"extract isosurfaces"
#endif
, shmname, name, moduleID)
, isocontrol(this) {

   setDefaultCacheMode(ObjectCache::CacheDeleteLate);
#ifdef CUTTINGSURFACE
   m_mapDataIn = createInputPort("data_in");
#else
   setReducePolicy(message::ReducePolicy::PerTimestepZeroFirst);
   m_isovalue = addFloatParameter("isovalue", "isovalue", 0.0);
   m_isopoint = addVectorParameter("isopoint", "isopoint", ParamVector(0.0, 0.0, 0.0));
   m_pointOrValue = addIntParameter("Interactor", "point or value interaction", Value, Parameter::Choice);
   V_ENUM_SET_CHOICES(m_pointOrValue, PointOrValue);

   createInputPort("data_in");
   m_mapDataIn = createInputPort("mapdata_in");
#endif    
   m_dataOut = createOutputPort("data_out");

   m_processortype = addIntParameter("processortype", "processortype", 0, Parameter::Choice);
   V_ENUM_SET_CHOICES(m_processortype, ThrustBackend);

   m_paraMin = m_paraMax = 0.f;
}

IsoSurface::~IsoSurface() {

}

bool IsoSurface::changeParameter(const Parameter* param) {

    if (isocontrol.changeParameter(param))
        return true;

#ifndef CUTTINGSURFACE
    if (param == m_isopoint) {
        setParameter(m_pointOrValue, (Integer)Point);
        setReducePolicy(message::ReducePolicy::PerTimestepZeroFirst);
    } else if (param == m_isovalue) {
        setParameter(m_pointOrValue, (Integer)Value);
        setReducePolicy(message::ReducePolicy::Locally);
    } else if (param == m_pointOrValue) {
        if (m_pointOrValue->getValue() == Point)
            setReducePolicy(message::ReducePolicy::PerTimestepZeroFirst);
        else
            setReducePolicy(message::ReducePolicy::Locally);
    }
#endif
   return true;
}

bool IsoSurface::prepare() {

   m_grids.clear();
   m_datas.clear();
   m_mapdatas.clear();

   m_min = std::numeric_limits<Scalar>::max() ;
   m_max = -std::numeric_limits<Scalar>::max();
   return Module::prepare();
}

bool IsoSurface::reduce(int timestep) {

#ifndef CUTTINGSURFACE
   if (timestep <=0 && m_pointOrValue->getValue() == Point) {
       Scalar value = m_isovalue->getValue();
       int found = 0;
       Vector point = m_isopoint->getValue();
       for (size_t i=0; i<m_grids.size(); ++i) {
           if (m_datas[i]->getTimestep() == 0 || m_datas[i]->getTimestep() == -1) {
               auto gi = m_grids[i]->getInterface<GridInterface>();
               Index cell = gi->findCell(point);
               if (cell != InvalidIndex) {
                   found = 1;
                   auto interpol = gi->getInterpolator(cell, point);
                   value = interpol(m_datas[i]->x());
               }
           }
       }
       int numFound = boost::mpi::all_reduce(comm(), found, std::plus<int>());
       if (m_rank == 0 && numFound > 1) {
           sendWarning("found isopoint in %d blocks", numFound);
       }
       int valRank = found ? m_rank : m_size;
       valRank = boost::mpi::all_reduce(comm(), valRank, boost::mpi::minimum<int>());
       if (valRank < m_size) {
           boost::mpi::broadcast(comm(), value, valRank);
           setParameter(m_isovalue, (Float)value);
           setParameter(m_pointOrValue, (Integer)Point);
       }
   }

   for (size_t i=0; i<m_grids.size(); ++i) {
       int t = m_grids[i] ? m_grids[i]->getTimestep() : -1;
       if (m_datas[i])
           t = std::max(t, m_datas[i]->getTimestep());
       if (m_mapdatas[i])
           t = std::max(t, m_mapdatas[i]->getTimestep());
       if (t == timestep)
           work(m_grids[i], m_datas[i], m_mapdatas[i]);
   }

   Scalar min, max;
   boost::mpi::all_reduce(comm(),
                          m_min, min, boost::mpi::minimum<Scalar>());
   boost::mpi::all_reduce(comm(),
                          m_max, max, boost::mpi::maximum<Scalar>());

   if (m_paraMin != (Float)min || m_paraMax != (Float)max)
      setParameterRange(m_isovalue, (Float)min, (Float)max);

   m_paraMax = max;
   m_paraMin = min;
#endif

   return Module::reduce(timestep);
}

bool IsoSurface::work(vistle::Object::const_ptr grid,
             vistle::Vec<vistle::Scalar>::const_ptr dataS,
             vistle::DataBase::const_ptr mapdata) {

   const int processorType = getIntParameter("processortype");
#ifdef CUTTINGSURFACE
   const Scalar isoValue = 0.0;
#else
   const Scalar isoValue = getFloatParameter("isovalue");
#endif

   auto unstr = UnstructuredGrid::as(grid);
   auto rect = RectilinearGrid::as(grid);

   Leveller l = unstr ? Leveller(isocontrol, unstr, isoValue, processorType)
                      : Leveller(isocontrol, rect, isoValue, processorType);

#ifndef CUTTINGSURFACE
   l.setIsoData(dataS);
#endif
   if(mapdata){
      l.addMappedData(mapdata);
   };
   l.process();

#ifndef CUTTINGSURFACE
   auto minmax = dataS->getMinMax();
   if (minmax.first[0] < m_min)
      m_min = minmax.first[0];
   if (minmax.second[0] > m_max)
      m_max = minmax.second[0];
#endif

   Object::ptr result = l.result();
   DataBase::ptr mapresult = l.mapresult();
   if (result) {
#ifndef CUTTINGSURFACE
      result->copyAttributes(dataS);
#endif
      result->copyAttributes(grid, false);
      result->setTransform(grid->getTransform());
      if (result->getTimestep() < 0) {
          result->setTimestep(grid->getTimestep());
          result->setNumTimesteps(grid->getNumTimesteps());
      }
      if (result->getBlock() < 0) {
          result->setBlock(grid->getBlock());
          result->setNumBlocks(grid->getNumBlocks());
      }
      if (mapdata && mapresult) {
         mapresult->copyAttributes(mapdata);
         mapresult->setGrid(result);
         addObject(m_dataOut, mapresult);
      }
#ifndef CUTTINGSURFACE
      else {
          addObject(m_dataOut, result);
      }
#endif
   }
   return true;
}

bool IsoSurface::compute() {

#ifdef CUTTINGSURFACE
   auto mapdata = expect<DataBase>(m_mapDataIn);
   if (!mapdata)
       return true;
   auto grid = mapdata->grid();
   auto unstr = UnstructuredGrid::as(mapdata->grid());
   auto rect = RectilinearGrid::as(mapdata->grid());
#else
   auto mapdata = accept<DataBase>(m_mapDataIn);
   auto dataS = expect<Vec<Scalar>>("data_in");
   if (!dataS)
      return true;
   if (dataS->guessMapping() != DataBase::Vertex) {
      sendError("need per-vertex mapping on data_in");
      return true;
   }
   auto grid = dataS->grid();
   auto unstr = UnstructuredGrid::as(dataS->grid());
   auto rect = RectilinearGrid::as(dataS->grid());
#endif
   if (!unstr && !rect) {
       sendError("grid required on input data");
       return true;
   }

#ifdef CUTTINGSURFACE
    return work(grid, nullptr, mapdata);
#else
    if (m_pointOrValue->getValue() == Value) {
        return work(grid, dataS, mapdata);
    } else {
        //unstr->getCelltree();
        m_grids.push_back(grid);
        m_datas.push_back(dataS);
        m_mapdatas.push_back(mapdata);
        return true;
    }
#endif
}
