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
    IntParameter* task_type = addIntParameter("taskType", "task type", 0, Parameter::Choice);
    std::vector<std::string> choices;
    choices.push_back("moving points");
    choices.push_back("streamlines");
    setParameterChoices(task_type, choices);
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
    std::vector<Points::ptr> m_points;
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

    std::vector<Points::ptr> getPoints(){
        return m_points;
    }

    std::vector<Vec<Scalar, 3>::ptr> getInterpolVelo(){
        return m_v_interpol;
    }

    std::vector<Vec<Scalar>::ptr> getInterpolPress(){
        return m_p_interpol;
    }

    void addLines(const std::vector<Vector3> &points){

        Index numpoints = points.size();
        Index size0 = m_lines->getNumVertices();
        Index size1 = size0 + numpoints;

        m_lines->cl().resize(size1);
        m_lines->x().resize(size1);
        m_lines->y().resize(size1);
        m_lines->z().resize(size1);

        for(Index i=0; i<numpoints; i++){

            m_lines->cl()[size0+i] = size0+i;
            m_lines->x()[size0+i] = points[i](0);
            m_lines->y()[size0+i] = points[i](1);
            m_lines->z()[size0+i] = points[i](2);
        }

        Index numcorn = m_lines->getNumCorners();
        m_lines->el().push_back(numcorn);
    }

    void addData(std::vector<Vector3> points,
                 std::vector<Vector3> velocities,
                 std::vector<Scalar> pressures,
                 Index tasktype,
                 Index first_timestep =0){

        switch(tasktype){

        case 0:{
            Index numpoints = points.size();
            Index numobjects = m_points.size();

            for(Index i=0; i<numpoints; i++){

                bool havepress = (pressures.size()==points.size());
                bool exists = false;
                Index timestep = first_timestep+i;

                for(Index j=0; j<numobjects; j++){

                    if(m_points[j]->getTimestep()==timestep){

                        exists = true;
                        m_points[j]->x().push_back(points[i](0));
                        m_points[j]->y().push_back(points[i](1));
                        m_points[j]->z().push_back(points[i](2));

                        m_v_interpol[j]->x().push_back(velocities[i](0));
                        m_v_interpol[j]->x().push_back(velocities[i](1));
                        m_v_interpol[j]->x().push_back(velocities[i](2));

                        if(havepress){
                            m_p_interpol[j]->x().push_back(pressures[i]);
                        }
                        break;
                    }
                }
                if(!exists){

                    m_points.emplace_back(new Points(Object::Initialized));
                    Index back = m_points.size() -1;
                    m_points[back]->x().push_back(points[i](0));
                    m_points[back]->y().push_back(points[i](1));
                    m_points[back]->z().push_back(points[i](2));
                    m_points[back]->setTimestep(timestep);

                    m_v_interpol.emplace_back(new Vec<Scalar, 3>(Object::Initialized));
                    m_v_interpol[back]->x().push_back(velocities[i](0));
                    m_v_interpol[back]->y().push_back(velocities[i](1));
                    m_v_interpol[back]->z().push_back(velocities[i](2));
                    m_v_interpol[back]->setTimestep(timestep);

                    if(havepress){
                        m_p_interpol.emplace_back(new Vec<Scalar>(Object::Initialized));
                        m_p_interpol[back]->x().push_back(pressures[i]);
                        m_p_interpol[back]->setTimestep(timestep);
                    }
                }
            }
        }

            break;

        case 1:{
            Index numpoints = points.size();
            Index size0 = m_lines->getNumVertices();
            Index size1 = size0 + numpoints;

            m_lines->cl().resize(size1);
            m_lines->x().resize(size1);
            m_lines->y().resize(size1);
            m_lines->z().resize(size1);

            if(m_v_interpol.size()==0){
                m_v_interpol.emplace_back(new Vec<Scalar, 3>(Object::Initialized));
            }
            m_v_interpol[0]->x().resize(size1);
            m_v_interpol[0]->y().resize(size1);
            m_v_interpol[0]->z().resize(size1);

            for(Index i=0; i<numpoints; i++){

                m_lines->cl()[size0+i] = size0+i;
                m_lines->x()[size0+i] = points[i](0);
                m_lines->y()[size0+i] = points[i](1);
                m_lines->z()[size0+i] = points[i](2);
                m_v_interpol[0]->x()[size0+i] = velocities[i](0);
                m_v_interpol[0]->y()[size0+i] = velocities[i](1);
                m_v_interpol[0]->z()[size0+i] = velocities[i](2);
            }

            Index numcorn = m_lines->getNumCorners();
            m_lines->el().push_back(numcorn);

            if(pressures.size()<velocities.size()){
                return;
            }

            if(m_p_interpol.size()==0){
                m_p_interpol.emplace_back(new Vec<Scalar>(Object::Initialized));
            }
            m_p_interpol[0]->x().resize(size1);
            for(Index i=0; i<numpoints; i++){
                m_p_interpol[0]->x()[size0+i] = pressures[i];
            }
        }
        }
    }
};


