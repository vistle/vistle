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

//*************************** TODO: IMPLEMENT XDMF-READER OUT OF THIS*****************************//

//header
#include "ReadSeisSol.h"

//vistle
#include "vistle/core/database.h"
#include "vistle/core/parameter.h"
#include "vistle/core/scalar.h"
#include "vistle/core/unstr.h"
#include "vistle/core/vec.h"
#include "vistle/module/module.h"

//boost
#include <boost/mpi/communicator.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>

//xdmf3
#include <XdmfAttribute.hpp>
#include <XdmfAttributeCenter.hpp>
#include <XdmfAttributeType.hpp>
#include <XdmfGeometry.hpp>
#include <XdmfInformation.hpp>
#include <XdmfGraph.hpp>
#include <XdmfGridCollection.hpp>
#include <XdmfUnstructuredGrid.hpp>
#include <XdmfDomain.hpp>
#include <XdmfReader.hpp>
#include <XdmfArray.hpp>
#include <XdmfArrayType.hpp>
#include <XdmfArrayReference.hpp>
#include <XdmfHDF5WriterDSM.hpp>
#include <XdmfHDF5ControllerDSM.hpp>
#include <XdmfHDF5Controller.hpp>

//std
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <mpi.h>
#include <string>
#include <algorithm>
#include <vector>

using namespace vistle;

MODULE_MAIN(ReadSeisSol)

namespace {
UnstructuredGrid::Type XdmfUgridToVistleUgridType(const shared_ptr<XdmfUnstructuredGrid> &ugrid)
{
    const auto &ugridType = ugrid->getTopology()->getType();
    const std::string &topologyType = ugridType->getName();
    if (topologyType == "Tetrahedron")
        return UnstructuredGrid::TETRAHEDRON;
    else if (topologyType == "Triangle")
        return UnstructuredGrid::TRIANGLE;
    else if (topologyType == "Polygon")
        return UnstructuredGrid::POLYGON;
    else if (topologyType == "Pyramid")
        return UnstructuredGrid::PYRAMID;
    else if (topologyType == "Hexahedron")
        return UnstructuredGrid::HEXAHEDRON;
    else if (topologyType == "Polyvertex")
        return UnstructuredGrid::POINT;
    else if (topologyType == "Polyline")
        return UnstructuredGrid::BAR;
    else if (topologyType == "Quadrilateral")
        return UnstructuredGrid::QUAD;
    else if (topologyType == "Wedge")
        return UnstructuredGrid::PRISM;
    else
        return UnstructuredGrid::NONE;
}
} // namespace

ReadSeisSol::ReadSeisSol(const std::string &name, int moduleID, mpi::communicator comm)
: vistle::Reader("Read ChEESE Seismic Data files (xmdf/hdf5)", name, moduleID, comm)
{
    addStringParameter("xfile", "Xdmf File", "/data/ChEESE/SeisSol/LMU_Sulawesi_example/Sulawesi-surface.xdmf",
                       Parameter::Filename);
    addIntParameter("ghost", "Ghost layer", 1, Parameter::Boolean);
    createOutputPort("ugrid", "UnstructuredGrid");
    createOutputPort("att", "Scalar");

    setParallelizationMode(Serial);
}

bool ReadSeisSol::examine(const vistle::Parameter *param)
{
    setTimesteps(-1);
    examineXdmf();
    return true;
}

bool ReadSeisSol::examineXdmf()
{
    //TODO: initialize geometry and topology and create a map for parameter choice which user want to visualize

    const std::string &xfile = getStringParameter("xfile");

    //read xdmf
    const auto xreader = XdmfReader::New();
    /* const auto xdomain = shared_dynamic_cast<XdmfDomain>(xreader->read(xfile)); */
    /* const auto &xgridCollect = xdomain->getGridCollection(0); */

    /* //geometry and topology is the same for all grid */
    /* const auto &xugrid = xgridCollect->getUnstructuredGrid(0); */

    /* const auto &xtopology = xugrid->getTopology(); */
    /* const auto &xgeometry = xugrid->getGeometry(); */
    /* const auto &time = xugrid->getTime(); */
    return true;
}

