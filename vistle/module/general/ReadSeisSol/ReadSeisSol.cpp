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
#include "vistle/core/port.h"
#include "vistle/core/database.h"
#include "vistle/core/parameter.h"
#include "vistle/core/scalar.h"
#include "vistle/core/unstr.h"
#include "vistle/core/vec.h"
#include "vistle/module/module.h"
#include "vistle/util/enum.h"

//boost
#include <boost/mpi/communicator.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>

//xdmf3
#include <XdmfAttribute.hpp>
#include <XdmfTopology.hpp>
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
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <mpi.h>
#include <numeric>
#include <string>
#include <algorithm>
#include <type_traits>
#include <vector>

using namespace vistle;

MODULE_MAIN(ReadSeisSol)

namespace {

//SeisSol Mode
DEFINE_ENUM_WITH_STRING_CONVERSIONS(SeisSolMode, (XDMF)(HDF5))

/**
  * Extracts XdmfArrayType from ugrid and returns corresponding vistle UnstructuredGrid::Type.
  *
  * @ugrid: Pointer to XdmfUnstructuredGrid.
  *
  * @return: Corresponding vistle type.
  */
const UnstructuredGrid::Type XdmfUgridToVistleUgridType(const XdmfUnstructuredGrid *ugrid)
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

/**
  * Checks if suffix string is the end of main.
  *
  * @main: main string.
  * @suffix: suffix string.
  *
  * @return: True if given suffix is end of main.
  */
bool checkEnd(const std::string &main, const std::string &suffix)
{
    if (main.size() >= suffix.size())
        if (main.compare(main.size() - suffix.size(), suffix.size(), suffix) == 0)
            return true;
    return false;
}
} // namespace

/**
  * Constructor.
  */
ReadSeisSol::ReadSeisSol(const std::string &name, int moduleID, mpi::communicator comm)
: vistle::Reader("Read ChEESE Seismic Data files (xmdf/hdf5)", name, moduleID, comm)
{
    //settings
    m_file = addStringParameter("xfile", "Xdmf File", "/data/ChEESE/SeisSol/LMU_Sulawesi_example/sula-surface.xdmf",
                                Parameter::Filename);
    m_seisMode = addIntParameter("SeisSolMode", "Select read format (HDF5 or Xdmf)", (Integer)XDMF, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_seisMode, SeisSolMode);
    m_xattributes = addStringParameter("Parameter", "test", "", Parameter::Choice);
    m_ghost = addIntParameter("ghost", "Ghost layer", 1, Parameter::Boolean);

    //ports
    m_gridOut = createOutputPort("ugrid", "UnstructuredGrid");
    m_scalarOut = createOutputPort("att", "Scalar");

    //observer
    observeParameter(m_file);
    observeParameter(m_xattributes);
    observeParameter(m_seisMode);

    //parallel mode
    setParallelizationMode(Serial);
}

/**
  * Template for switching between XDMF and HDF5 mode. Calls given function with args according to current mode.
  * @xdmfFunc: Function that will be called when in XDMF mode.
  * @hdfFunc: Function that will be called when in HDF5 mode.
  *
  * return: True if function has been called correctly.
  */
template<class... Args>
bool ReadSeisSol::switchSeisMode(std::function<bool(Args...)> xdmfFunc, std::function<bool(Args...)> hdfFunc,
                                 Args... args)
{
    switch (m_seisMode->getValue()) {
    case XDMF:
        return xdmfFunc(args...);
    case HDF5:
        return hdfFunc(args...);
    }
    return false;
}

/**
  * Helper function to tell user that HDF5 mode is not implemented.
  *
  * @return: false.
  */
bool ReadSeisSol::hdfModeNotImplemented()
{
    sendInfo("HDF5 mode is not implemented.");
    return false;
}
bool ReadSeisSol::hdfModeNotImplementedRead(Token &, const int &, const int &)
{
    return hdfModeNotImplemented();
}

/**
  * Clears attributes choice.
  */
void ReadSeisSol::clearChoice()
{
    m_xattributes->setChoices(std::vector<std::string>());
    m_attChoiceStr.clear();
}

/**
  * Iterate through properties of a xdmfgridcollection.
  *
  * @xgridCol: Constant grid collection pointer.
  */
