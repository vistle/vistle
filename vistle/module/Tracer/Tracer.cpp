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
#include <fstream>


MODULE_MAIN(Tracer)


using namespace vistle;

//for testing
namespace chrono{
boost::chrono::high_resolution_clock::time_point fc_start;
boost::chrono::high_resolution_clock::time_point fc_stop;
boost::chrono::high_resolution_clock::time_point integ_start;
boost::chrono::high_resolution_clock::time_point integ_stop;
boost::chrono::high_resolution_clock::time_point inter_start;
boost::chrono::high_resolution_clock::time_point inter_stop;
boost::chrono::high_resolution_clock::time_point comm_start;
boost::chrono::high_resolution_clock::time_point comm_stop;
boost::chrono::high_resolution_clock::time_point total_start;
boost::chrono::high_resolution_clock::time_point total_stop;
double add(boost::chrono::high_resolution_clock::time_point start, boost::chrono::high_resolution_clock::time_point stop){
    return 1e-9*boost::chrono::duration_cast<boost::chrono::nanoseconds>(stop-start).count();
}
boost::chrono::high_resolution_clock::time_point now(){
    return boost::chrono::high_resolution_clock::now();
}
}
double fc_dur,integ_dur,inter_dur,comm_dur,other_dur;
Index no_interpol;

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
    const Index m_id;
    UnstructuredGrid::const_ptr m_grid;
    Vec<Scalar, 3>::const_ptr m_vecfld;
    Vec<Scalar>::const_ptr m_sclfld;
    Lines::ptr m_lines;
    std::vector<Vec<Scalar, 3>::ptr> m_ivector;
    std::vector<Vec<Scalar>::ptr> m_iscalar;

public:

    BlockData(Index i,
              UnstructuredGrid::const_ptr grid,
              Vec<Scalar, 3>::const_ptr vdata,
              Vec<Scalar>::const_ptr pdata = nullptr):
        m_id(i),
        m_grid(grid),
        m_vecfld(vdata),
        m_sclfld(pdata),
        m_lines(new Lines(Object::Initialized)){
    }

    UnstructuredGrid::const_ptr getGrid(){
        return m_grid;
    }

    Vec<Scalar, 3>::const_ptr getVField(){
        return m_vecfld;
    }

    Vec<Scalar>::const_ptr getScField(){
        return m_sclfld;
    }

    Lines::ptr getLines(){
        return m_lines;
    }

    std::vector<Vec<Scalar, 3>::ptr> getInterpolVelo(){
        return m_ivector;
    }

    std::vector<Vec<Scalar>::ptr> getInterpolPress(){
        return m_iscalar;
    }

    void addData(const std::vector<Vector3> &points,
                 const std::vector<Vector3> &velocities,
                 const std::vector<Scalar> &pressures){

            Index numpoints = points.size();

            if(m_ivector.size()==0){
                m_ivector.emplace_back(new Vec<Scalar, 3>(Object::Initialized));
            }

            if(pressures.size()==numpoints && m_iscalar.size()==0){
                m_iscalar.emplace_back(new Vec<Scalar>(Object::Initialized));
            }

            for(Index i=0; i<numpoints; i++){

                    m_lines->x().push_back(points[i](0));
                    m_lines->y().push_back(points[i](1));
                    m_lines->z().push_back(points[i](2));
                    m_lines->cl().push_back(m_lines->getNumCorners());

                    m_ivector[0]->x().push_back(velocities[0](0));
                    m_ivector[0]->y().push_back(velocities[0](1));
                    m_ivector[0]->z().push_back(velocities[0](2));

                    if(pressures.size()==numpoints){
                        m_iscalar[0]->x().push_back(pressures[0]);
                    }
            }
            Index numcorn = m_lines->getNumCorners();
            m_lines->el().push_back(numcorn);
    }
};

class Integrator{

private:
    Vector3 m_k[3];
    const Scalar m_b2nd[2];
    const Scalar m_b3rd[3];
    const Scalar m_a2;
    const Scalar m_a3[2];
    const Scalar m_c[2];
    Scalar m_h;
    Scalar m_hmin;
    Scalar m_hmax;
    short m_stg;
    Vector3 m_x0;

public:
    Integrator(Scalar hmin, Scalar hmax, Scalar hinit):
        m_b2nd{0.5,0.5},
        m_b3rd{1.0/6.0,1.0/6.0,2.0/3.0},
        m_a2(1),
        m_a3{0.25,0.25},
        m_c{1,0.5},
        m_h(hinit),
        m_hmin(hmin),
        m_hmax(hmax),
        m_stg(0){
    }

    ~Integrator();

