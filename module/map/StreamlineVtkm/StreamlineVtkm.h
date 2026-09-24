#ifndef VISTLE_STREAMLINEVTKM_STREAMLINEVTKM_H
#define VISTLE_STREAMLINEVTKM_STREAMLINEVTKM_H

#include <cstddef>
#include <mutex>
#include <string>
#include <vector>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/filter/Filter.h>
#include <viskores/Particle.h>

#include <vistle/alg/objalg.h>
#include <vistle/core/objectmeta.h>
#include <vistle/module/module.h>
#include <vistle/util/enum.h>
#include <vistle/vtkm/module_status.h>

// TODO: add backwards integration to the Streamline filter

class StreamlineVtkm: public vistle::Module {
public:
    struct GlobalData {
        // the input grid in the Vistle data format (stored per timestep and per data block)
        std::vector<std::vector<vistle::Object::const_ptr>> inputGrids;
        // the input fields in the Vistle data format (stored per input port, per timestep and per data block)
        std::vector<std::vector<std::vector<vistle::DataBase::const_ptr>>> inputFields;
        // the partitioned input datasets in the Viskores data format (stored per timestep)
        std::vector<viskores::cont::PartitionedDataSet> partitionedDatasets;

        std::mutex mutex;

        void clear();
        bool isEmpty() const;
        /*
            Get the input grids, input fields and corresponding partitioned dataset at
            the given index (i.e., timestep). Returns false if the index is out of bounds
            or if the partitioned dataset at the given index is empty. In that case, no
            data is stored in `grids`, `fields` and `dataset`.
        */
        bool getDataAtIndex(unsigned int index, std::vector<vistle::Object::const_ptr> &grids,
                            std::vector<std::vector<vistle::DataBase::const_ptr>> &fieldsPerPort,
                            viskores::cont::PartitionedDataSet &dataset);
        void resize(std::size_t newSize, std::size_t numFields);
    };

    StreamlineVtkm(const std::string &name, int moduleID, mpi::communicator comm);

    bool prepare() override;
    bool compute(const std::shared_ptr<vistle::BlockTask> &task) const override;
    bool reduce(int timestep) override;

    bool changeParameter(const vistle::Parameter *param) override;

private:
    const int m_numPorts = 3;
    std::vector<vistle::Port *> m_inputPorts, m_outputPorts;

    vistle::IntParameter *m_integrationMethod;
    vistle::IntParameter *m_numberOfSeeds, *m_maxNumberOfSeeds, *m_numberOfSteps;
    vistle::IntParameter *m_startStyle;

    vistle::FloatParameter *m_stepSize;

    vistle::VectorParameter *m_direction;
    vistle::VectorParameter *m_startPoint1, *m_startPoint2;

    mutable GlobalData m_globalData;

    ModuleStatusPtr readInAndVerifyPorts(const std::shared_ptr<vistle::BlockTask> &task,
                                         vistle::Object::const_ptr &grid,
                                         std::vector<vistle::DataBase::const_ptr> &fields) const;

    ModuleStatusPtr transformInputToViskores(const vistle::Object::const_ptr &grid,
                                             const std::vector<vistle::DataBase::const_ptr> &fields,
                                             viskores::cont::DataSet &dataset) const;

    viskores::cont::ArrayHandle<viskores::Particle> createSeedArray() const;

    std::unique_ptr<viskores::filter::Filter> setUpFilter() const;

    bool tryToExecuteFilter(viskores::filter::Filter &filter, const viskores::cont::DataSet &inputDataset,
                            viskores::cont::DataSet &outputDataset) const;

    bool tryToExecuteFilter(viskores::filter::Filter &filter, const viskores::cont::PartitionedDataSet &inputDataset,
                            viskores::cont::PartitionedDataSet &outputDataset) const;

    bool checkAndNotify(const ModuleStatusPtr &status) const;

    void createModulePorts();
    void createModuleParameters();
    void setViskoresMPICommunicatorToCopyOf(const mpi::communicator &comm) const;

    std::string getFieldName(int index, bool output = false) const;
    vistle::Meta getMetaForOutput(int timestep = -1) const;
    std::string getPortName(int index, bool output) const;
};

#endif // VISTLE_STREAMLINEVTKM_STREAMLINEVTKM_H