class Particle{

private:
    const Index m_index;
    Vector3 m_position;
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

public:
    Particle(Index i, Vector3 pos):
        m_index(i),
        m_stepcount(0),
        m_cell(InvalidIndex),
        m_block(nullptr),
        m_ingrid(true),
        m_position(pos),
        m_in(true),
        m_out(false){
        //initialize velocity: velocity.norm()!=0
        m_velocity << 1,0,0;
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

    void setStatus(std::vector<BlockData*> block, Index steps_max, Index tasktype){

        if(!m_ingrid){
            return;
        }

        if(m_out){
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
            if(m_block){

                m_block->addData(m_positions, m_velocities, m_pressures, tasktype);
                m_positions.clear();
                m_velocities.clear();
                m_pressures.clear();
                m_out = true;
            }
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

                    m_block = block[i];
                    m_positions.resize(1);
                    m_positions[0] = m_position;
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

        /*UnstructuredGrid::const_ptr grid = m_block->getGrid();
        Vec<Scalar, 3>::const_ptr v_data = m_block->getVelocityData();
        Vec<Scalar>::const_ptr p_data = m_block->getPressureData();
        Index cell = m_cell;
        Vector3 point = m_position;
        Index numvert = grid->el()[cell+1] - grid->el()[cell];
        Vector3 vert, d_vec;
        Scalar d, d_min;
        Index vert_dmin;
        //nearest neighbor interpolation
        vert << grid->x()[grid->cl()[grid->el()[cell]]],
                grid->y()[grid->cl()[grid->el()[cell]]],
                grid->z()[grid->cl()[grid->el()[cell]]];
        d_vec = point - vert;
        d_min = d_vec.norm();
        vert_dmin = grid->cl()[grid->el()[cell]];

        for(Index i=1; i<numvert; i++){

            vert << grid->x()[grid->cl()[grid->el()[cell]+i]],
                    grid->y()[grid->cl()[grid->el()[cell]+i]],
                    grid->z()[grid->cl()[grid->el()[cell]+i]];
            d_vec = point - vert;
            d = d_vec.norm();

            if(d<d_min){

                d_min = d;
                vert_dmin = grid->cl()[grid->el()[cell]+i];

            }
        }

        m_velocity << v_data->x()[vert_dmin],
                            v_data->y()[vert_dmin],
                            v_data->z()[vert_dmin];
        m_velocities.push_back(this->m_velocity);

        if(p_data){
            m_pressures.push_back(p_data->x()[vert_dmin]);
        }*/

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

    void Integration(const Scalar &dt){

        Vector3 v0 = this->m_velocity;
        Vector3 p0 = this->m_position;
        Vector3 k[4];
        Vector3 v[3];

        //fourth order Runge-Kutta method
        k[0] = v0*dt;
        v[0] = p0 + k[0]/2;
        k[1] = v[0]*dt;
        v[1] = p0 + k[1]/2;
        k[2] = v[1]*dt;
        v[2] = p0 + k[2];
        k[3] = v[2]*dt;

        this->m_position = p0 + (k[0] + 2*k[1] + 2*k[2] + k[3])/6;
        this->m_positions.push_back(m_position);
        this->m_stepcount +=1;
    }

    void Communicator(boost::mpi::communicator mpi_comm, Index root){

        boost::mpi::broadcast(mpi_comm, m_position, root);
        boost::mpi::broadcast(mpi_comm, m_stepcount, root);
        //m_ingrid = boost::mpi::all_reduce(mpi_comm, m_ingrid, std::logical_and<bool>());
        if(mpi_comm.rank()==root && !m_ingrid){
            boost::mpi::broadcast(mpi_comm, m_ingrid, root);
        }
        m_out = false;
        if(mpi_comm.rank()!=root){

            m_in = true;
        }
    }

    bool leftNode(){

        return m_out;
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

        data_in1.push_back(Vec<Scalar>::as(takeFirstObject("data_in1")));
    }

    return true;
}


bool Tracer::reduce(int timestep){

    Index numblocks = grid_in.size();
    Index numpoints = getIntParameter("no_startp");
    Index steps_max = getIntParameter("steps_max");
    Index steps_comm = getIntParameter("steps_comm");
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
    std::vector<BlockData*> block(numblocks);
    for(Index i=0; i<numblocks; i++){

        block[i] = new BlockData(i, grid_in[i], data_in0[i], data_in1[i]);
    }

    //create particle objects
    std::vector<Particle*> particle(numpoints);
    for(Index i=0; i<numpoints; i++){

        particle[i] = new Particle(i, startpoints[i]);
    }

    //find cell and set status of particle objects
    for(Index i=0; i<numpoints; i++){

        particle[i]->setStatus(block, steps_max, task_type);
    }

    Index stepcount=0;
    bool ingrid = true;
    Index mpisize = world.size();
    std::vector<Index> sendlist(numpoints, mpisize);

    while(ingrid){

        sendlist.assign(numpoints, mpisize);

        for(Index i=0; i<numpoints; i++){

            if(particle[i]->isActive()){

                particle[i]->Interpolation();
                particle[i]->Integration(dt);
            }

            particle[i]->setStatus(block, steps_max, task_type);

            if(particle[i]->leftNode()){

                sendlist[i] = world.rank();
            }
        }
        ++stepcount;

        if(stepcount == steps_comm){

            for(Index i=0; i<numpoints; i++){

                Index root = boost::mpi::all_reduce(world, sendlist[i], boost::mpi::minimum<Index>());
                if(root<mpisize){
                    particle[i]->Communicator(world, root);
                    particle[i]->setStatus(block, steps_max, task_type);
                    bool active = particle[i]->isActive();
                    active = boost::mpi::all_reduce(world, active, std::logical_or<bool>());
                    if(!active){
                        particle[i]->Deactivate();
                    }
                }
            }
            ingrid = false;
            for(Index i=0; i<numpoints; i++){

                if(particle[i]->inGrid()){

                    ingrid = true;
                    break;
                }
            }
            stepcount = 0;
        }
    }

    switch(task_type){

    case 0:

        //add Points objects to output port
        for(Index i=0; i<numblocks; i++){


            std::vector<Points::ptr> points = block[i]->getPoints();
            Index numobjects = points.size();
            for(Index j=0; j<numobjects; j++){

                if(points[j]->getNumPoints() >0){
                    addObject("geom_out", points[j]);
                }
            }

            /*std::vector<Vec<Scalar, 3>::ptr> v_vec = block[i]->getInterpolVelo();
            Vec<Scalar, 3>::ptr v;
            for(Index j=0; j<numobjects; j++){
                if(v_vec.size()>0){
                    v = v_vec[j];
                    addObject("data_out0", v);
                }
            }

            if(data_in1[0]){
                std::vector<Vec<Scalar>::ptr> p_vec = block[i]->getInterpolPress();
                Vec<Scalar>::ptr p;
                for(Index j=0; j<numobjects; j++){
                    if(p_vec.size()>0){
                        p = p_vec[j];
                        addObject("data_in1", p);
                    }
                }
            }*/
        }

    case 1:
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
                    addObject("data_in1", p);
                    p = p_vec[0];
                }
            }
        }        
    }
    world.barrier();
    if(world.rank()==0){
        std::cout << "Tracer done" << std::endl;
    }
    return true;
}

