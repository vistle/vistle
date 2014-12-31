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
#include <boost/chrono.hpp>


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
    createOutputPort("data_out1");

    addVectorParameter("startpoint1", "1st initial point", ParamVector(0,0.2,0));
    addVectorParameter("startpoint2", "2nd initial point", ParamVector(1,0,0));
    addIntParameter("no_startp", "number of startpoints", 2);
    setParameterRange("no_startp", (Integer)0, (Integer)10000);
    addIntParameter("steps_max", "maximum number of integrations per particle", 1000);
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
    const Index m_index;
    UnstructuredGrid::const_ptr m_grid;
    Vec<Scalar, 3>::const_ptr m_vdata;
    Vec<Scalar>::const_ptr m_pdata;
    Lines::ptr m_lines;
    std::vector<Vec<Scalar, 3>::ptr> m_v_interpol;
    std::vector<Vec<Scalar>::ptr> m_p_interpol;

public:

    BlockData(Index i,
              UnstructuredGrid::const_ptr grid,
              Vec<Scalar, 3>::const_ptr vdata,
              Vec<Scalar>::const_ptr pdata = nullptr):
        m_index(i),
        m_grid(grid),
        m_vdata(vdata),
        m_pdata(pdata),
        m_lines(new Lines(Object::Initialized)){
    }

    UnstructuredGrid::const_ptr getGrid(){
        return m_grid;
    }

    Vec<Scalar, 3>::const_ptr getVelocityData(){
        return m_vdata;
    }

    Vec<Scalar>::const_ptr getPressureData(){
        return m_pdata;
    }

    Lines::ptr getLines(){
        return m_lines;
    }

    std::vector<Vec<Scalar, 3>::ptr> getInterpolVelo(){
        return m_v_interpol;
    }

    std::vector<Vec<Scalar>::ptr> getInterpolPress(){
        return m_p_interpol;
    }

    void addData(const std::vector<Vector3> &points,
                 const std::vector<Vector3> &velocities,
                 const std::vector<Scalar> &pressures,
                 Index tasktype,
                 Index first_timestep =0){

        switch(tasktype){

        case 0:{

            Index numpoints = points.size();
            Scalar phi_max = 1e-03;

            if(m_v_interpol.size()==0){
                m_v_interpol.emplace_back(new Vec<Scalar, 3>(Object::Initialized));
            }

            if(pressures.size()==numpoints && m_p_interpol.size()==0){
                m_p_interpol.emplace_back(new Vec<Scalar>(Object::Initialized));
            }

            for(Index i=0; i<numpoints; i++){

                bool addpoint = true;
                if(i>1 && i<numpoints-1){

                    Vector3 vec1 = points[i-1]-points[i-2];
                    Vector3 vec2 = points[i]-points[i-1];
                    Scalar cos_phi = vec1.dot(vec2)/(vec1.norm()*vec2.norm());
                    Scalar phi = acos(cos_phi);
                    if(phi<phi_max){
                        addpoint = false;
                    }
                }

                if(addpoint){

                    m_lines->x().push_back(points[i](0));
                    m_lines->y().push_back(points[i](1));
                    m_lines->z().push_back(points[i](2));
                    Index numcorn = m_lines->getNumCorners();
                    m_lines->cl().push_back(numcorn);

                    m_v_interpol[0]->x().push_back(velocities[0](0));
                    m_v_interpol[0]->y().push_back(velocities[0](1));
                    m_v_interpol[0]->z().push_back(velocities[0](2));

                    if(pressures.size()==numpoints){
                        m_p_interpol[0]->x().push_back(pressures[0]);
                    }
                }
            }
            Index numcorn = m_lines->getNumCorners();
            m_lines->el().push_back(numcorn);
        }
        }
    }
};


class Particle{

private:
    const Index m_index;
    Vector3 m_position;
    Vector3 m_positionold;
    std::vector<Vector3> m_positions;
    Vector3 m_velocity;
    std::vector<Vector3> m_velocities;
    std::vector<Scalar> m_pressures;
    Index m_stepcount;
    BlockData* m_block;
    Index m_cell;
    bool m_ingrid;
    bool m_out;
    bool m_in;
    Scalar m_stepsize;

public:
    Particle(Index i, Vector3 pos, Scalar dt):
        m_index(i),
        m_position(pos),
        m_velocity(Vector3(1,0,0)),
        m_stepcount(0),
        m_block(nullptr),
        m_cell(InvalidIndex),
        m_ingrid(true),
        m_out(false),
        m_in(true),
        m_stepsize(dt){
    }

    bool isActive(){

        if(m_ingrid && m_block){
            return true;
        }
        return false;
    }

