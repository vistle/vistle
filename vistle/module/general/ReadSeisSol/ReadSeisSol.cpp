/**************************************************************************\
 **                                                                      **
 **                                                                      **
 ** Description: Read module for ChEESE Seismic HDF5/XDMF-files  	     **
 **                                                                      **
 **                                                                      **
 **                                                                      **
 **                                                                      **
 **                                                                      **
 ** Author:    Marko Djuric                                              **
 **                                                                      **
 **                                                                      **
 **                                                                      **
 ** Date:  04.03.2021                                                    **
\**************************************************************************/

//header
#include "ReadSeisSol.h"

//vistle
#include "vistle/core/parameter.h"
#include "vistle/core/unstr.h"
#include "vistle/module/module.h"

//boost
#include <array>
#include <boost/mpi/communicator.hpp>

//vtk
/* #include <vtkAlgorithm.h> */
/* #include <vtkCommunicator.h> */
/* #include <vtkCompositeDataIterator.h> */
/* #include <vtkDataObject.h> */
/* #include <vtkPolyData.h> */
/* #include <vtkSmartPointer.h> */
/* #include <vtkType.h> */
/* #include <vtkGraph.h> */
/* #include <vtkPolyData.h> */
/* #include <vtkFieldData.h> */
/* #include <vtkPointData.h> */
/* #include <vtkCellData.h> */
/* #include <vtkCellTypes.h> */
/* #include <vtkUnstructuredGrid.h> */
/* #include <vtkXdmf3DataSet.h> */
/* #include <vtkXdmf3Reader.h> */
/* #include <vtkUniformGrid.h> */
/* #include <vtkProcess.h> */
/* #include <vtkMultiProcessController.h> */
/* #include <vtkMPIController.h> */
/* #include <vtkXdmf3ArraySelection.h> */
/* #include <vtkMultiBlockDataSet.h> */
/* #include <vtkInformation.h> */

//xdmf3
#include <XdmfGeometry.hpp>
#include <XdmfGridCollection.hpp>
#include <XdmfArrayType.hpp>
#include <XdmfUnstructuredGrid.hpp>
#include <XdmfDomain.hpp>
#include <XdmfReader.hpp>
#include <XdmfArray.hpp>
#include <XdmfHDF5WriterDSM.hpp>
#include <XdmfHDF5ControllerDSM.hpp>
#include <XdmfHDF5Controller.hpp>

//std
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mpi.h>
#include <qt/QtCore/qmutex.h>
#include <string>
#include <algorithm>
#include <vector>
#include <array>

using namespace vistle;
namespace {
/**
  * Multiprocess Reader vtk.
  */
/* class XReaderProcess: public vtkProcess { */
/* public: */
/*     static XReaderProcess *New(); */
/*     vtkTypeMacro(XReaderProcess, vtkProcess) void Execute() override; */
/*     void SetArgs(int argc, char *argv[], const std::string &fname) */
/*     { */
/*         this->Argc = argc; */
/*         this->Argv = argv; */
/*         this->FileName = fname; */
/*     } */
/*     /1* void CreatePipeline() *1/ */
/*     /1* { *1/ */
/*     /1*     int num_procs = this->Controller->GetNumberOfProcesses(); *1/ */
/*     /1*     int my_id = this->Controller->GetLocalProcessId(); *1/ */
/*     /1*     this->Reader = vtkXdmf3Reader::New(); *1/ */
/*     /1*     this->Reader->SetFileName(this->FileName.c_str()); *1/ */
/*     /1*     if (my_id == 0) { *1/ */
/*     /1*         cerr << my_id << "/" << num_procs << endl; *1/ */
/*     /1*         cerr << "FILENAME " << this->FileName << endl; *1/ */
/*     /1*     } *1/ */
/*     /1* } *1/ */

/* protected: */
/*     XReaderProcess() */
/*     { */
/*         this->Argc = 0; */
/*         this->Argv = nullptr; */
/*     } */
/*     int Argc; */
/*     char **Argv; */
/*     std::string FileName; */
/*     vtkSmartPointer<vtkXdmf3Reader> Reader; */
/* }; */

/* vtkStandardNewMacro(XReaderProcess) */

/*     void XReaderProcess::Execute() */
/* { */
/*     int proc = this->Controller->GetLocalProcessId(); */
/*     int numprocs = this->Controller->GetNumberOfProcesses(); */

/*     this->Controller->Barrier(); */
/*     this->Reader->UpdatePiece(proc, numprocs, 0); */

/*     //do something with reader */
/*     this->ReturnValue = 1; */
/* } */
} // namespace

