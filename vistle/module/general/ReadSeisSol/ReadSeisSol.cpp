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
#include <boost/mpi/communicator.hpp>

//xdmf3
#include <XdmfGeometry.hpp>
#include <XdmfInformation.hpp>
#include <XdmfGraph.hpp>
#include <XdmfGridCollection.hpp>
#include <XdmfUnstructuredGrid.hpp>
#include <XdmfDomain.hpp>
#include <XdmfReader.hpp>
#include <XdmfArray.hpp>
#include <XdmfHDF5WriterDSM.hpp>
#include <XdmfHDF5ControllerDSM.hpp>
#include <XdmfHDF5Controller.hpp>

//std
#include <memory>
#include <mpi.h>
#include <string>
#include <algorithm>
#include <vector>

using namespace vistle;

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
    //read xdmf
    const auto xreader = XdmfReader::New();
    const auto xdomain = shared_dynamic_cast<XdmfDomain>(xreader->read(p_xdmfDir->getValue()));
    const auto &xgridCollect = xdomain->getGridCollection(0);
    const auto &xugrid = xgridCollect->getUnstructuredGrid(0);
    const auto &xtopology = xugrid->getTopology();
    const auto &xgeometry = xugrid->getGeometry();

    //read connectionlist
    const shared_ptr<XdmfArray> xArrConn(XdmfArray::New());
    xArrConn->insert(xtopology->getHeavyDataController());
    xArrConn->read();

    //read geometry
    const shared_ptr<XdmfArray> xArrGeo(XdmfArray::New());
    xArrGeo->insert(xgeometry->getHeavyDataController());
    xArrGeo->read();

    //create unstructured grid
    UnstructuredGrid::ptr ugrid_ptr(new UnstructuredGrid(xtopology->getNumberElements(), xArrConn->getSize(),
                                                         xArrGeo->getSize() / 3));

    const int &numCornerPerElem = xtopology->getDimensions().at(1);

    //connectionlist
    auto cl = ugrid_ptr->cl().data();
    for (unsigned i = 0; i < xArrConn->getSize(); i++)
        cl[i] = xArrConn->getValue<int>(i);

    //coords
    auto x = ugrid_ptr->x().data(), y = ugrid_ptr->y().data(), z = ugrid_ptr->z().data();

    //arrGeo => x1 y1 z1 x2 y2 z2 x3 y3 z3 ... xn yn zn
    unsigned i{0};
    unsigned n{0};
    while (n < xArrGeo->getSize() / 3) {
        x[n] = xArrGeo->getValue<float>(i++);
        y[n] = xArrGeo->getValue<float>(i++);
        z[n++] = xArrGeo->getValue<float>(i++);
    }

    //4 corner = tetrahedron
    std::generate(ugrid_ptr->el().begin(), ugrid_ptr->el().end(),
                  [n = 0, &numCornerPerElem]() mutable { return n++ * numCornerPerElem; });

    //typelist
    std::fill(ugrid_ptr->tl().begin(), ugrid_ptr->tl().end(), UnstructuredGrid::TETRAHEDRON);

    if (!ugrid_ptr->check()) {
        sendInfo("Something went wrong while creating of ugrid!");
        return false;
    }

    ugrid_ptr->setBlock(block);
    ugrid_ptr->setTimestep(-1);
    ugrid_ptr->updateInternals();
    token.addObject(p_ugrid, ugrid_ptr);

    return true;
}

bool readHDF5()
{
    return true;
}

/* bool ReadSeisSol::readXDMF(shared_ptr<XdmfArray> &array, const HDF5ControllerParameter &param) */
/* { */
/*     const auto &comm = Module::comm(); */
/*     const auto &processes = size(); */
/*     constexpr int cores{4}; */
/*     constexpr int dsmSize{64}; */
/*     const auto &mpiID = id(); */

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

/* sendInfo(controller->getDataSetPath()); */
/* sendInfo(controller->getName()); */
/* sendInfo(std::to_string(controller->getSize())); */
/* array->insert(controller); */
/* array->read(); */

/* MPI_Barrier(comm); */

/* for (unsigned i = 0; i < array->getSize(); i++) */
/*     sendInfo(std::to_string(array->getValue<float>(i))); */

/* if (mpiID == 0) */
/*     heavyDSMWriter->stopDSM(); */

/* return true; */
/* } */
