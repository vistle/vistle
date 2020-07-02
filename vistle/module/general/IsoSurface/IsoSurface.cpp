﻿//
//This code is used for both IsoCut and IsoSurface!
//

#include <iostream>
#include <cmath>
#include <ctime>
#include <boost/mpi/collectives.hpp>
#include <core/message.h>
#include <core/object.h>
#include <core/unstr.h>
#include <core/uniformgrid.h>
#include <core/rectilineargrid.h>
#include <core/structuredgrid.h>
#include <core/vec.h>

#ifdef CUTTINGSURFACE
#define IsoSurface CuttingSurface
#endif


#include "IsoSurface.h"
#include "Leveller.h"

#ifdef CUTTINGSURFACE
MODULE_MAIN(CuttingSurface)
#else
MODULE_MAIN(IsoSurface)
#endif

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(PointOrValue,
                                    (PointPerTimestep)
                                    (Value)
                                    (PointInFirstStep)
)

IsoSurface::IsoSurface(const std::string &name, int moduleID, mpi::communicator comm)
   : Module(
#ifdef CUTTINGSURFACE
"cut through grids"
#else
"extract isosurfaces"
#endif
, name, moduleID, comm)
, isocontrol(this) {

   isocontrol.init();

   setDefaultCacheMode(ObjectCache::CacheDeleteLate);
#ifdef CUTTINGSURFACE
   m_mapDataIn = createInputPort("data_in");
#else
   m_isovalue = addFloatParameter("isovalue", "isovalue", 0.0);
   m_isopoint = addVectorParameter("isopoint", "isopoint", ParamVector(0.0, 0.0, 0.0));
   setReducePolicy(message::ReducePolicy::Locally);
   m_pointOrValue = addIntParameter("point_or_value", "point or value interaction", Value, Parameter::Choice);
   V_ENUM_SET_CHOICES(m_pointOrValue, PointOrValue);

   createInputPort("data_in");
   m_mapDataIn = createInputPort("mapdata_in");
#endif    
   m_dataOut = createOutputPort("data_out");

   m_processortype = addIntParameter("processortype", "processortype", 0, Parameter::Choice);
   V_ENUM_SET_CHOICES(m_processortype, ThrustBackend);

   m_computeNormals = addIntParameter("compute_normals", "compute normals (structured grids only)", 1, Parameter::Boolean);

   m_paraMin = m_paraMax = 0.f;
}

IsoSurface::~IsoSurface() = default;

bool IsoSurface::changeParameter(const Parameter* param) {

    bool ok = isocontrol.changeParameter(param);

#ifndef CUTTINGSURFACE
    if (param == m_pointOrValue) {
        if (m_pointOrValue->getValue() == PointInFirstStep)
            setReducePolicy(message::ReducePolicy::PerTimestepZeroFirst);
        else if (m_pointOrValue->getValue() == PointPerTimestep)
            setReducePolicy(message::ReducePolicy::PerTimestep);
        else
            setReducePolicy(message::ReducePolicy::Locally);
    }
#endif

   return Module::changeParameter(param) && ok;
}

bool IsoSurface::prepare() {

   m_blocksForTime.clear();

   m_min = std::numeric_limits<Scalar>::max() ;
   m_max = -std::numeric_limits<Scalar>::max();

   m_performedPointSearch = false;
   m_foundPoint = false;

   return Module::prepare();
}

bool IsoSurface::reduce(int timestep) {

#ifndef CUTTINGSURFACE
    if (rank() == 0)
        std::cerr << "IsoSurface::reduce(" << timestep << ")" << std::endl;
    Scalar value = m_isovalue->getValue();
    Vector point = m_isopoint->getValue();

    std::unique_lock<std::mutex> lock(m_mutex);
    std::vector<BlockData> &blocks = m_blocksForTime[timestep];
    lock.unlock();
    if ((m_pointOrValue->getValue() == PointInFirstStep && !m_performedPointSearch && timestep <= 0)
        || m_pointOrValue->getValue() == PointPerTimestep) {
        int found = 0;
        const int numBlocks = blocks.size();
#pragma omp parallel for
        for (int i=0; i<numBlocks; ++i) {
            const auto &b = blocks[i];
            auto gi = b.grid->getInterface<GridInterface>();
            Index cell = gi->findCell(point, InvalidIndex, GridInterface::ForceCelltree);
            if (cell != InvalidIndex) {
                ++found;
                auto interpol = gi->getInterpolator(cell, point);
                value = interpol(b.datas->x());
            }
        }

        int numFound = boost::mpi::all_reduce(comm(), found, std::plus<int>());
        m_foundPoint = numFound>0;
        if (m_rank == 0) {
            if (numFound == 0) {
                if (m_pointOrValue->getValue()!=PointPerTimestep || (m_performedPointSearch && timestep != -1)) {
                    sendInfo("iso-value point out of domain for timestep %d", timestep);
                }
            } else if (numFound > 1) {
                sendWarning("found isopoint in %d blocks", numFound);
            }
        }
        int valRank = found ? m_rank : m_size;
        valRank = boost::mpi::all_reduce(comm(), valRank, boost::mpi::minimum<int>());
        if (valRank < m_size) {
            boost::mpi::broadcast(comm(), value, valRank);
            if (m_pointOrValue->getValue() == PointInFirstStep)
                setParameter(m_isovalue, (Float)value);
        }
        m_performedPointSearch = true;
    }

    if (m_foundPoint) {
        int numProcessed = 0;
        for (const auto &b: blocks) {
            ++numProcessed;
            auto obj = work(b.grid, b.datas, b.mapdata, value);
            addObject(m_dataOut, obj);
        }
    }

    if (timestep == -1) {
        Scalar min, max;
        boost::mpi::all_reduce(comm(),
                               m_min, min, boost::mpi::minimum<Scalar>());
        boost::mpi::all_reduce(comm(),
                               m_max, max, boost::mpi::maximum<Scalar>());

        if (max >= min) {
            if (m_paraMin != (Float)min || m_paraMax != (Float)max)
                setParameterRange(m_isovalue, (Float)min, (Float)max);

            m_paraMax = max;
            m_paraMin = min;
        }
    }
#endif

    return Module::reduce(timestep);
}