    bool inGrid(){

        return m_ingrid;
    }

    void setStatus(const std::vector<std::unique_ptr<BlockData>> &block, Index steps_max, Index tasktype){

        if(!m_ingrid){
            return;
        }

        if(m_stepcount == steps_max){

            m_block->addData(m_positions, m_velocities, m_pressures, tasktype);
            m_positions.clear();
            m_velocities.clear();
            m_pressures.clear();
            m_out = true;
            this->Deactivate();
            return;
        }

        bool moving = (m_velocity(0)!=0 || m_velocity(1)!=0 || 0 || m_velocity(2)!=0);
        if(!moving){

            m_block->addData(m_positions, m_velocities, m_pressures, tasktype);
            m_positions.clear();
            m_velocities.clear();
            m_pressures.clear();
            m_out = true;
            this->Deactivate();
            return;
        }

        if(m_block){

            UnstructuredGrid::const_ptr grid = m_block->getGrid();
            if(grid->inside(m_cell, m_position)){
                return;
            }
            m_cell = grid->findCell(m_position);
            if(m_cell==InvalidIndex){

                m_block->addData(m_positions, m_velocities, m_pressures, tasktype);
                m_positions.clear();
                m_velocities.clear();
                m_pressures.clear();
                m_block = nullptr;
                m_out = true;
                m_in = true;
                setStatus(block, steps_max, tasktype);
            }
            return;
        }

        if(m_in){

            for(Index i=0; i<block.size(); i++){

                UnstructuredGrid::const_ptr grid = block[i]->getGrid();
                m_cell = grid->findCell(m_position);
                if(m_cell!=InvalidIndex){

                    m_block = block[i].get();
                    if(m_stepcount!=0){
                        m_velocities.push_back(m_velocity);
                        m_positions.push_back(m_positionold);
                    }
                    m_positions.push_back(m_position);
                    m_out = false;
                    break;
                }
            }
            m_in = false;
            return;
        }
    }

    void Deactivate(){

        m_block = nullptr;
        m_ingrid = false;
    }

    void Interpolation(){

        UnstructuredGrid::const_ptr grid = m_block->getGrid();
        Vec<Scalar, 3>::const_ptr v_data = m_block->getVelocityData();
        Vec<Scalar>::const_ptr p_data = m_block->getPressureData();
        UnstructuredGrid::Interpolator interpolator = grid->getInterpolator(m_cell, m_position);
        Scalar* u = v_data->x().data();
        Scalar* v = v_data->y().data();
        Scalar* w = v_data->z().data();
        m_velocity = interpolator(u,v,w);
        m_velocities.push_back(m_velocity);
    }

    void Integration(){

        m_positionold = m_position;
        m_position = m_position + m_velocity*m_stepsize;
        m_positions.push_back(m_position);
        ++m_stepcount;
    }

    void Communicator(boost::mpi::communicator mpi_comm, Index root){

        boost::mpi::broadcast(mpi_comm, m_position, root);
        boost::mpi::broadcast(mpi_comm, m_positionold, root);
        boost::mpi::broadcast(mpi_comm, m_stepcount, root);
        boost::mpi::broadcast(mpi_comm, m_velocity, root);
        boost::mpi::broadcast(mpi_comm, m_ingrid, root);

        m_out = false;
        if(mpi_comm.rank()!=root){

            m_in = true;
        }
    }

    bool leftNode(){

        if(m_out){
            m_out = false;
            return true;
        }
        return false;
    }

};


bool Tracer::prepare(){

    grid_in.clear();
    data_in0.clear();
    data_in1.clear();

    return true;
}


bool Tracer::compute(){

    while(hasObject("grid_in") && hasObject("data_in0")){

        UnstructuredGrid::const_ptr grid = UnstructuredGrid::as((takeFirstObject("grid_in")));
        grid->getCelltree();
        grid_in.push_back(grid);
        data_in0.push_back(Vec<Scalar, 3>::as(takeFirstObject("data_in0")));
    }


    while(hasObject("data_in1")){

        Object::const_ptr datain1 = takeFirstObject(("data_in1"));
        if(Vec<Scalar>::as(datain1)){
            data_in1.push_back(Vec<Scalar>::as(datain1));
        }
    }

    return true;
}