MODULE_MAIN(ReadSeisSol)

ReadSeisSol::ReadSeisSol(const std::string &name, int moduleID, mpi::communicator comm)
: vistle::Reader("Read ChEESE Seismic Data files (xmdf/hdf5)", name, moduleID, comm)
{
    p_h5Dir = addStringParameter("file_dir", "HDF5 File directory", "/data/ChEESE/SeisSol/LMU_Sulawesi_example/",
                                 Parameter::ExistingDirectory);
    p_xdmfDir = addStringParameter("file", "Xdmf File", "/data/ChEESE/SeisSol/LMU_Sulawesi_example/Sulawesi.xdmf",
                                   Parameter::Filename);
    p_ghost = addIntParameter("Ghost Layer", "Ghost layer", 1, Parameter::Boolean);
    p_ugrid = createOutputPort("UnstructuredGrid");

    setParallelizationMode(Serial);
}

ReadSeisSol::~ReadSeisSol()
{}

bool ReadSeisSol::examine(const vistle::Parameter *param)
{
    setTimesteps(-1);
    return true;
}

bool ReadSeisSol::read(Token &token, int timestep, int block)
{
    auto reader = XdmfReader::New();
    auto domain = shared_dynamic_cast<XdmfDomain>(reader->read(p_xdmfDir->getValue()));

    sendInfo("domain gridCollect: %d", domain->getNumberGridCollections());
    auto gridCollect = domain->getGridCollection(0);

    auto prop = gridCollect->getItemProperties();
    for (auto &[v1, v2]: prop)
        sendInfo("v1: %s \t v2: %s", v1.c_str(), v2.c_str());

    auto ugrid = gridCollect->getUnstructuredGrid(0);
    sendInfo("gridname: %s", ugrid->getName().c_str());
    const auto topology = ugrid->getTopology();
    sendInfo("topology-valStr: %s", topology->getValuesString().c_str());

    auto topProp = topology->getItemProperties();
    for (auto &[v1, v2]: topProp)
        sendInfo("v1: %s \t v2: %s", v1.c_str(), v2.c_str());

    std::vector<int> connect(topology->getDimensions().at(0));
    topology->getValues(0, connect.data());

    const auto geo = ugrid->getTopology();
    std::vector<float> geometry(geo->getDimensions().at(0) * geo->getDimensions().at(1));
    geo->getValues(0, geometry.data());
    UnstructuredGrid::ptr vUgrid(new UnstructuredGrid(geometry.size(), geometry.size(), connect.size()));
    int numCorner{4};
    std::copy(connect.begin(), connect.end(), vUgrid->cl().data());
    /* auto x = vUgrid->x().data(), y = vUgrid->y().data(), z = vUgrid->z().data(); */

    std::generate(vUgrid->el().begin(), vUgrid->el().end(), [n = 0, &numCorner]() mutable { return n++ * numCorner; });

    vUgrid->setBlock(block);
    vUgrid->setTimestep(timestep);
    vUgrid->updateInternals();
    token.addObject(p_ugrid, vUgrid);

    /* sendInfo("ugrids: %d", domain->getNumberUnstructuredGrids()); */
    /* sendInfo("domain graph: %d", domain->getNumberGraphs()); */
    /* sendInfo("domain rectGrid: %d", domain->getNumberRectilinearGrids()); */
    /* sendInfo("domain regGrid: %d", domain->getNumberRegularGrids()); */
    /* sendInfo("domain Infor: %d", domain->getNumberInformations()); */
    /* shared_ptr<XdmfGridCollection> gridCollect = domain->getGridCollection(0); */
    /* /1* shared_ptr<XdmfUnstructuredGrid> ugrid = gridCollect->getUnstructuredGrid(0); *1/ */
    /* auto ugrid = domain->getUnstructuredGrid(0); */
    /* auto geo = ugrid->getGeometry(); */
    /* sendInfo("Geo Type =  %s #  Points = %u\n", geo->getType()->getName().c_str(), geo->getNumberPoints()); */
    /* std::cout << "Points = " << geo->getValuesString() << '\n'; */

    /* vtkSmartPointer<vtkXdmf3Reader> xreader = vtkSmartPointer<vtkXdmf3Reader>::New(); */
    /* if (!openXdmfVTK(xreader)) */
    /*     return false; */

    /* readXdmfVTK(xreader); */

    //vtkMultiProcessController
    /* vtkSmartPointer<vtkMPIController> controller = vtkMPIController::New(); */
    /* controller->Initialize(); */

    /* vtkMultiProcessController::SetGlobalController(controller); */
    /* vtkSmartPointer<XReaderProcess> xr_proc = XReaderProcess::New(); */
    /* xr_proc->SetArgs(0, nullptr, p_xdmfDir->getValue()); */
    /* controller->SetSingleProcessObject(xr_proc); */
    /* controller->SingleMethodExecute(); */
    /* controller->Finalize(); */
    /* int retVal = xr_proc->GetReturnValue(); */
    /* if (retVal) */
    /*     return true; */
    /* else */
    /*     sendInfo("Unable to execute XReaderProcess."); */
    /* return false; */


    /* auto dataSet = openXdmfVTK(); */
    /* if (!dataSet) */
    /*     sendInfo("No valid dataset."); */

    /* sendInfo("Cells: %d", (int)dataSet->GetNumberOfCells()); */
    /* sendInfo("Points: %d", (int)dataSet->GetNumberOfPoints()); */
    /* /1* sendInfo("InputPorts: %d", (int)dataSet); *1/ */

    /* dataSet->Print(std::cerr); */

    /* // Generate a report */
    /* sendInfo("------------------------"); */
    /* typedef std::map<int, int> CellContainer; */
    /* CellContainer cellMap; */
    /* for (int i = 0; i < dataSet->GetNumberOfCells(); i++) { */
    /*     cellMap[dataSet->GetCellType(i)]++; */
    /* } */

    /* CellContainer::const_iterator it = cellMap.begin(); */
    /* while (it != cellMap.end()) { */
    /*     std::cout << "\tCell type " << vtkCellTypes::GetClassNameFromTypeId(it->first) << " occurs " << it->second */
    /*               << " times." << std::endl; */
    /*     ++it; */
    /* } */

    /* // Now check for point data */
    /* vtkPointData *pd = dataSet->GetPointData(); */
    /* if (pd) { */
    /*     std::cout << " contains point data with " << pd->GetNumberOfArrays() << " arrays." << std::endl; */
    /*     for (int i = 0; i < pd->GetNumberOfArrays(); i++) { */
    /*         std::cout << "\tArray " << i << " is named " << (pd->GetArrayName(i) ? pd->GetArrayName(i) : "NULL") */
    /*                   << std::endl; */
    /*     } */
    /* } */
    /* // Now check for cell data */
    /* vtkCellData *cd = dataSet->GetCellData(); */
    /* if (cd) { */
    /*     std::cout << " contains cell data with " << cd->GetNumberOfArrays() << " arrays." << std::endl; */
    /*     for (int i = 0; i < cd->GetNumberOfArrays(); i++) { */
    /*         std::cout << "\tArray " << i << " is named " << (cd->GetArrayName(i) ? cd->GetArrayName(i) : "NULL") */
    /*                   << std::endl; */
    /*     } */
    /* } */
    /* // Now check for field data */
    /* if (dataSet->GetFieldData()) { */
    /*     std::cout << " contains field data with " << dataSet->GetFieldData()->GetNumberOfArrays() << " arrays." */
    /*               << std::endl; */
    /*     for (int i = 0; i < dataSet->GetFieldData()->GetNumberOfArrays(); i++) { */
    /*         std::cout << "\tArray " << i << " is named " << dataSet->GetFieldData()->GetArray(i)->GetName() */
    /*                   << std::endl; */
    /*     } */
    /* } */

    /* dataSet->Delete(); */

    /*     std::vector<unsigned> start{0, 0}; */
    /*     std::vector<unsigned> stride{1, 1}; */
    /*     std::vector<unsigned> count{1, 100}; */
    /*     std::vector<unsigned> dataSize{1, 100}; */

    /*     HDF5ControllerParameter param(p_h5Dir->getValue(), p_xdmfDir->getValue(), start, stride, count, dataSize, */
    /*                                   XdmfArrayType::Float64()); */
    /*     shared_ptr<XdmfArray> xarr = XdmfArray::New(); */
    /*     readXDMF(xarr, param); */

    return true;
}