bool ReadSeisSol::read(Token &token, int timestep, int block)
{
    /** TODO:
      * - Polish
      * - Timesteps
      * - Distribute across MPI processes
      */

    constexpr unsigned gridColNum{0};
    constexpr unsigned gridNum{3};
    constexpr unsigned gridAtt{1};

    const std::string xfile = getStringParameter("xfile");

    //read xdmf
    const auto xreader = XdmfReader::New();
    const auto xdomain = shared_dynamic_cast<XdmfDomain>(xreader->read(xfile));
    const auto &xgridCollect = xdomain->getGridCollection(gridColNum);

    //geometry and topology is the same for all grid
    const auto &xugrid = xgridCollect->getUnstructuredGrid(gridNum);
    const auto &xtopology = xugrid->getTopology();
    const auto &xgeometry = xugrid->getGeometry();
    const auto &xtime = xugrid->getTime();
    for (auto [name, val]: xtime->getItemProperties())
        sendInfo("%s: %s", name.c_str(), val.c_str());
    sendInfo("itemtag: %s", xtime->getItemTag().c_str());


    //read connectionlist
    const shared_ptr<XdmfArray> xArrConn(XdmfArray::New());
    xArrConn->insert(xtopology->getHeavyDataController());
    xArrConn->read();

    //read geometry
    const shared_ptr<XdmfArray> xArrGeo(XdmfArray::New());
    xArrGeo->insert(xgeometry->getHeavyDataController());
    xArrGeo->read();

    //create unstructured grid
    UnstructuredGrid::ptr ugrid_ptr(
        new UnstructuredGrid(xtopology->getNumberElements(), xArrConn->getSize(), xArrGeo->getSize() / 3));

    const int &numCornerPerElem = xtopology->getDimensions().at(1);

    //connectionlist
    auto cl = ugrid_ptr->cl().data();
    xArrConn->getValues(0, cl, xArrConn->getSize());

    //DEBUG
    //invert elementvertices for tetrahedron => vtk vertice order = inverse vistle vertice order
    /* for (unsigned i = 0; i < xArrConn->getSize(); i++) { */
    /*     auto tmp = i; */
    /*     cl[i++] = xArrConn->getValue<int>(tmp+3); */
    /*     cl[i++] = xArrConn->getValue<int>(tmp+2); */
    /*     cl[i++] = xArrConn->getValue<int>(tmp+1); */
    /*     cl[i] = xArrConn->getValue<int>(tmp); */
    /* } */

    //coords
    auto x = ugrid_ptr->x().data(), y = ugrid_ptr->y().data(), z = ugrid_ptr->z().data();

    //arrGeo => x1 y1 z1 x2 y2 z2 x3 y3 z3 ... xn yn zn
    xArrGeo->getValues(0, x, xArrGeo->getSize() / 3, 3, 1);
    xArrGeo->getValues(1, y, xArrGeo->getSize() / 3, 3, 1);
    xArrGeo->getValues(2, z, xArrGeo->getSize() / 3, 3, 1);

    //4 corner = tetrahedron
    std::generate(ugrid_ptr->el().begin(), ugrid_ptr->el().end(),
                  [n = 0, &numCornerPerElem]() mutable { return n++ * numCornerPerElem; });

    //typelist
    std::fill(ugrid_ptr->tl().begin(), ugrid_ptr->tl().end(), XdmfUgridToVistleUgridType(xugrid));

    if (!ugrid_ptr->check()) {
        sendInfo("Something went wrong while creation of ugrid! Possible errors:\n"
                 "-invalid connectivity list \n"
                 "-invalid coordinates\n"
                 "-invalid typelist\n"
                 "-invalid geo type");
        return false;
    }

    // Attribute visualize with hyperslap
    const auto &xattribute = xugrid->getAttribute(gridAtt);

    auto type = xattribute->getArrayType();
    if (type == XdmfArrayType::Int8()) {
        sendInfo("int8");
    } else if (type == XdmfArrayType::Int16()) {
        sendInfo("int16");
    } else if (type == XdmfArrayType::Int32()) {
        sendInfo("int32");
    } else if (type == XdmfArrayType::Int64()) {
        sendInfo("int64");
    } else if (type == XdmfArrayType::Float32()) {
        sendInfo("float32");
    } else if (type == XdmfArrayType::Float64()) {
        sendInfo("float64");
    } else if (type == XdmfArrayType::UInt8()) {
        sendInfo("UInt8");
    } else if (type == XdmfArrayType::UInt16()) {
        sendInfo("UInt16");
    } else if (type == XdmfArrayType::UInt32()) {
        sendInfo("UInt32");
    } else if (type == XdmfArrayType::String()) {
        sendInfo("String");
    }

    auto readmode = xattribute->getReadMode();
    if (readmode == XdmfArray::Controller)
        sendInfo("controller");
    else if (readmode == XdmfArray::Reference)
        sendInfo("Reference");

    auto attCenter = xattribute->getCenter();
    if (attCenter == XdmfAttributeCenter::Grid())
        sendInfo("grid");
    else if (attCenter == XdmfAttributeCenter::Cell())
        sendInfo("cell");
    else if (attCenter == XdmfAttributeCenter::Node())
        sendInfo("node");
    else if (attCenter == XdmfAttributeCenter::Other())
        sendInfo("other");

    sendInfo("att name: %s", xattribute->getName().c_str());
    sendInfo("xattribute->dimension: %d", xattribute->getDimensions().at(1));
    Vec<Scalar>::ptr att(new Vec<Scalar>(xattribute->getSize()));
    const shared_ptr<XdmfArray> xArrAtt(XdmfArray::New());
    xArrAtt->insert(xattribute->getHeavyDataController());
    xArrAtt->read();

    auto ux = att->x().data();
    /* double *internalAtt_ptr = static_cast<double *>(xArrAtt->getValuesInternal()); */

    /* std::cout << std::setprecision(8); */
    /* std::cout << "xArrAtt:" << '\n'; */
    /* std::cout << '\n'; */
    /* for (int i = xattribute->getDimensions().at(0); i < xattribute->getDimensions().at(1) - 1; i++) */
    /*     std::cout << i << ":" << xArrAtt->getValue<float>(i) << '\n'; */

    /* std::copy_n(internalAtt_ptr + xattribute->getDimensions().at(1) * gridNum, xattribute->getDimensions().at(0) - 1, */
    /*             ux); */
    xArrAtt->getValues(xattribute->getDimensions().at(1), ux, xattribute->getDimensions().at(1), 1, 1);
    /* xattribute->getValues(xattribute->getDimensions().at(1), ux, xattribute->getDimensions().at(1), 1, 1); */

    /* std::cout << "ux:" << '\n'; */
    /* std::cout << '\n'; */
    /* for (int i = 0; i < xattribute->getDimensions().at(1); i++) */
    /*     std::cout << i << ":" << ux[i] << '\n'; */

    ugrid_ptr->setBlock(block);
    ugrid_ptr->setTimestep(-1);
    ugrid_ptr->updateInternals();

    att->setGrid(ugrid_ptr);
    att->setBlock(block);
    att->setMapping(DataBase::Element);
    att->addAttribute("_species", "partition");
    att->setTimestep(-1);
    att->updateInternals();

    token.addObject("ugrid", ugrid_ptr);
    sendInfo("Teschd");
    token.addObject("att", att);
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