    void hInit(BlockData* bl, Index el, const Vector3 &point){

        UnstructuredGrid::const_ptr grid = bl->getGrid();
        UnstructuredGrid::Interpolator ipol = grid->getInterpolator(el,point);
        Vec<Scalar,3>::const_ptr vecfld = bl->getVField();
        Scalar* u = vecfld->x().data();
        Scalar* v = vecfld->y().data();
        Scalar* w = vecfld->z().data();
        Vector3 velo = ipol(u,v,w);
        Index first = grid->el()[el];
        Index next = grid->el()[el+1];
        Scalar xmin = grid->x()[grid->cl()[first]];
        Scalar xmax = xmin;
        Scalar ymin = grid->y()[grid->cl()[first]];
        Scalar ymax = ymin;
        Scalar zmin = grid->z()[grid->cl()[first]];
        Scalar zmax = zmin;
        for(Index i=first+1; i<next; i++){
            Scalar x = grid->x()[grid->cl()[first+1]];
            Scalar y = grid->y()[grid->cl()[first+1]];
            Scalar z = grid->z()[grid->cl()[first+1]];
            xmin = fmin(xmin,x);
            xmax = fmax(xmax,x);
            ymin = fmin(ymin,y);
            ymax = fmax(ymax,y);
            zmin = fmin(zmin,z);
            zmax = fmax(zmax,z);
        }
        Scalar diag = Vector3(xmax-xmin,ymax-ymin,zmax-zmin).norm();
        //Scalar repvol = fabs((xmax-xmin)*(ymax-ymin)*(zmax-zmin));
        //m_h = 0.5*std::pow(repvol,1/3.)/velo.norm();
        m_h = 0.5*diag/velo.norm();
    }

    bool rk23(BlockData* block, Index &el, const Vector3 &v, Vector &x){

        Scalar errabs;
        Scalar tolabs;
        Index eltmp;
        Vector3 f,xn;
        UnstructuredGrid::const_ptr grid = block->getGrid();
        Vec<Scalar,3>::const_ptr vecfld = block->getVField();
        m_x0 = x;
        Vector3 x2nd;
        Vector3 x3rd;

        switch(m_stg){

        case 0:
            m_stg++;
            m_k[0] = v;
            xn = x + m_h*m_a2*m_k[0];
            if(!grid->inside(el,xn)){
                eltmp = grid->findCell(xn);
                if(eltmp==InvalidIndex){
                    x = xn;
                    return false;
                }
                el = eltmp;
            }

        case 1:{
            UnstructuredGrid::Interpolator ipol = grid->getInterpolator(el,xn);
            Scalar* u = vecfld->x().data();
            Scalar* v = vecfld->y().data();
            Scalar* w = vecfld->z().data();
            m_stg++;
            m_k[1] = ipol(u,v,w);
            xn = xn + (m_a3[0]*m_k[0]+m_a3[1]*m_k[1])*m_h;
            if(!grid->inside(el,xn)){
                eltmp = grid->findCell(xn);
                if(eltmp==InvalidIndex){
                    x = xn;
                    return false;
                }
                el = eltmp;
            }}
        case 2:{
            UnstructuredGrid::Interpolator ipol = grid->getInterpolator(el,xn);
            Scalar* u = vecfld->x().data();
            Scalar* v = vecfld->y().data();
            Scalar* w = vecfld->z().data();
            m_k[2] = ipol(u,v,w);
        }
        case 3:{
           x2nd = x + m_h*(m_b2nd[0]*m_k[0] + m_b2nd[1]*m_k[1]);
           x3rd = x + m_h*(m_b3rd[0]*m_k[0] + m_b3rd[1]*m_k[1] +m_b3rd[2]*m_k[2]);
           //errabs = fabs(x3rd-x2nd);
           //tolabs = fmin(x3rd.norm(),x2nd.norm())*0.01;
           bool accept = true;
           x = x3rd;
           m_stg=0;
           return true;
        }}
    }

    void send(boost::mpi::communicator world, Index root){

        if(m_stg>0){
            boost::mpi::broadcast(world,m_h,root);
            boost::mpi::broadcast(world,m_k,3,root);
            boost::mpi::broadcast(world,m_x0,root);
            return;
        }
        boost::mpi::broadcast(world,m_h,root);

    }
};