/* vtkDataSet *ReadSeisSol::openXdmfVTK2() */
/* { */
/*     auto xreader = vtkXdmf3Reader::New(); */
/*     auto filename = p_xdmfDir->getValue(); */
/*     if (!xreader->CanReadFile(filename.c_str())) { */
/*         sendInfo("No valid xdmf-format or file can't be read by this reader!"); */
/*         return nullptr; */
/*     } */
/*     xreader->SetFileName(filename.c_str()); */
/*     xreader->UpdatePiece(rank(), size(), p_ghost->getValue()); */
/*     xreader->GetOutput()->Register(xreader); */
/*     auto obj = xreader->GetOutputDataObject(0); */
/*     sendInfo("Dataobjecttype: %d", obj->GetDataObjectType()); */
/*     return vtkDataSet::SafeDownCast(xreader->GetOutput()); */
/* } */

/* bool ReadSeisSol::openXdmfVTK(vtkSmartPointer<vtkXdmf3Reader> &xreader) */
/* { */
/*     auto filename = p_xdmfDir->getValue(); */
/*     if (!xreader->CanReadFile(filename.c_str())) { */
/*         sendInfo("No valid xdmf-format or file can't be read by this reader!"); */
/*         return false; */
/*     } */
/*     xreader->SetFileName(filename.c_str()); */
/*     xreader->UpdateInformation(); */
/*     xreader->UpdatePiece(rank(), size(), p_ghost->getValue()); */
/*     return true; */
/* } */

