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
#include <XdmfSharedPtr.hpp>
#include <XdmfTime.hpp>
#include <XdmfItem.hpp>
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
#include <cstring>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <mpi.h>
#include <numeric>
#include <string>
#include <algorithm>
#include <vector>

using namespace vistle;

MODULE_MAIN(ReadSeisSol)

namespace {
const UnstructuredGrid::Type XdmfUgridToVistleUgridType(XdmfUnstructuredGrid *ugrid)
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
    m_xfile = addStringParameter(
        "xfile", "Xdmf File", "/data/ChEESE/SeisSol/LMU_Sulawesi_example/Sulawesi-surface.xdmf", Parameter::Filename);

    addIntParameter("ghost", "Ghost layer", 1, Parameter::Boolean);
    createOutputPort("ugrid", "UnstructuredGrid");
    createOutputPort("att", "Scalar");

    observeParameter(m_xfile);

    setParallelizationMode(Serial);
}

void ReadSeisSol::clearChoice()
{
    for (auto param: m_attChoice) {
        removeParameter(param);
    }
    m_attChoice.clear();
}

void ReadSeisSol::iterateXdmfGridCollection(const boost::shared_ptr<XdmfGridCollection> &xgridCol)
{
    if (xgridCol != nullptr) {
        const auto &gridColType = xgridCol->getType();
        if (gridColType == XdmfGridCollectionType::Temporal()) {
            for (auto i = 0; i < xgridCol->getNumberUnstructuredGrids(); i++)
                iterateXdmfUnstrGrid(xgridCol->getUnstructuredGrid(i));
            sendInfo("Other gridtypes than unstructured have not been implemented yet!");
        } else if (gridColType == XdmfGridCollectionType::Spatial())
            sendInfo("Spatial collectiontype not implemented yet!");
        else
            sendInfo("No implementation without xdmfcollection.");
    }
}

void ReadSeisSol::iterateXdmfUnstrGrid(const boost::shared_ptr<XdmfUnstructuredGrid> &xugrid)
{
    if (xugrid != nullptr) {
        /* iterateXdmfTopology(xugrid->getTopology()); */
        /* iterateXdmfGeometry(xugrid->getGeometry()); */
        /* iterateXdmfTime(xugrid->getTime()); */
        clearChoice();

        const auto &numParam = xugrid->getNumberAttributes();

        for (auto i = 0; i < numParam; i++)
            iterateXdmfAttribute(xugrid->getAttribute(i));
    }
}

bool ReadSeisSol::XAttributeInSet(const std::string &name)
{
    return std::any_of(m_attChoice.begin(), m_attChoice.end(),
                       [&name](auto param) { return param->getName() == name; });
}

void ReadSeisSol::iterateXdmfAttribute(const boost::shared_ptr<XdmfAttribute> &xatt)
{
    if (xatt != nullptr) {
        if (XAttributeInSet(xatt->getName()))
            return;
        auto param = addIntParameter(xatt->getName(), "Grid parameter", 1, Parameter::Boolean);
        m_attChoice.insert(param);
    }
}

void ReadSeisSol::iterateXdmfDomain(const boost::shared_ptr<XdmfDomain> &xdomain)
{
    if (xdomain != nullptr)
        for (auto i = 0; i < xdomain->getNumberGridCollections(); i++)
            iterateXdmfGridCollection(xdomain->getGridCollection(i));
}

/**
  *
  */
bool ReadSeisSol::examineXdmf()
{
    const std::string &xfile = m_xfile->getValue();

    //read xdmf
    const auto xreader = XdmfReader::New();

    iterateXdmfDomain(shared_dynamic_cast<XdmfDomain>(xreader->read(xfile)));
    return true;
}

/**
  *
  */
bool ReadSeisSol::examine(const vistle::Parameter *param)
{
    setTimesteps(-1);
    if (param == m_xfile)
        return examineXdmf();
    return false;
}


/**
  *
  */
void ReadSeisSol::fillUnstrGridTypeList(vistle::UnstructuredGrid::ptr unstr, const vistle::UnstructuredGrid::Type &type)
{
    std::fill(unstr->tl().begin(), unstr->tl().end(), type);
}

/**
  *
  */
template<class T>
void ReadSeisSol::fillUnstrGridElemList(vistle::UnstructuredGrid::ptr unstr, const T &numCornerPerElem)
{
    //4 corner = tetrahedron
    std::generate(unstr->el().begin(), unstr->el().end(),
                  [n = 0, &numCornerPerElem]() mutable { return n++ * numCornerPerElem; });
}

/**
  *
  */
bool ReadSeisSol::fillUnstrGridCoords(vistle::UnstructuredGrid::ptr unstr, XdmfArray *xArrGeo)
{
    auto x = unstr->x().data(), y = unstr->y().data(), z = unstr->z().data();

    //maybe depending on dim
    constexpr auto numCoords{3};
    constexpr auto strideVistleArr{1};

    //TODO: make it generic?
    //arrGeo => x1 y1 z1 x2 y2 z2 x3 y3 z3 ... xn yn zn
    xArrGeo->getValues(0, x, xArrGeo->getSize() / numCoords, numCoords, strideVistleArr);
    xArrGeo->getValues(1, y, xArrGeo->getSize() / numCoords, numCoords, strideVistleArr);
    xArrGeo->getValues(2, z, xArrGeo->getSize() / numCoords, numCoords, strideVistleArr);

    return true;
}

