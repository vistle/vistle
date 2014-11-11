#include <sstream>
#include <iostream>
#include <limits>
#include <algorithm>

#include <boost/mpl/for_each.hpp>
#include <boost/mpi/collectives/all_reduce.hpp>
#include <boost/mpi/collectives/all_gather.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/mpi/collectives/broadcast.hpp>
#include <boost/mpi/packed_iarchive.hpp>
#include <boost/mpi/packed_oarchive.hpp>
#include <boost/mpi.hpp>
#include <boost/mpi/operations.hpp>

#include <core/vec.h>
#include <module/module.h>
#include <core/scalars.h>
#include <core/paramvector.h>
#include <core/message.h>
#include <core/coords.h>
#include <core/lines.h>
#include <core/unstr.h>
#include <core/points.h>
#include "Tracer.h"
#include <core/celltree.h>
#include <eigen3/Eigen/Geometry>
#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Dense>
#include <math.h>

MODULE_MAIN(Tracer)


using namespace vistle;


Tracer::Tracer(const std::string &shmname, const std::string &name, int moduleID)
   : Module("Tracer", shmname, name, moduleID) {

   setDefaultCacheMode(ObjectCache::CacheAll);
   setReducePolicy(message::ReducePolicy::OverAll);

    createInputPort("grid_in");
    createInputPort("data_in0");
    createInputPort("data_in1");
    createOutputPort("geom_out");
    createOutputPort("data_out0");

    addVectorParameter("startpoint1", "1st initial point", ParamVector(0,0.2,0));
    addVectorParameter("startpoint2", "2nd initial point", ParamVector(1,0,0));
    addIntParameter("no_startp", "number of startpoints", 2);
    setParameterRange("no_startp", (Integer)0, (Integer)10000);
    addIntParameter("steps_max", "maximum number of timesteps per particle", 1000);
    addIntParameter("steps_comm", "number of timesteps until communication", 10);
    IntParameter* tasktype = addIntParameter("taskType", "task type", 0, Parameter::Choice);
    std::vector<std::string> choices;
    choices.push_back("streamlines");
    setParameterChoices(tasktype, choices);
    addFloatParameter("timestep", "timestep for integration", 1e-04);
}

Tracer::~Tracer() {
}


class BlockData{

private:
    Index m_index;
    UnstructuredGrid::const_ptr m_grid;
    Vec<Scalar, 3>::const_ptr m_data0;
    Vec<Scalar>::const_ptr m_data1;
    Lines::ptr m_lines;
    Vec<Scalar, 3>::ptr m_datainterp0;
    Vec<Scalar>::ptr m_datainterp1;

public:
    BlockData(Index i, UnstructuredGrid::const_ptr grid, Vec<Scalar, 3>::const_ptr data0, Vec<Scalar>::const_ptr data1):
        m_index(i),
        m_grid(grid),
        m_data0(data0),
        m_data1(data1),
        m_lines(new Lines(Object::Initialized)){
    }

    UnstructuredGrid::const_ptr getGrid(){
        return m_grid;
    }

    Vec<Scalar, 3>::const_ptr getVectorData(){
        return m_data0;
    }

    Vec<Scalar>::const_ptr getScalarData(){
        return m_data1;
    }

    void addGeom(std::vector<Vector3> points,
                std::vector<Vector3> v_data,
                std::vector<Scalar> s_data,
                Index tasktype){

        switch(tasktype){
        case 1:
        {
            Index size0 = m_lines->getNumVertices();
            Index numpoints = points.size();
            Index size1 = size0 + numpoints;

            m_lines->x().resize(size1);
            m_lines->y().resize(size1);
            m_lines->z().resize(size1);
            m_lines->cl().resize(size1);

            for(Index i=size0; i<size1; i++){

                m_lines->x()[i] = points[i-size0](0);
                m_lines->y()[i] = points[i-size0](1);
                m_lines->z()[i] = points[i-size0](2);
                m_lines->cl()[i] = i;
            }
            m_lines->el().push_back(size1);
        }
        }
    }
};


class Particle{

private:
    Index m_index;
    Index m_step;
    Vector3 m_point;
    std::vector<Vector3> m_points;
    Vector3 m_veloc;
    std::vector<Vector3> m_velocs;
    std::vector<Scalar> m_scalars;
    Scalar m_timestep;
    BlockData* m_block;
    bool m_in;
    bool m_out;
    bool m_ingrid;
    Index m_cell;

public:
    Particle(Index i, Vector3 p, Scalar dt):
        m_index(i),
        m_point(p),
        m_timestep(dt),
        m_in(true),
        m_out(false),
        m_ingrid(true),
        m_block(nullptr),
        m_veloc(Vector3(1,0,0)),
        m_cell(InvalidIndex){
    }

    bool isInside(){
        if(m_ingrid && m_block){
            true;
        }
    }

    bool inGrid(){
        return m_ingrid;
    }

    bool leftNode(){
        return m_out;
    }

    void Deactivate(){
        m_block = nullptr;
        m_ingrid = false;
    }

    void setStatus(std::vector<BlockData*> blocks, Index maxstep, Index tasktype){

        if(m_ingrid){
            return;
        }

        if(m_step==maxstep){
            m_block->addGeom(m_points, m_velocs, m_scalars, tasktype);
            m_points.clear();
            m_velocs.clear();
            m_scalars.clear();
            m_out = true;
            this->Deactivate();
            return;
        }

        bool moving = (m_veloc(0)!=0 || m_veloc(1)!=0 || m_veloc(2)!=0);
        if(!moving){
            m_block->addGeom(m_points, m_velocs, m_scalars, tasktype);
            m_points.clear();
            m_velocs.clear();
            m_scalars.clear();
            m_out = true;
            this->Deactivate();
            return;
        }

        if(m_block){
            UnstructuredGrid::const_ptr grid = m_block->getGrid();
            if(grid->inside(m_cell, m_point)){
                return;
            }
            m_cell = grid->findCell(m_point);
            if(m_cell==InvalidIndex){
                m_block->addGeom(m_points, m_velocs, m_scalars, tasktype);
                m_out = true;
                m_in = true;
            }
            return;
        }

        if(m_in){
            for(Index i=0; i<blocks.size(); i++){
                UnstructuredGrid::const_ptr grid = blocks[i]->getGrid();
                m_cell = grid->findCell(m_point);
                if(m_cell!=InvalidIndex){
                    m_block = blocks[i];
                    m_points.push_back(m_point);
                    break;
                }
            }
            m_in = false;
        }
    }

    void Interpolation(){

        UnstructuredGrid::const_ptr grid = m_block->getGrid();
        Vec<Scalar, 3>::const_ptr vdata = m_block->getVectorData();
        Vec<Scalar>::const_ptr sdata = m_block->getScalarData();
        UnstructuredGrid::Interpolator interpolator = grid->getInterpolator;

        Scalar* u = vdata->x().data();
        Scalar* v = vdata->y().data();
        Scalar* u = vdata->z().data();

        m_veloc = interpolator(u,v,w);
        m_velocs.push_back(m_veloc);
    }

    void Integration(){


    }
};


class Integrator{

private:
    bool m_adaptive;
    Scalar m_stepsize;
    Scalar m_eps;
    const double m_c[2];
    const double m_c[3];

}


