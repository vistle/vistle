#include "VtkmModule.h"

using namespace vistle;

VtkmModule::VtkmModule(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    m_mapDataIn = createInputPort("data_in", "input data");
    m_dataOut = createOutputPort("data_out", "output data");
}

VtkmModule::~VtkmModule()
{}

bool VtkmModule::compute(const std::shared_ptr<vistle::BlockTask> &task) const
{
    inputToVistle(task);
    runFilter();
    outputToVtkm(task);
}

bool VtkmModule::inputToVistle(const std::shared_ptr<vistle::BlockTask> &task) const
{}

bool VtkmModule::outputToVtkm(const std::shared_ptr<vistle::BlockTask> &task) const
{}