/**
  *
  */
bool ReadSeisSol::fillUnstrGridConnectList(vistle::UnstructuredGrid::ptr unstr, XdmfArray *xArrConn)
{
    //TODO: DEBUG invert elementvertices for tetrahedron => vtk vertice order = inverse vistle vertice order for (unsigned i = 0; i < xArrConn->getSize(); i++) { */
    auto cl = unstr->cl().data();
    xArrConn->getValues(0, cl, xArrConn->getSize());
    return true;
}

/**
  *
  */
void ReadSeisSol::readXdmfHeavyController(XdmfArray *xArr, const boost::shared_ptr<XdmfHeavyDataController> &controller)
{
    xArr->insert(controller);
    xArr->read();
}

/**
  *
  */
vistle::UnstructuredGrid::ptr ReadSeisSol::generateUnstrGridFromXdmfGrid(XdmfUnstructuredGrid *unstr)
{
    //TODO: geometry always the same for each ugrid
    const auto &xtopology = unstr->getTopology();
    const auto &xgeometry = unstr->getGeometry();
    const int &numCornerPerElem = xtopology->getDimensions().at(1);

    //read connectionlist
    const shared_ptr<XdmfArray> xArrConn(XdmfArray::New());
    readXdmfHeavyController(xArrConn.get(), xtopology->getHeavyDataController());

    //read geometry
    const shared_ptr<XdmfArray> xArrGeo(XdmfArray::New());
    readXdmfHeavyController(xArrGeo.get(), xgeometry->getHeavyDataController());

    UnstructuredGrid::ptr unstr_ptr(
        new UnstructuredGrid(xtopology->getNumberElements(), xArrConn->getSize(), xArrGeo->getSize() / 3));

    fillUnstrGridConnectList(unstr_ptr, xArrConn.get());
    fillUnstrGridCoords(unstr_ptr, xArrGeo.get());
    fillUnstrGridElemList(unstr_ptr, numCornerPerElem);
    fillUnstrGridTypeList(unstr_ptr, XdmfUgridToVistleUgridType(unstr));

    //return copy of shared_ptr
    return unstr_ptr;
}

/**
  *
  */
void ReadSeisSol::setArrayType(boost::shared_ptr<const XdmfArrayType> type)
{
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
}

/**
  *
  */
void ReadSeisSol::setGridCenter(boost::shared_ptr<const XdmfAttributeCenter> attCenter)
{
    if (attCenter == XdmfAttributeCenter::Grid())
        sendInfo("grid");
    else if (attCenter == XdmfAttributeCenter::Cell())
        sendInfo("cell");
    else if (attCenter == XdmfAttributeCenter::Node())
        sendInfo("node");
    else if (attCenter == XdmfAttributeCenter::Other())
        sendInfo("other");
}

/**
  *
  */
bool ReadSeisSol::read(Token &token, int timestep, int block)
{
    /** TODO:
      * - Polish
      * - Timesteps
      * - Distribute across MPI processes
      */
    constexpr unsigned gridColNum{0};
    constexpr unsigned gridStep{3};
    constexpr unsigned gridAtt{1};

    const std::string xfile = m_xfile->getValue();

    //read xdmf
    const auto xreader = XdmfReader::New();
    const auto xdomain = shared_dynamic_cast<XdmfDomain>(xreader->read(xfile));
    const auto &xgridCollect = xdomain->getGridCollection(gridColNum);

    //geometry and topology is the same for all grid
    const auto &xugrid = xgridCollect->getUnstructuredGrid(gridStep);

    //create unstructured grid
    auto ugrid_ptr = generateUnstrGridFromXdmfGrid(xugrid.get());

    if (!ugrid_ptr->check()) {
        sendInfo("Something went wrong while creation of ugrid! Possible errors:\n"
                 "-invalid connectivity list \n"
                 "-invalid coordinates\n"
                 "-invalid typelist\n"
                 "-invalid geo type");
        return false;
    }

    // Attribute visualize without hyperslap
    const auto &xattribute = xugrid->getAttribute(gridAtt);
    const auto attDim = xattribute->getDimensions().at(1);

    auto type = xattribute->getArrayType();
    setArrayType(type);

    auto readmode = xattribute->getReadMode();
    if (readmode == XdmfArray::Controller)
        sendInfo("controller");
    else if (readmode == XdmfArray::Reference)
        sendInfo("Reference");

    auto attCenter = xattribute->getCenter();
    setGridCenter(attCenter);

    sendInfo("att name: %s", xattribute->getName().c_str());
    sendInfo("xattribute->dimension: %d", xattribute->getDimensions().at(1));

    Vec<Scalar>::ptr att(new Vec<Scalar>(attDim));
    const shared_ptr<XdmfArray> xArrAtt(XdmfArray::New());
    readXdmfHeavyController(xArrAtt.get(), xattribute->getHeavyDataController());

    auto ux = att->x().data();

    //without memcpy
    xArrAtt->getValues<float>(attDim * gridStep, ux, attDim, 1, 1);

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
    token.addObject("att", att);

    return true;
}

/**
  *
  */
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
