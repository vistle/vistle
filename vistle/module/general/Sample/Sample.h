#ifndef SAMPLE_H
#define SAMPLE_H

#include <module/module.h>
#include <core/object.h>
#include <core/grid.h>
#include <core/uniformgrid.h>
#include <core/structuredgrid.h>
#include <core/rectilineargrid.h>
#include <core/unstr.h>

namespace mpi = boost::mpi;

class Sample: public vistle::Module {
public:
    Sample(const std::string &name, int moduleID, mpi::communicator comm);
    ~Sample();

private:
    //virtual bool compute() override;
    virtual bool compute(std::shared_ptr<vistle::PortTask> task) const override;

    virtual bool reduce(int timestep) override;
    bool objectAdded(int sender, const std::string &senderPort, const vistle::Port *port) override;
    bool changeParameter(const vistle::Parameter *p) override;

    int SampleToGrid(const vistle::GeometryInterface *target, vistle::DataBase::const_ptr inData, vistle::DataBase::ptr &sampled );

    vistle::IntParameter *m_mode, *m_valOutside, *m_hits ;
    vistle::GridInterface::InterpolationMode mode;
    vistle::Port *m_out = nullptr;
    vistle::IntParameter *m_createCelltree;
    vistle::FloatParameter *m_userDef;

    std::vector<vistle::Object::const_ptr> objListLocal;
    std::vector<vistle::DataBase::const_ptr> dataList;
    std::vector<int> blockIdx;

    bool m_useCelltree = false;

    float NO_VALUE = 0.0;
};

#endif