/* bool ReadSeisSol::readXdmfVTK(const vtkSmartPointer<vtkXdmf3Reader> &xreader) */
/* { */
/* auto ugrid = vtkSmartPointer<vtkUnstructuredGrid>::New(); */
/* xreader->SetOutput(ugrid); */
/* ugrid->Print(std::cerr); */
/* auto ds = xreader->GetOutputDataObject(0); */
/* ds->Print(std::cerr); */

/* sendInfo("number elem dataobj: %d", (int)ds->GetNumberOfElements(0)); */

/* sendInfo("oport: %d", xreader->GetNumberOfOutputPorts()); */
/* sendInfo("iport: %d", xreader->GetNumberOfInputPorts()); */
/* auto info = xreader->GetOutputInformation(0); */
/* info->Print(std::cerr); */

/* auto sil = xreader->GetSIL(); */
/* sil->Print(std::cerr); */

/* auto silInfo = sil->GetInformation(); */
/* silInfo->Print(std::cerr); */

/* auto points = sil->GetPoints(); */
/* points->Print(std::cerr); */

/* auto arrP = points->GetData(); */
/* arrP->Print(std::cerr); */
/* for (vtkIdType i = 0; i < arrP->GetSize(); i++) { */
/*     sendInfo("data: %llu", arrP->GetVariantValue(i).ToTypeUInt64()); */
/* } */