class Particle{
friend class Tracer;
private:
    const Index m_id;
    Vector3 m_x;
    std::vector<Vector3> m_xhist;
    Vector3 m_v;
    std::vector<Vector3> m_vhist;
    std::vector<Scalar> m_sclhist;
    Index m_stp;
    BlockData* m_block;
    Index m_el;
    bool m_ingrid;
    bool m_out;
    bool m_in;
    Integrator* m_integrator;

public:
    Particle(Index i, const std::vector<std::unique_ptr<BlockData>> &bl, const Vector3 &pos):
        m_id(i),
        m_x(pos),
        m_v(Vector3(1,0,0)),
        m_stp(0),
        m_block(nullptr),
        m_el(InvalidIndex),
        m_ingrid(true),
        m_out(false),
        m_in(true){

        findCell(bl,1000);
        m_integrator = new Integrator(0.0001,0.01, 0.001);
        /*if(m_block){
            m_integrator->hInit(m_block,m_el,pos);
        }*/
    }

    ~Particle(){}

    bool isActive(){

        if(m_ingrid && m_block){
            return true;
        }
        return false;
    }

    bool inGrid(){

        return m_ingrid;
    }

    void findCell(const std::vector<std::unique_ptr<BlockData>> &block, Index steps_max){

        if(!m_ingrid){
            return;
        }

        if(m_stp == steps_max){

            m_block->addData(m_xhist, m_vhist, m_sclhist);
            m_xhist.clear();
            m_vhist.clear();
            m_sclhist.clear();
            m_out = true;
            this->deact();
            return;
        }

        bool moving = (m_v(0)!=0 || m_v(1)!=0 || 0 || m_v(2)!=0);
        if(!moving){

            m_block->addData(m_xhist, m_vhist, m_sclhist);
            m_xhist.clear();
            m_vhist.clear();
            m_sclhist.clear();
            m_out = true;
            this->deact();
            return;
        }

        if(m_block){

            UnstructuredGrid::const_ptr grid = m_block->getGrid();
            if(grid->inside(m_el, m_x)){
                return;
            }
            m_el = grid->findCell(m_x);
            if(m_el==InvalidIndex){

                m_vhist.push_back(m_v);
                m_xhist.push_back(m_x);
                m_block->addData(m_xhist, m_vhist, m_sclhist);
                m_xhist.clear();
                m_vhist.clear();
                m_sclhist.clear();
                m_block = nullptr;
                m_out = true;
                m_in = true;
                findCell(block, steps_max);
            }
            return;
        }

        if(m_in){

            for(Index i=0; i<block.size(); i++){

                UnstructuredGrid::const_ptr grid = block[i]->getGrid();
                m_el = grid->findCell(m_x);
                if(m_el!=InvalidIndex){

                    m_block = block[i].get();
                    if(m_stp!=0){
                        m_vhist.push_back(m_v);
                    }
                    m_xhist.push_back(m_x);
                    m_out = false;
                    break;
                }
            }
            m_in = false;
            return;
        }
    }

    void deact(){

        m_block = nullptr;
        m_ingrid = false;
    }

    void interpol(){

        UnstructuredGrid::const_ptr grid = m_block->getGrid();
        Vec<Scalar, 3>::const_ptr v_data = m_block->getVField();
        Vec<Scalar>::const_ptr p_data = m_block->getScField();
        UnstructuredGrid::Interpolator interpolator = grid->getInterpolator(m_el, m_x);
        Scalar* u = v_data->x().data();
        Scalar* v = v_data->y().data();
        Scalar* w = v_data->z().data();
        m_v = interpolator(u,v,w);
        m_vhist.push_back(m_v);
    }

    void integr(){

        /*m_x = m_x + m_v*0.001;
        m_xhist.push_back(m_x);
        ++m_stp;*/
        m_integrator->rk23(m_block,m_el,m_v,m_x);
        m_xhist.push_back(m_x);
        m_stp++;
    }

    void Communicator(boost::mpi::communicator mpi_comm, Index root){

        boost::mpi::broadcast(mpi_comm, m_x, root);
        boost::mpi::broadcast(mpi_comm, m_stp, root);
        boost::mpi::broadcast(mpi_comm, m_ingrid, root);


        m_out = false;
        if(mpi_comm.rank()!=root){

            m_in = true;
        }
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

    Index numblocks = grid_in.size();

    //get parameters
    Index numpoints = getIntParameter("no_startp");
    Index steps_max = getIntParameter("steps_max");
    Scalar dt = getFloatParameter("timestep");
    Index task_type = getIntParameter("taskType");

    boost::mpi::communicator world;
    Index noel = 0;
    Index novert = 0;
    for(Index i=0;i<grid_in.size();i++){
        noel+=grid_in[i]->getNumElements();
        novert += grid_in[i]->getNumVertices();
    }
    noel = boost::mpi::all_reduce(world,noel,std::plus<Index>());
    novert = boost::mpi::all_reduce(world,novert,std::plus<Index>());
    if(world.rank()==0){
        std::cout << "no_elem:  " << noel << std::endl << "no_vert:  " << novert << std::endl;
    }
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

        particle[i].reset(new Particle(i,block,startpoints[i]));
    }

