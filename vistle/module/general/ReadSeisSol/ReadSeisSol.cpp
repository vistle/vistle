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
#include "vistle/module/module.h"

//boost
#include <boost/mpi/communicator.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>

//vtk
#include <vtkDataObject.h>
#include <vtkType.h>
#include <vtkFieldData.h>

//xdmf3
/* #include <XdmfGeometry.hpp> */
/* #include <XdmfGridCollection.hpp> */
/* #include <XdmfArrayType.hpp> */
/* #include <XdmfUnstructuredGrid.hpp> */
/* #include <XdmfDomain.hpp> */
/* #include <XdmfReader.hpp> */
/* #include <XdmfArray.hpp> */
/* #include <XdmfHDF5WriterDSM.hpp> */
/* #include <XdmfHDF5ControllerDSM.hpp> */
/* #include <XdmfHDF5Controller.hpp> */

//std
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mpi.h>
#include <string>
#include <algorithm>
#include <vector>

using namespace vistle;
/* using namespace H5; */

MODULE_MAIN(ReadSeisSol)

ReadSeisSol::ReadSeisSol(const std::string &name, int moduleID, mpi::communicator comm)
: vistle::Reader("Read ChEESE Seismic Data files (xmdf/hdf5)", name, moduleID, comm)
{
    p_h5Dir = addStringParameter("file_dir", "HDF5 File directory", "/data/ChEESE/SeisSol/LMU_Sulawesi_example/",
                                 Parameter::ExistingDirectory);
    p_xdmfDir = addStringParameter("file", "Xdmf File", "/data/ChEESE/SeisSol/LMU_Sulawesi_example/Sulawesi.xdmf",
                                   Parameter::Filename);
    /* p_dsm = addIntParameter("Distributed Shared Memory", "Xdmf server distributed memorysize", 64); */
    /* p_cores = addIntParameter("Servercores", "Servercores", 4); */
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
    /* auto reader = XdmfReader::New(); */
    /* auto domain = shared_dynamic_cast<XdmfDomain>(reader->read(p_xdmfDir->getValue())); */
    
    /* shared_ptr<XdmfGridCollection> gridCollect = domain->getGridCollection(0); */
    /* shared_ptr<XdmfUnstructuredGrid> ugrid = gridCollect->getUnstructuredGrid(0); */
    /* auto ugrid = domain->getUnstructuredGrid(0); */
    /* auto geo = ugrid->getGeometry(); */
    /* sendInfo("Geo Type =  %s #  Points = %u\n", geo->getType()->getName().c_str(), geo->getNumberPoints()); */
    /* std::cout << "Points = " << geo->getValuesString() << '\n'; */
    vtkSmartPointer<vtkXdmf3Reader> xreader = vtkSmartPointer<vtkXdmf3Reader>::New();
    if (!openXDMF(xreader)) {
        xreader->Delete();
        return false;
    }

    readXDMF(xreader);

    /* HDF5ControllerParameter param{}; */
    /* xreader->Delete(); */
    /* std::vector<unsigned> start{0, 0}; */
    /* std::vector<unsigned> stride{1, 1}; */
    /* std::vector<unsigned> count{1, 100}; */
    /* std::vector<unsigned> dataSize{1, 100}; */

    /* HDF5ControllerParameter param(p_h5Dir->getValue(), p_xdmfDir->getValue(), start, stride, count, dataSize, */
    /*                               XdmfArrayType::Float64()); */
    /* shared_ptr<XdmfArray> xarr = XdmfArray::New(); */
    /* readXDMF(xarr, param); */
    return true;
}

bool ReadSeisSol::openXDMF(vtkSmartPointer<vtkXdmf3Reader> &xreader)
{
    auto filename = p_xdmfDir->getValue();
    if (!xreader->CanReadFile(filename.c_str())) {
        sendInfo("No valid xdmf-format or file can't be read by this reader!");
        return false;
    }
    xreader->SetFileName(filename.c_str());
    xreader->Update();
    return true;
}

bool ReadSeisSol::readXDMF(const vtkSmartPointer<vtkXdmf3Reader> &xreader)
{
    xreader->Print(std::cerr);

    auto output = xreader->GetOutput();
    output->Print(std::cerr);
    sendInfo("output-points: " + std::to_string(output->GetNumberOfElements(0)));

    auto obj = xreader->GetOutputDataObject(0);
    obj->Print(std::cerr);

    auto field = obj->GetAttributesAsFieldData(1);
    field->Print(std::cerr);

    sendInfo("obj-memSize: " + std::to_string(obj->GetActualMemorySize()));
    sendInfo("obj-numElem: " + std::to_string(obj->GetNumberOfElements(0)));
    sendInfo("fileseries: " + std::to_string(xreader->GetFileSeriesAsTime()));
    sendInfo("grid: " + std::to_string(xreader->GetNumberOfGrids()));
    sendInfo("cell: " + std::to_string(xreader->GetNumberOfCellArrays()));
    sendInfo("field: " + std::to_string(xreader->GetNumberOfFieldArrays()));
    sendInfo("point: " + std::to_string(xreader->GetNumberOfPointArrays()));
    sendInfo("set array: " + std::to_string(xreader->GetNumberOfSetArrays()));
    sendInfo("set: " + std::to_string(xreader->GetNumberOfSets()));
    for (int i = 0; i < xreader->GetNumberOfCellArrays(); i++) {
        std::string aName = xreader->GetCellArrayName(i);
        sendInfo("cell arr name " + std::to_string(i) + ": " + aName);
    }
    return true;
}

/* bool ReadSeisSol::readXDMF(shared_ptr<XdmfArray> &array, const HDF5ControllerParameter &param) */
/* { */

    /* const auto &comm = Module::comm(); */
    /* const auto &processes = size(); */
    /* const auto &cores = p_cores->getValue(); */
    /* const auto &dsmSize = p_dsm->getValue(); */
    /* const auto &mpiID = id(); */

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

    /* shared_ptr<XdmfHDF5WriterDSM> heavyDSMWriter = */
    /*     XdmfHDF5WriterDSM::New("", comm, dsmSize / cores, processes - cores, processes - 1); */

    /* if (!heavyDSMWriter) { */
    /*     sendInfo("No valid MPI-Parameters."); */
    /*     return false; */
    /* } */

    /* heavyDSMWriter->setMode(XdmfHeavyDataWriter::Hyperslab); */

    /* shared_ptr<XdmfHDF5ControllerDSM> controller = */
    /*     XdmfHDF5ControllerDSM::New(param.path, param.setPath, param.readType, param.start, param.stride, param.count, */
    /*                                param.dataSize, heavyDSMWriter->getServerBuffer()); */

    /* shared_ptr<XdmfHDF5Controller> controller = XdmfHDF5Controller::New( */
    /*     param.path, param.setPath, param.readType, param.start, param.stride, param.count, param.dataSize); */

    /* shared_ptr<XdmfHDF5ControllerDSM> controller = */
    /* XdmfHDF5Controller::New(param.path, param.setPath, param.readType, param.start, param.stride, */
    /*                            param.count, param.dataSize); */

    /* sendInfo(controller->getDataSetPath()); */
    /* sendInfo(controller->getName()); */
    /* sendInfo(std::to_string(controller->getSize())); */
    /* array->insert(controller); */
    /* array->read(); */

    /* MPI_Barrier(comm); */

    /* for (int i = 0; i < array->getSize(); i++) { */
    /*     sendInfo(std::to_string(array->getValue<float>(i))); */
    /*     ; */
    /* } */

    /* if (mpiID == 0) */
    /*     heavyDSMWriter->stopDSM(); */

    /* return true; */
/* } */