/* sendInfo("vertices: %d", (int)sil->GetNumberOfVertices()); */
/* sendInfo("edges: %d", (int)sil->GetNumberOfEdges()); */

/* auto vertices = sil->GetVertexData(); */
/* vertices->Print(std::cerr); */

/* auto dataSet = vtkDataSet::SafeDownCast(xreader->GetOutputDataObject(0)); */
/* if (!dataSet) */
/*     sendInfo("dataSet = null"); */

/* auto datasetInfo = dataSet->GetInformation(); */
/* datasetInfo->Print(std::cerr); */

/* auto fieldData = dataSet->GetFieldData(); */
/* fieldData->Print(std::cerr); */

/* auto cellData = dataSet->GetCellData(); */
/* cellData->Print(std::cerr); */

/* auto pointData = dataSet->GetPointData(); */
/* pointData->Print(std::cerr); */

// Generate a report
/* sendInfo("------------------------"); */
/* typedef std::map<int, int> CellContainer; */
/* CellContainer cellMap; */
/* for (int i = 0; i < dataSet->GetNumberOfCells(); i++) { */
/*     cellMap[dataSet->GetCellType(i)]++; */
/* } */

/* CellContainer::const_iterator it = cellMap.begin(); */
/* while (it != cellMap.end()) { */
/*     std::cout << "\tCell type " << vtkCellTypes::GetClassNameFromTypeId(it->first) << " occurs " << it->second */
/*               << " times." << std::endl; */
/*     ++it; */
/* } */

/* // Now check for point data */
/* vtkPointData *pd = dataSet->GetPointData(); */
/* if (pd) { */
/*     std::cout << " contains point data with " << pd->GetNumberOfArrays() << " arrays." << std::endl; */
/*     for (int i = 0; i < pd->GetNumberOfArrays(); i++) { */
/*         std::cout << "\tArray " << i << " is named " << (pd->GetArrayName(i) ? pd->GetArrayName(i) : "NULL") */
/*                   << std::endl; */
/*     } */
/* } */
/* // Now check for cell data */
/* vtkCellData *cd = dataSet->GetCellData(); */
/* if (cd) { */
/*     std::cout << " contains cell data with " << cd->GetNumberOfArrays() << " arrays." << std::endl; */
/*     for (int i = 0; i < cd->GetNumberOfArrays(); i++) { */
/*         std::cout << "\tArray " << i << " is named " << (cd->GetArrayName(i) ? cd->GetArrayName(i) : "NULL") */
/*                   << std::endl; */
/*     } */
/* } */
/* // Now check for field data */
/* if (dataSet->GetFieldData()) { */
/*     std::cout << " contains field data with " << dataSet->GetFieldData()->GetNumberOfArrays() << " arrays." */
/*               << std::endl; */
/*     for (int i = 0; i < dataSet->GetFieldData()->GetNumberOfArrays(); i++) { */
/*         std::cout << "\tArray " << i << " is named " << dataSet->GetFieldData()->GetArray(i)->GetName() */
/*                   << std::endl; */
/*     } */
/* } */

/* auto output = xreader->GetOutputDataObject(0); */

/* auto ugrid = vtkUnstructuredGrid::SafeDownCast(xreader->GetOutputDataObject(0)); */
/* ugrid->Print(std::cerr); */
/* sendInfo("ugrid->cells: " + std::to_string(ugrid->GetNumberOfCells())); */
/* sendInfo("ugrid->points: " + std::to_string(ugrid->GetNumberOfPoints())); */

/* auto data = vtkDataSet::SafeDownCast(output); */
/* data->Print(std::cerr); */
/* sendInfo("Memory Size:" + std::to_string(output->GetActualMemorySize())); */
/* output->Print(std::cerr); */
/* sendInfo("Elements: " + std::to_string(output->GetNumberOfElements(0))); */

