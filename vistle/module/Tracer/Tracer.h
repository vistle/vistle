#ifndef TRACER_H
#define TRACER_H

#include <future>

#include <core/unstr.h>
#include <module/module.h>
#include <core/lines.h>


class Tracer: public vistle::Module {

public:
    Tracer(const std::string &shmname, const std::string &name, int moduleID);
    ~Tracer();


 private:
    virtual bool compute();
    virtual bool prepare();
    virtual bool reduce(int timestep);

    std::vector<std::vector<vistle::UnstructuredGrid::const_ptr>> grid_in;
    std::vector<std::vector<std::future<vistle::Celltree3::const_ptr>>> celltree;
    std::vector<std::vector<vistle::Vec<vistle::Scalar,3>::const_ptr>> data_in0;
    std::vector<std::vector<vistle::Vec<vistle::Scalar>::const_ptr>> data_in1;
    vistle::Vector3 Integration(const vistle::Vector3 &p0, const vistle::Vector3 &v0, const vistle::Scalar &dt);
    vistle::Index findCell(const vistle::Vector3 &point,
                           vistle::UnstructuredGrid::const_ptr grid,
                           std::array<vistle::Vector3, 2> boundingbox,
                           vistle::Index lastcell);

    vistle::IntParameter *m_useCelltree;

};


class BlockData{

private:
    vistle::UnstructuredGrid::const_ptr m_grid;
    vistle::Vec<vistle::Scalar, 3>::const_ptr m_xecfld;
    vistle::Vec<vistle::Scalar>::const_ptr m_scafld;
    vistle::Lines::ptr m_lines;
    std::vector<vistle::Vec<vistle::Scalar, 3>::ptr> m_ivec;
    std::vector<vistle::Vec<vistle::Scalar>::ptr> m_iscal;

public:
    BlockData(vistle::Index i,
              vistle::UnstructuredGrid::const_ptr grid,
              vistle::Vec<vistle::Scalar, 3>::const_ptr vdata,
              vistle::Vec<vistle::Scalar>::const_ptr pdata = nullptr);
    vistle::UnstructuredGrid::const_ptr getGrid();
    vistle::Vec<vistle::Scalar, 3>::const_ptr getVecFld();
    vistle::Vec<vistle::Scalar>::const_ptr getScalFld();
    vistle::Lines::ptr getLines();
    std::vector<vistle::Vec<vistle::Scalar, 3>::ptr> getIplVec();
    std::vector<vistle::Vec<vistle::Scalar>::ptr> getIplScal();
    ~BlockData();
    void addLines(const std::vector<vistle::Vector3> &points,
                 const std::vector<vistle::Vector3> &velocities,
                 const std::vector<vistle::Scalar> &pressures);
};

class Integrator;

class Particle{

    friend class Integrator;

private:
    vistle::Vector3 m_x;
    vistle::Vector3 m_xold;
    std::vector<vistle::Vector3> m_xhist;
    vistle::Vector3 m_v;
    std::vector<vistle::Vector3> m_vhist;
    std::vector<vistle::Scalar> m_pressures;
    vistle::Index m_stp;
    BlockData* m_block;
    vistle::Index m_el;
    bool m_ingrid;
    bool m_in;
    Integrator* m_integrator;
    const vistle::Index m_stpmax;

public:
    Particle(vistle::Index i, const vistle::Vector3 &pos, vistle::Scalar h, vistle::Scalar hmin,
             vistle::Scalar hmax, vistle::Scalar errtol, int int_mode,const std::vector<std::unique_ptr<BlockData>> &bl,
             vistle::Index stepsmax);
    ~Particle();
    void PointsToLines();
    bool isActive();
    bool inGrid();
    bool findCell(const std::vector<std::unique_ptr<BlockData>> &block);
    void Deactivate();
    void Step();
    void Communicator(boost::mpi::communicator mpi_comm, int root);
    bool leftNode();
};
#endif