bool Tracer::reduce(int timestep){

boost::chrono::high_resolution_clock::time_point st = boost::chrono::high_resolution_clock::now();
boost::chrono::high_resolution_clock::time_point et;
boost::chrono::high_resolution_clock::time_point sc;
boost::chrono::high_resolution_clock::time_point ec;
double ect = 0;

    Index numblocks = grid_in.size();

    //get parameters
    Index numpoints = getIntParameter("no_startp");
    Index steps_max = getIntParameter("steps_max");
    Scalar dt = getFloatParameter("timestep");
    Index task_type = getIntParameter("taskType");

    boost::mpi::communicator world;
    if(data_in1.size()<numblocks){

        data_in1.assign(numblocks, nullptr);
    }

    //determine startpoints
    std::vector<Vector3> startpoints(numpoints);
    Vector3 startpoint1 = getVectorParameter("startpoint1");
    Vector3 startpoint2 = getVectorParameter("startpoint2");
    Vector3 delta = (startpoint2-startpoint1)/(numpoints-1);
    for(Index i=0; i<numpoints; i++){

        startpoints[i] = startpoint1 + i*delta;
    }

    //create BlockData objects
    std::vector<std::unique_ptr<BlockData>> block(numblocks);
    for(Index i=0; i<numblocks; i++){

        block[i].reset(new BlockData(i, grid_in[i], data_in0[i], data_in1[i]));
    }

    //create particle objects
    std::vector<std::unique_ptr<Particle>> particle(numpoints);
    for(Index i=0; i<numpoints; i++){

        particle[i].reset(new Particle(i, startpoints[i], dt));
    }

    //find cell and set status of particle objects
    for(Index i=0; i<numpoints; i++){

        particle[i]->setStatus(block, steps_max, task_type);
    }


    for(Index i=0; i<numpoints; i++){

        bool active = particle[i]->isActive();
        active = boost::mpi::all_reduce(world, active, std::logical_or<bool>());
        if(!active){
            particle[i]->Deactivate();
        }
    }


    Index stepcount=0;
    bool ingrid = true;
    Index mpisize = world.size();
    std::vector<Index> sendlist(0);
    while(ingrid){

            for(Index i=0; i<numpoints; i++){

                while(particle[i]->isActive()){

                    particle[i]->Interpolation();
                    particle[i]->Integration();
                    particle[i]->setStatus(block, steps_max, task_type);
                    if(particle[i]->leftNode()){
                        sendlist.push_back(i);
                    }
                }
            }

sc = boost::chrono::high_resolution_clock::now();
            for(Index mpirank=0; mpirank<mpisize; mpirank++){

                Index num_send = sendlist.size();
                boost::mpi::broadcast(world, num_send, mpirank);

                if(num_send>0){
                    std::vector<Index> tmplist = sendlist;
                    boost::mpi::broadcast(world, tmplist, mpirank);
                    for(Index i=0; i<num_send; i++){
                        Index p_index = tmplist[i];
                        particle[p_index]->Communicator(world, mpirank);
                        particle[p_index]->setStatus(block, steps_max, task_type);
                        bool active = particle[p_index]->isActive();
                        active = boost::mpi::all_reduce(world, particle[p_index]->isActive(), std::logical_or<bool>());
                        if(!active){
                            particle[p_index]->Deactivate();
                        }
                    }
                }
ec = boost::chrono::high_resolution_clock::now();
ect = ect + 1e-9*boost::chrono::duration_cast<boost::chrono::nanoseconds>(ec-sc).count();
                ingrid = false;
                for(Index i=0; i<numpoints; i++){

                    if(particle[i]->inGrid()){

                        ingrid = true;
                        break;
                    }
                }
            }
            sendlist.clear();
    }

    switch(task_type){

    case 0:
        //add Lines-objects to output port
        for(Index i=0; i<numblocks; i++){

            Lines::ptr lines = block[i]->getLines();
            if(lines->getNumElements()>0){
                addObject("geom_out", lines);
            }

            std::vector<Vec<Scalar, 3>::ptr> v_vec = block[i]->getInterpolVelo();
            Vec<Scalar, 3>::ptr v;
            if(v_vec.size()>0){
                v = v_vec[0];
                addObject("data_out0", v);
            }

            if(data_in1[0]){
                std::vector<Vec<Scalar>::ptr> p_vec = block[i]->getInterpolPress();
                Vec<Scalar>::ptr p;
                if(p_vec.size()>0){
                    addObject("data_out1", p);
                    p = p_vec[0];
                }
            }
        }
        break;
    }

    world.barrier();
et = boost::chrono::high_resolution_clock::now();

if(world.rank()==0){
    std::cout << "total: " << 1e-9*boost::chrono::duration_cast<boost::chrono::nanoseconds>(et-st).count() << std::endl <<
                 "comm: " << ect << std::endl;
}
    if(world.rank()==0){
        std::cout << "Tracer done" << std::endl;
    }
    return true;
}