Object::ptr IsoSurface::work(vistle::Object::const_ptr grid,
             vistle::Vec<vistle::Scalar>::const_ptr dataS,
             vistle::DataBase::const_ptr mapdata,
             Scalar isoValue) const {

   const int processorType = getIntParameter("processortype");

   Leveller l(isocontrol, grid, isoValue, processorType);
   l.setComputeNormals(m_computeNormals->getValue());

#ifndef CUTTINGSURFACE
   l.setIsoData(dataS);
#endif
   if(mapdata){
      l.addMappedData(mapdata);
   }
   l.process();

#ifndef CUTTINGSURFACE
   auto minmax = dataS->getMinMax();
   {
       std::lock_guard<std::mutex> guard(m_mutex);
       if (minmax.first[0] < m_min)
           m_min = minmax.first[0];
       if (minmax.second[0] > m_max)
           m_max = minmax.second[0];
   }
#endif

   Object::ptr result = l.result();
   DataBase::ptr mapresult = l.mapresult();
   if (result && !result->isEmpty()) {
#ifndef CUTTINGSURFACE
      result->copyAttributes(dataS);
#endif
      result->updateInternals();
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
         mapresult->updateInternals();
         mapresult->copyAttributes(mapdata);
         mapresult->setGrid(result);
         return mapresult;
      }
#ifndef CUTTINGSURFACE
      else {
          return result;
      }
#endif
   }
   return Object::ptr();
}


bool IsoSurface::compute(std::shared_ptr<PortTask> task) const {

#ifdef CUTTINGSURFACE
    auto mapdata = task->expect<DataBase>(m_mapDataIn);
   if (!mapdata)
       return true;
   auto grid = mapdata->grid();
#else
    auto mapdata = task->accept<DataBase>(m_mapDataIn);
    auto dataS = task->expect<Vec<Scalar>>("data_in");
   if (!dataS)
      return true;
   if (dataS->guessMapping() != DataBase::Vertex) {
       sendError("need per-vertex mapping on data_in");
      return true;
   }
   auto grid = dataS->grid();
   if (mapdata) {
       if (!mapdata->grid()) {
           sendError("no grid for mapped data");
           return true;
       }
       if (mapdata->grid() != grid) {
           sendError("grids on mapped data and iso-data do not match");
           return true;
       }
   }
#endif
   auto uni = UniformGrid::as(grid);
   auto rect = RectilinearGrid::as(grid);
   auto str = StructuredGrid::as(grid);
   auto unstr = UnstructuredGrid::as(grid);
   auto tri = Triangles::as(grid);
   auto quad = Quads::as(grid);
   auto poly = Polygons::as(grid);
   if (!uni && !rect && !str && !unstr && !tri && !quad && !poly) {
       if (grid)
           sendError("grid required on input data: invalid type");
       else
           sendError("grid required on input data: none present");
       return true;
   }

#ifdef CUTTINGSURFACE
    auto obj = work(grid, nullptr, mapdata);
    task->addObject(m_dataOut, obj);
    return true;
#else
    if (m_pointOrValue->getValue() == Value) {
        auto obj = work(grid, dataS, mapdata, m_isovalue->getValue());
        task->addObject(m_dataOut, obj);
        return true;
    } else {
        std::lock_guard<std::mutex> guard(m_mutex);
        BlockData b(grid, dataS, mapdata);
        int t = b.getTimestep();
        m_blocksForTime[t].emplace_back(b);
        return true;
    }
#endif
}

int IsoSurface::BlockData::getTimestep() const {

    int t = vistle::getTimestep(datas);
    if (t < 0)
        t = vistle::getTimestep(grid);
    if (t < 0)
        t = vistle::getTimestep(mapdata);
    return t;
}