void ReadSeisSol::inspectXdmfGridCollection(const XdmfGridCollection *xgridCol)
{
    if (xgridCol != nullptr) {
        const auto &gridColType = xgridCol->getType();
        if (gridColType == XdmfGridCollectionType::Temporal()) {
            //at the moment attribute number and type are the same for all ugrids.
            if (xgridCol->getNumberUnstructuredGrids()) {
                setTimesteps(xgridCol->getNumberUnstructuredGrids());
                inspectXdmfUnstrGrid(xgridCol->getUnstructuredGrid(1).get());
            }
            if (xgridCol->getNumberCurvilinearGrids() | xgridCol->getNumberRectilinearGrids() |
                xgridCol->getNumberRegularGrids())
                sendInfo("Other gridtypes than unstructured have not been implemented yet!");
        } else if (gridColType == XdmfGridCollectionType::Spatial())
            sendInfo("Spatial collectiontype not implemented yet!");
        else
            sendInfo("No implementation without xdmfcollection.");
    }
}

/**
  * Iterate through unstructured grid attributes stored in an xdmf.
  *
  * @xugrid: const unstructured grid pointer.
  */
void ReadSeisSol::inspectXdmfUnstrGrid(const XdmfUnstructuredGrid *xugrid)
{
    if (xugrid != nullptr) {
        //TODO: NOT IMPLEMENTED YET
        inspectXdmfTopology(xugrid->getTopology().get());
        inspectXdmfGeometry(xugrid->getGeometry().get());
        inspectXdmfTime(xugrid->getTime().get());

        clearChoice();
        for (unsigned i = 0; i < xugrid->getNumberAttributes(); i++)
            inspectXdmfAttribute(xugrid->getAttribute(i).get());
    }
}

void ReadSeisSol::inspectXdmfGeometry(const XdmfGeometry *xgeo)
{
    if (xgeo != nullptr) {
        // set geo global ?
    }
}

void ReadSeisSol::inspectXdmfTime(const XdmfTime *xtime)
{
    if (xtime != nullptr) {
        // set timestep defined in xdmf
    }
}

void ReadSeisSol::inspectXdmfTopology(const XdmfTopology *xtopo)
{
    if (xtopo != nullptr) {
        // set topo global ?
    }
}

/**
  * Inspect XdmfAttribute and generate a parameter in reader.
  *
  * @xatt: Const attribute pointer.
  */
void ReadSeisSol::inspectXdmfAttribute(const XdmfAttribute *xatt)
{
    if (xatt != nullptr)
        m_attChoiceStr.push_back(xatt->getName());
}

/**
  * Inspect XdmfDomain.
  *
  * @xdomain: Const pointer of domain-object.
  */
void ReadSeisSol::inspectXdmfDomain(const XdmfDomain *xdomain)
{
    if (xdomain != nullptr)
        for (unsigned i = 0; i < xdomain->getNumberGridCollections(); i++)
            inspectXdmfGridCollection(xdomain->getGridCollection(i).get());
}

/**
  * Inspect Xdmf.
  *
  * return: False if something went wrong while inspection else true.
  */
bool ReadSeisSol::inspectXdmf()
{
    const std::string &xfile = m_file->getValue();

    //read xdmf
    const auto xreader = XdmfReader::New();

    if (xreader.get() == nullptr)
        return false;

    inspectXdmfDomain(shared_dynamic_cast<XdmfDomain>(xreader->read(xfile)).get());
    setParameterChoices(m_xattributes, m_attChoiceStr);
    m_xAttSelect = PseudoEnum(m_attChoiceStr);

    return true;
}

/**
  * Called if observeParameter changes.
  *
  * @param: changed parameter.
  *
  * @return: True if xdmf is in correct format else false.
  */
bool ReadSeisSol::examine(const vistle::Parameter *param)
{
    if (!param || param == m_file) {
        if (!checkEnd(m_file->getValue(), ".xdmf"))
            return false;
        std::function<bool(ReadSeisSol *)> xFunc = &ReadSeisSol::inspectXdmf;
        std::function<bool(ReadSeisSol *)> hFunc = &ReadSeisSol::hdfModeNotImplemented;
        return switchSeisMode(xFunc, hFunc, this);
    }
    return true;
}

/**
  * Fill typelist of unstructured grid vistle pointer with equal type.
  *
  * @unstr: UnstructuredGrid::ptr that contains type list.
  * @type: UnstructuredGrid::Type.
  */
void ReadSeisSol::fillUnstrGridTypeList(vistle::UnstructuredGrid::ptr unstr, const vistle::UnstructuredGrid::Type &type)
{
    std::fill(unstr->tl().begin(), unstr->tl().end(), type);
}