/* auto obj = xreader->GetOutputDataObject(0); */
/* /1* vtkUniformGrid *grid = dynamic_cast<vtkUniformGrid>(obj); *1/ */
/* /1* obj->Print(std::cerr); *1/ */

/* auto field = obj->GetAttributesAsFieldData(1); */
/* field->Print(std::cerr); */

/*     sendInfo("fileseries: " + std::to_string(xreader->GetFileSeriesAsTime())); */
/*     sendInfo("grid: " + std::to_string(xreader->GetNumberOfGrids())); */
/*     sendInfo("cell: " + std::to_string(xreader->GetNumberOfCellArrays())); */
/*     sendInfo("field: " + std::to_string(xreader->GetNumberOfFieldArrays())); */
/*     sendInfo("point: " + std::to_string(xreader->GetNumberOfPointArrays())); */
/*     sendInfo("set array: " + std::to_string(xreader->GetNumberOfSetArrays())); */
/*     sendInfo("sets: " + std::to_string(xreader->GetNumberOfSets())); */


/*     for (int i = 0; i < xreader->GetNumberOfCellArrays(); i++) { */
/*         std::string aName = xreader->GetCellArrayName(i); */
/*         sendInfo("cell arr name " + std::to_string(i) + ": " + aName); */
/*     } */
/*     return true; */
/* } */


bool readHDF5()
{
    return true;
}

bool ReadSeisSol::readXDMF(shared_ptr<XdmfArray> &array, const HDF5ControllerParameter &param)
{
    const auto &comm = Module::comm();
    const auto &processes = size();
    constexpr int cores{4};
    constexpr int dsmSize{64};
    /* const auto &cores = p_cores->getValue(); */
    /* const auto &dsmSize = p_dsm->getValue(); */
    const auto &mpiID = id();

    //own mpi
    /* array = XdmfArray::New(); */
    /* array->initialize(0); */
    /* MPI_Comm workerComm; */
    /* MPI_Group workers, dsmGroup; */

    /* MPI_Comm_group(MPI_COMM_WORLD, &dsmGroup); */
    /* int *ServerIds = (int *)calloc(cores, sizeof(int)); */
    /* unsigned index{0}; */
    /* std::fill_n(ServerIds + processes - cores, ServerIds + processes - 1, [&index]() mutable { return index++; }); */
    /* MPI_Group_excl(dsmGroup, index, ServerIds, &workers); */
    /* int val = MPI_Comm_create(curRank, workers, &workerComm); */
    /* free(ServerIds); */

    shared_ptr<XdmfHDF5WriterDSM> heavyDSMWriter =
        XdmfHDF5WriterDSM::New("", comm, dsmSize / cores, processes - cores, processes - 1);

    if (!heavyDSMWriter) {
        sendInfo("No valid MPI-Parameters.");
        return false;
    }

    heavyDSMWriter->setMode(XdmfHeavyDataWriter::Hyperslab);

    shared_ptr<XdmfHDF5ControllerDSM> controller =
        XdmfHDF5ControllerDSM::New(param.path, param.setPath, param.readType, param.start, param.stride, param.count,
                                   param.dataSize, heavyDSMWriter->getServerBuffer());

    /* shared_ptr<XdmfHDF5Controller> controller = XdmfHDF5Controller::New( */
    /*     param.path, param.setPath, param.readType, param.start, param.stride, param.count, param.dataSize); */

    /* shared_ptr<XdmfHDF5ControllerDSM> controller = XdmfHDF5Controller::New( */
    /*     param.path, param.setPath, param.readType, param.start, param.stride, param.count, param.dataSize); */

    sendInfo(controller->getDataSetPath());
    sendInfo(controller->getName());
    sendInfo(std::to_string(controller->getSize()));
    array->insert(controller);
    array->read();

    MPI_Barrier(comm);

    for (int i = 0; i < array->getSize(); i++)
        sendInfo(std::to_string(array->getValue<float>(i)));

    if (mpiID == 0)
        heavyDSMWriter->stopDSM();

    return true;
}