    //find cell and set status of particle objects
    for(Index i=0; i<numpoints; i++){

        particle[i]->findCell(block, steps_max);
    }


    for(Index i=0; i<numpoints; i++){

        bool active = particle[i]->isActive();
        active = boost::mpi::all_reduce(world, active, std::logical_or<bool>());
        if(!active){
            particle[i]->deact();
        }
    }
    Index count =0;
    bool ingrid = true;
    Index mpisize = world.size();
    std::vector<boost::mpi::request> reqs;
    std::vector<Index> sendlist(0);
    while(ingrid){

            for(Index i=0; i<numpoints; i++){
                bool traced = false;
                while(particle[i]->isActive()){
                    traced = true;
                    particle[i]->interpol();
                    particle[i]->integr();
                    particle[i]->findCell(block, steps_max);
                }
                if(traced){
                    sendlist.push_back(i);
                }
            }

            for(Index mpirank=0; mpirank<mpisize; mpirank++){

                Index num_send = sendlist.size();
                boost::mpi::broadcast(world, num_send, mpirank);

                if(num_send>0){
                    std::vector<Index> tmplist = sendlist;
                    boost::mpi::broadcast(world, tmplist, mpirank);
                    for(Index i=0; i<num_send; i++){
                        Index p_index = tmplist[i];
                        particle[p_index]->Communicator(world, mpirank);
                        particle[p_index]->findCell(block, steps_max);
                        bool active = particle[p_index]->isActive();
                        active = boost::mpi::all_reduce(world, particle[p_index]->isActive(), std::logical_or<bool>());
                        if(!active){
                            particle[p_index]->deact();
                        }
                    }
                }
            }
/*Index left, right;
right = (world.rank()+1)%world.size();
if(world.rank()==0){left=world.size()-1;}else{left=world.rank()-1;}
for(Index i=0; i<sendlist.size(); i++){
    Index id = sendlist[i];
    reqs.push_back(world.isend(right,numpoints,id));
    reqs.push_back(world.isend(right,id,particle[id]->m_x));
    reqs.push_back(world.isend(right,id,particle[id]->m_ingrid));
    reqs.push_back(world.isend(right,id,particle[id]->m_stp));
}
sendlist.clear();
while(world.iprobe(left,numpoints)){
    Index id;
    world.recv(left,numpoints,id);
    world.recv(left,id,particle[id]->m_x);
    world.recv(left,id,particle[id]->m_ingrid);
    world.recv(left,id,particle[id]->m_stp);
    particle[id]->m_in = true;
    particle[id]->findCell(block, steps_max, task_type);
    if(!particle[id]->m_block){sendlist.push_back(id);}
}
Index req=0;
while(req<reqs.size()){
    if(reqs[req].test()){reqs.erase(reqs.begin()+req);}else{req++;}
}*/
                ingrid = false;
                for(Index i=0; i<numpoints; i++){

                    if(particle[i]->inGrid()){

                        ingrid = true;
                        break;
                    }
                }
                //if(world.rank()==0){std::cout << count << std::endl;}count++; ingrid = count<100;
            //}
            sendlist.clear();
    }
    /*world.barrier();
    for(Index i=0; i<reqs.size(); i++){
        reqs[i].cancel();
    }*/

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

    world.barrier();/*
//********************************************************
//  FOR TIME MEASUREMENTS
    std::ofstream out;
    if(world.rank()==0){
        out.open("MPISIZE" << world.size() << "-" << meascount << ".csv");
        for(Index i=0;i<world.size();i++){
            out << "," << i;
        }
        out << ",sum,avrg\n";
    }
    double dur[5] = {fc_dur,integ_dur,inter_dur,comm_dur,other_dur}
    std::vector<double> durall(world.size()-1);
    for(Index i=0;i<5;i++){
        boost::mpi::gather(world,dur[i],durall,0);
        if(world.rank()==0){
            for(Index j=0; j<durall.size();j++){
                out <<  "," << dur[i];
            }
            out << "\n";
        }
    }
    boost::mpi::gather(world,no_interpol,durall,0);
    if(world.rank()==0){
        for(Index j=0; j<durall.size();j++){
            out <<  "," << dur[i];
        }
    }
//********************************************************
*/
    if(world.rank()==0){
        std::cout << "Tracer done" << std::endl;
    }
    return true;
}
//double fc_dur,integ_dur,inter_dur,comm_dur,other_dur;
//Index no_interpol;