/**
  * Fill elementlist of unstructured grid vistle pointer.
  *
  * @unstr: UnstructuredGrid::ptr with elementlist to fill.
  * @numCornerPerElem: Number of Corners.
  */
template<class T>
void ReadSeisSol::fillUnstrGridElemList(vistle::UnstructuredGrid::ptr unstr, const T &numCornerPerElem)
{
    //4 corner = tetrahedron
    std::generate(unstr->el().begin(), unstr->el().end(),
                  [n = 0, &numCornerPerElem]() mutable { return n++ * numCornerPerElem; });
}

/**
  * Fill coordinates stored in a XdmfArray into unstructured grid vistle pointer.
  *
  * @unstr: UnstructuredGrid::ptr with coordinates to fill.
  * @xArrGeo: XdmfArray which contains coordinates in an one dimensional array.
  *
  * @return: .
  */
bool ReadSeisSol::fillUnstrGridCoords(vistle::UnstructuredGrid::ptr unstr, XdmfArray *xArrGeo)
{
    if(xArrGeo == nullptr)
        return false;

    auto x = unstr->x().data(), y = unstr->y().data(), z = unstr->z().data();

    //only 3D
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
  * Fill connectionlist stored in a XdmfArray into unstructured grid vistle pointer.
  *
  * @unstr: UnstructuredGrid::ptr with connectionlist to fill.
  * @xArrConn: XdmfArray which contains connectionlist in an one dimensional array.
  *
  * @return: TODO: add proper error-handling.
  */
bool ReadSeisSol::fillUnstrGridConnectList(vistle::UnstructuredGrid::ptr unstr, XdmfArray *xArrConn)
{
    if (xArrConn == nullptr)
        return false;

    //TODO: DEBUG invert elementvertices for tetrahedron => vtk vertice order = inverse vistle vertice order for (unsigned i = 0; i < xArrConn->getSize(); i++) { */
    auto cl = unstr->cl().data();
    xArrConn->getValues(0, cl, xArrConn->getSize());
    return true;
}

/**
  * Helper function for reading XdmfArray with given XdmfHeavyDataController.
  *
  * @xArr: XdmfArray for read operation.
  * @controller: XdmfHeavyDataController that specifies how to read the XdmfArray.
  */
void ReadSeisSol::readXdmfHeavyController(XdmfArray *xArr, const boost::shared_ptr<XdmfHeavyDataController> &controller)
{
    xArr->insert(controller);
    xArr->read();
}

/**
  * Generate an unstructured grid from XdmfUnstructuredGrid.
  *
  * @unstr: XdmfUnstructuredGrid pointer.
  *
  * @return: Created UnstructuredGrid pointer.
  */
vistle::UnstructuredGrid::ptr ReadSeisSol::generateUnstrGridFromXdmfGrid(XdmfUnstructuredGrid *unstr)
{
    //TODO: geometry always the same for each ugrid
    const auto &xtopology = unstr->getTopology();
    const auto &xgeometry = unstr->getGeometry();
    const int &numCornerPerElem = xtopology->getDimensions().at(1);

    //read xdmf connectionlist
    const shared_ptr<XdmfArray> xArrConn(XdmfArray::New());
    readXdmfHeavyController(xArrConn.get(), xtopology->getHeavyDataController());

    //read xdmf geometry
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
  * TODO: implement format trancision between XdmfArrayType and vistle array type.
  * Int8, Int16, Int32 = Integer
  * Float32 = float32
  * Float64 = double/Float
  * UInt8, UInt16, UInt32 = Unsigned
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
  * TODO: Implement trancision between XdmfAttributeCenter and vistle cell center.
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
  * Initialize xdmf varibales before read will be called.
  *
  * return: true if every xdmf variable could be initialized.
  */
bool ReadSeisSol::prepareReadXdmf()
{
    const std::string xfile = m_file->getValue();
    const auto xreader = XdmfReader::New();
    if (xreader.get() == nullptr)
        return false;
    const auto xdomain = shared_dynamic_cast<XdmfDomain>(xreader->read(xfile));
    xgridCollect = xdomain->getGridCollection(0);
    const auto &xugrid = xgridCollect->getUnstructuredGrid(0);
    const auto &xattribute = xugrid->getAttribute(m_xAttSelect[m_xattributes->getValue()]);
    if (xattribute->getReadMode() == XdmfArray::Reference) {
        sendInfo("Hyperslab is buggy in current Xdmf3 version. Chosen Param: %s", m_xattributes->getValue().c_str());
        return false;
    }
    return true;
}

/**
  * Call prepare function to chosen seismode.
  *
  * @ return: true if everything went well in called prepare function.
  */
bool ReadSeisSol::prepareRead()
{
    std::function<bool(ReadSeisSol *)> xFunc = &ReadSeisSol::prepareReadXdmf;
    std::function<bool(ReadSeisSol *)> hFunc = &ReadSeisSol::hdfModeNotImplemented;
    return switchSeisMode(xFunc, hFunc, this);
}

/**
  * Called by read(). Reads internal data links to hdf5 files stored in xdmf.
  * 
  * @token: Token for adding objects to ports.
  * @timestep: current timestep.
  * @block: current block.
  *
  * @return: true if everything has been added correctly to the ports.
  */
bool ReadSeisSol::readXdmf(Token &token, const int &timestep, const int &block)
{
    /** TODO:
      * - Distribute across MPI processes
      */

    /*************** Create Unstructured Grid **************/
    const auto &xugrid = xgridCollect->getUnstructuredGrid(timestep);
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

    /*************** Create Scalar **************/
    const auto &xattribute = xugrid->getAttribute(m_xAttSelect[m_xattributes->getValue()]);
    const auto attDim = xattribute->getDimensions().at(xattribute->getDimensions().size() - 1);

    //Debugging
    /* auto type = xattribute->getArrayType(); */
    /* setArrayType(type); */
    /* auto attCenter = xattribute->getCenter(); */
    /* setGridCenter(attCenter); */
    /* sendInfo("att name: %s", xattribute->getName().c_str()); */
    /* sendInfo("xattribute->dimension: %d", attDim); */
    //Debugging

    Vec<Scalar>::ptr att(new Vec<Scalar>(attDim));
    auto vattDataX = att->x().data();
    const shared_ptr<XdmfArray> xArrAtt(XdmfArray::New());
    readXdmfHeavyController(xArrAtt.get(), xattribute->getHeavyDataController());
    unsigned startRead{0};
    const unsigned arrStride{1};
    const unsigned valStride{1};
    if (m_xattributes->getValue() != "partition")
        startRead = attDim * timestep;

    xArrAtt->getValues<Scalar>(startRead, vattDataX, attDim, arrStride, valStride);

    /*************** Add data to ports & set meta data **************/
    ugrid_ptr->setBlock(block);
    ugrid_ptr->setTimestep(-1);
    ugrid_ptr->updateInternals();

    att->setGrid(ugrid_ptr);
    att->setBlock(block);
    att->setMapping(DataBase::Element);
    att->addAttribute("_species", "partition");
    att->setTimestep(-1);
    att->updateInternals();

    token.addObject(m_gridOut, ugrid_ptr);
    token.addObject("att", att);

    return true;
}

/**
  * Called every timestep and block.
  *
  * @token: Token for adding objects to ports.
  * @timestep: current timestep.
  * @block: current block.
  *
  * @return: true if everything has been added correctly to the ports.
  */
bool ReadSeisSol::read(Token &token, int timestep, int block)
{
    if (timestep == -1)
        return true;

    std::function<bool(ReadSeisSol *, Token &, const int &, const int &)> xFunc = &ReadSeisSol::readXdmf;
    std::function<bool(ReadSeisSol *, Token &, const int &, const int &)> hFunc =
        &ReadSeisSol::hdfModeNotImplementedRead;
    return switchSeisMode<ReadSeisSol *, Token &, const int &, const int &>(xFunc, hFunc, this, token, timestep, block);
}

/**
  * Delete xdmf element variables cached.
  *
  * @return: no proper error handling at the moment.
  */
bool ReadSeisSol::finishReadXdmf()
{
    if (xgridCollect != nullptr) {
        xgridCollect.reset();
    }
    return true;
}

/**
  * Called after final read call. Calls function according chosen seismode.
  *
  * @return: Return value from called function.
  */
bool ReadSeisSol::finishRead()
{
    std::function<bool(ReadSeisSol *)> xFunc = &ReadSeisSol::finishReadXdmf;
    std::function<bool(ReadSeisSol *)> hFunc = &ReadSeisSol::hdfModeNotImplemented;
    return switchSeisMode(xFunc, hFunc, this);
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

/**
  * Template for PseudoEnum which initializes map with an index like enum.
  *
  * @params: vector with values to store.
  */
template<class T>
void ReadSeisSol::PseudoEnum<T>::init(const std::vector<T> &params)
{
    std::for_each(params.begin(), params.end(), [n = 0, &indices = indices_map](auto &param) mutable {
        indices.insert({param, n++});
    });
}
