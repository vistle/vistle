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

//mpi
#include <mpi.h>

//vistle
#include "vistle/core/object.h"
#include "vistle/core/port.h"
#include "vistle/core/database.h"
#include "vistle/core/parameter.h"
#include "vistle/core/scalar.h"
#include "vistle/core/unstr.h"
#include "vistle/core/vec.h"
#include "vistle/module/module.h"
#include "vistle/module/reader.h"
#include "vistle/util/enum.h"

//boost
#include <boost/mpi/communicator.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>

//xdmf3
#include <XdmfAttribute.hpp>
#include <XdmfTopology.hpp>
#include <XdmfAttributeCenter.hpp>
#include <XdmfAttributeType.hpp>
#include <XdmfTime.hpp>
#include <XdmfGeometry.hpp>
#include <XdmfGridCollection.hpp>
#include <XdmfUnstructuredGrid.hpp>
#include <XdmfDomain.hpp>
#include <XdmfReader.hpp>
#include <XdmfArray.hpp>
#include <XdmfArrayType.hpp>
#include <XdmfHDF5WriterDSM.hpp>
#include <XdmfHDF5ControllerDSM.hpp>
#include <XdmfHDF5Controller.hpp>

//std
#include <array>
#include <functional>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
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
  * @brief: Extracts XdmfArrayType from ugrid and returns corresponding vistle UnstructuredGrid::Type.
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
 * @brief: Checks if suffix string is the end of main.
 *
 * @param main: main string.
 * @param suffix: suffix string
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

/**
 * @brief: Call check function. Sends the msgFailure string if check() returns false.
 *
 * @param obj: ReadSeisSol object pointer.
 * @param vObj_ptr: Object to check.
 * @param msgFailure: Error message.
 *
 * @return: obj->check() result.
 */
bool checkObjectPtr(ReadSeisSol *obj, vistle::Object::ptr vObj_ptr, const std::string &msgFailure)
{
    if (!vObj_ptr->check()) {
        obj->sendInfo("%s", msgFailure.c_str());
        return false;
    }
    return true;
}
} // namespace

/**
  * @brief: Constructor.
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
    m_blocks[0] = addIntParameter("x", "Number of blocks in x direction", 1);
    m_blocks[1] = addIntParameter("y", "Number of blocks in y direction", 1);
    m_blocks[2] = addIntParameter("z", "Number of blocks in z direction", 1);

    setParameterRange(m_blocks[0], Integer(1), Integer(999999));
    setParameterRange(m_blocks[1], Integer(1), Integer(999999));
    setParameterRange(m_blocks[2], Integer(1), Integer(999999));
    //TODO: Implement GhostCellGenerator
    /* m_ghost = addIntParameter("ghost", "Ghost layer", 1, Parameter::Boolean); */

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
 * @brief Destructor.
 */
ReadSeisSol::~ReadSeisSol()
{
    releaseXdmfObjects();
}

/**
 * @brief Clear up allocated xdmf objects by reader.
 */
void ReadSeisSol::releaseXdmfObjects()
{
    if (xgridCollect != nullptr) {
        xgridCollect->release();
        xgridCollect.reset();
    }
}

/**
  * @brief: Template for switching between XDMF and HDF5 mode. Calls given function with args according to current mode.
  *
  * @xdmfFunc: Function that will be called when in XDMF mode.
  * @hdfFunc: Function that will be called when in HDF5 mode.
  *
  * return: seisFunc(args) -> return type.
  */
template<class Ret, class... Args>
auto ReadSeisSol::switchSeisMode(std::function<Ret(Args...)> xdmfFunc, std::function<Ret(Args...)> hdfFunc,
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

//TODO: Template export to core?
/**
 * @brief Template for block partition.
 *
 * @tparam InputBlockIt Iteratortype begin of container which holds values for partition.
 * @tparam OutputBlockIt Iteratortype end of container which hodls values for partition.
 * @tparam NumericType Block num type.
 * @param first begin of container which holds values for partition.
 * @param last end of container which hodls values for partition.
 * @param d_first container to fill.
 * @param blockNum current blockNum.
 *
 * @return iterator to restult container.
 */
template<class InputBlockIt, class OutputBlockIt, class NumericType>
OutputBlockIt ReadSeisSol::blockPartition(InputBlockIt first, InputBlockIt last, OutputBlockIt d_first,
                                          const NumericType &blockNum)
{
    const auto numBlocks{std::distance(first, last)};
    std::transform(first, last, d_first, [it = 0, &numBlocks, b = blockNum](const NumericType &bS) mutable {
        if (it++ == numBlocks - 1)
            return b;
        else {
            NumericType bDist = b % bS;
            b /= bS;
            return bDist;
        };
    });
    return d_first;
}

/**
  * @brief: Wrapper function for easier call of seismode-function for selected seismode.
  *
  * @xdmfFunc: Function pointer to XdfmFunction.
  * @hdfFunc: Function pointer to hdfFunction.
  * @args: Arguments to pass.
  *
  * @return: Return val of called function.
  */
template<class Ret, class... Args>
auto ReadSeisSol::callSeisModeFunction(Ret (ReadSeisSol::*xdmfFunc)(Args...), Ret (ReadSeisSol::*hdfFunc)(Args...),
                                       Args... args)
{
    using funcType = Ret(ReadSeisSol *, Args...);
    std::function<funcType> xFunc{xdmfFunc};
    std::function<funcType> hFunc{hdfFunc};
    return switchSeisMode<Ret, ReadSeisSol *, Args...>(xFunc, hFunc, this, args...);
}

/**
  * @brief: Helper function to tell user that HDF5 mode is not implemented.
  *
  * @return: false.
  */
bool ReadSeisSol::hdfModeNotImplemented()
{
    sendInfo("HDF5 mode is not implemented.");
    return false;
}
bool ReadSeisSol::hdfModeNotImplementedRead(Token &, int, int)
{
    return hdfModeNotImplemented();
}

/**
  * @brief: Clears attributes choice.
  */
void ReadSeisSol::clearChoice()
{
    m_xattributes->setChoices(std::vector<std::string>());
    m_attChoiceStr.clear();
}

/**
  * @brief: Iterate through properties of a xdmfgridcollection.
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
                //number of unstructured grids = timestep
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
  * @brief: Iterate through unstructured grid attributes stored in an xdmf.
  *
  * @xugrid: const unstructured grid pointer.
  */
void ReadSeisSol::inspectXdmfUnstrGrid(const XdmfUnstructuredGrid *xugrid)
{
    if (xugrid != nullptr) {
        //TODO: NOT IMPLEMENTED YET => DO NOTHING AT THE MOMENT.
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
  * @brief: Inspect XdmfAttribute and generate a parameter in reader.
  *
  * @xatt: Const attribute pointer.
  */
void ReadSeisSol::inspectXdmfAttribute(const XdmfAttribute *xatt)
{
    if (xatt != nullptr)
        m_attChoiceStr.push_back(xatt->getName());
}

/**
  * @brief: Inspect XdmfDomain.
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
  * @brief: Inspect Xdmf.
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
    m_xAttSelect = DynamicPseudoEnum(m_attChoiceStr);

    return true;
}


/**
 * @brief Checks that total product of blocks are equal mpi size.
 *
 * @return result of "product blocks == mpisize".
 */
bool ReadSeisSol::checkBlocks()
{
    const int &nBlocks = m_blocks[0]->getValue() * m_blocks[1]->getValue() * m_blocks[2]->getValue();
    setPartitions(nBlocks);
    if (nBlocks == size())
        return true;
    else {
        sendInfo("Number of blocks should equal MPISIZE.");
        return false;
    }
}

/**
  * @brief: Called if observeParameter changes.
  *
  * @param: changed parameter.
  *
  * @return: True if xdmf is in correct format else false.
  */
bool ReadSeisSol::examine(const vistle::Parameter *param)
{
    static bool check = false;
    if (!param || param == m_file) {
        if (!checkEnd(m_file->getValue(), ".xdmf"))
            return false;

        check = callSeisModeFunction(&ReadSeisSol::inspectXdmf, &ReadSeisSol::hdfModeNotImplemented);
    }
    return checkBlocks();
}

/**
  * @brief: Fill typelist of unstructured grid vistle pointer with equal type.
  *
  * @unstr: UnstructuredGrid::ptr that contains type list.
  * @type: UnstructuredGrid::Type.
  */
void ReadSeisSol::fillUnstrGridTypeList(vistle::UnstructuredGrid::ptr unstr, const vistle::UnstructuredGrid::Type &type)
{
    std::fill(unstr->tl().begin(), unstr->tl().end(), type);
}

/**
  * @brief: Fill elementlist of unstructured grid vistle pointer.
  *
  * @unstr: UnstructuredGrid::ptr with elementlist to fill.
  * @numCornerPerElem: Number of Corners.
  */
template<class T>
void ReadSeisSol::fillUnstrGridElemList(vistle::UnstructuredGrid::ptr unstr, const T &numCornerPerElem)
{
    //e.g. 4 corner = tetrahedron
    std::generate(unstr->el().begin(), unstr->el().end(),
                  [n = 0, &numCornerPerElem]() mutable { return n++ * numCornerPerElem; });
}

/**
  * @brief: Fill coordinates stored in a XdmfArray into unstructured grid vistle pointer.
  *
  * @unstr: UnstructuredGrid::ptr with coordinates to fill.
  * @xArrGeo: XdmfArray which contains coordinates in an one dimensional array.
  *
  * @return: true if values stored in XdmfArray could be stored in coords array of given unstructured grid.
  */
bool ReadSeisSol::fillUnstrGridCoords(vistle::UnstructuredGrid::ptr unstr, XdmfArray *xArrGeo)
{
    if (xArrGeo == nullptr)
        return false;

    auto x = unstr->x().data(), y = unstr->y().data(), z = unstr->z().data();

    //TODO: not the same for all xdmf => hyperslap not working => maybe using XML parser and reading hyperslap to concretize read.
    // -> only 3D at the moment
    constexpr auto numCoords{3};
    constexpr auto strideVistleArr{1};

    //current order for cheeseSeisSol-files: arrGeo => x1 y1 z1 x2 y2 z2 x3 y3 z3 ... xn yn zn
    xArrGeo->getValues(0, x, xArrGeo->getSize() / numCoords, numCoords, strideVistleArr);
    xArrGeo->getValues(1, y, xArrGeo->getSize() / numCoords, numCoords, strideVistleArr);
    xArrGeo->getValues(2, z, xArrGeo->getSize() / numCoords, numCoords, strideVistleArr);

    return true;
}

/**
  * @brief: Fill connectionlist stored in a XdmfArray into unstructured grid vistle pointer.
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

    //TODO: Debug invert elementvertices for tetrahedron => vtk vertice order = inverse vistle vertice order for (unsigned i = 0; i < xArrConn->getSize(); i++) { */
    auto cl = unstr->cl().data();
    xArrConn->getValues(0, cl, xArrConn->getSize());
    return true;
}

/**
  * @brief: Helper function for reading XdmfArray with given XdmfHeavyDataController.
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
 * @brief: Generate a scalar pointer with values from XdmfAttribute.
 *
 * @param xattribute: XdmfAttribute pointer.
 * @param timestep: Current timestep.
 * @param block: Current block number.
 *
 * @return: generated vistle scalar pointer.
 */
vistle::Vec<Scalar>::ptr ReadSeisSol::generateScalarFromXdmfAttribute(XdmfAttribute *xattribute, const int &timestep,
                                                                      const int &block)
{
    if (xattribute == nullptr)
        return nullptr;

    const auto &numDims = xattribute->getDimensions().size();

    //last dim relevant
    const auto attDim = xattribute->getDimensions().at(numDims - 1);

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
    if (numDims > 1)
        startRead = attDim * timestep;

    xArrAtt->getValues<Scalar>(startRead, vattDataX, attDim, arrStride, valStride);
    return att;
}

/**
  * @brief: Generate an unstructured grid from XdmfUnstructuredGrid.
  *
  * @param unstr: XdmfUnstructuredGrid pointer.
  * @param timestep: Current timestep.
  * @param block: Current block number.
  *
  * @return: Created UnstructuredGrid pointer.
  */
vistle::UnstructuredGrid::ptr ReadSeisSol::generateUnstrGridFromXdmfGrid(XdmfUnstructuredGrid *unstr, const int block)
{
    if (unstr == nullptr)
        return nullptr;

    //TODO: geometry always the same for each ugrid for seissol
    const auto &xtopology = unstr->getTopology();
    const auto &xgeometry = unstr->getGeometry();
    const int &numCornerPerElem = xtopology->getDimensions().at(xtopology->getDimensions().size() - 1);

    const shared_ptr<XdmfArray> xArrConn(XdmfArray::New());
    const shared_ptr<XdmfArray> xArrGeo(XdmfArray::New());
    const auto &geoContr = xgeometry->getHeavyDataController();
    const auto &topoContr = xtopology->getHeavyDataController();
    if (numPartitions() == 1) {
        //read xdmf connectionlist
        readXdmfHeavyController(xArrConn.get(), topoContr);

        //read xdmf geometry
        readXdmfHeavyController(xArrGeo.get(), geoContr);
    } else {
    }

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
  * @brief: TODO: implement format trancision between XdmfArrayType and vistle array type.
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
  * => relevant for color module not this reader.
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
  * @brief: Initialize xdmf varibales before read will be called.
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
    const auto &xugrid = xgridCollect->getUnstructuredGrid(1);
    const auto &xattribute = xugrid->getAttribute(m_xAttSelect[m_xattributes->getValue()]);
    if (xattribute->getReadMode() == XdmfArray::Reference) {
        /* auto arrayTest = XdmfArray::New(); */
        /* arrayTest->setReference(xattribute->getReference()); */
        /* xattribute->setReadMode(XdmfArray::Reference); */
        /* arrayTest->insert(xattribute->getHeavyDataController()); */
        /* arrayTest->read(); */

        /* arrayTest->setReference(xattribute->getReference()); */
        /* arrayTest->insert(xattribute->getHeavyDataController()); */
        /* arrayTest->read(); */
        /* xattribute->readReference(); */
        /* xattribute->read(); */
        /* sendInfo("reference read"); */
        /* std::vector<float> test(xattribute->getSize()); */
        /* arrayTest->getValues(0, test.data()); */
        /* for (auto i : test) */
        /*     std::cout << i << '\n'; */

        sendInfo("Hyperslab is buggy in current Xdmf3 version or you aren't using HDF5 files. Chosen Param: %s",
                 m_xattributes->getValue().c_str());
        return false;
    }
    return true;
}

/**
  * @brief: Call prepare function to chosen seismode.
  *
  * @return: true if everything went well in called prepare function.
  */
bool ReadSeisSol::prepareRead()
{
    return callSeisModeFunction(&ReadSeisSol::prepareReadXdmf, &ReadSeisSol::hdfModeNotImplemented);
}

/**
  * @brief: Called by read(). Reads internal data links to hdf5 files stored in xdmf.
  *
  * @token: Token for adding objects to ports.
  * @timestep: current timestep.
  * @block: current block.
  *
  * @return: true if everything has been added correctly to the ports.
  */
bool ReadSeisSol::readXdmf(Token &token, int timestep, int block)
{
    /** TODO:
      * - Distribute across MPI processes
      */

    /*************** Create Unstructured Grid **************/
    const auto &xugrid = xgridCollect->getUnstructuredGrid(timestep);
    auto ugrid_ptr = generateUnstrGridFromXdmfGrid(xugrid.get(), block);
    checkObjectPtr(this, ugrid_ptr,
                   "Something went wrong while creation of ugrid! Possible errors:\n"
                   "-invalid connectivity list \n"
                   "-invalid coordinates\n"
                   "-invalid typelist\n"
                   "-invalid geo type");

    /*************** Create Scalar **************/
    const auto &xattribute = xugrid->getAttribute(m_xAttSelect[m_xattributes->getValue()]);
    auto att_ptr = generateScalarFromXdmfAttribute(xattribute.get(), timestep, block);
    checkObjectPtr(this, att_ptr,
                   "Something went wrong while creation of attribute pointer! Possible errors:\n"
                   "-wrong format (hyperslap not supported at the moment) \n"
                   "-empty attribute\n"
                   "-wrong dimensions defined");

    /*************** Add data to ports & set meta data **************/
    ugrid_ptr->setBlock(block);
    ugrid_ptr->setTimestep(timestep);
    ugrid_ptr->updateInternals();

    att_ptr->setGrid(ugrid_ptr);
    att_ptr->setBlock(block);
    att_ptr->setMapping(DataBase::Element);
    att_ptr->addAttribute("_species", xattribute->getName());
    att_ptr->setTimestep(timestep);
    att_ptr->updateInternals();

    token.addObject(m_gridOut, ugrid_ptr);
    token.addObject(m_scalarOut, att_ptr);

    return true;
}

/**
  * @brief: Called every timestep and block.
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

    return callSeisModeFunction<bool, Token &, int, int>(
        &ReadSeisSol::readXdmf, &ReadSeisSol::hdfModeNotImplementedRead, token, timestep, block);
}

/**
  * @brief: Delete xdmf element variables cached.
  *
  * @return: no proper error handling at the moment.
  */
bool ReadSeisSol::finishReadXdmf()
{
    releaseXdmfObjects();
    return true;
}

/**
  * @brief: Called after final read call. Calls function according chosen seismode.
  *
  * @return: Return value from called function.
  */
bool ReadSeisSol::finishRead()
{
    return callSeisModeFunction(&ReadSeisSol::finishReadXdmf, &ReadSeisSol::hdfModeNotImplemented);
}

bool ReadSeisSol::readXdmfParallel(XdmfArray *array, const shared_ptr<XdmfHeavyDataController> &defaultController,
                                   const int block)
{
    constexpr auto numBlocks = 3;
    const auto &dimVec = defaultController->getDimensions();
    std::array<unsigned, numBlocks> blocks;
    std::for_each(m_blocks.begin(), m_blocks.end(),
                  [n = 0, &blocks](IntParameter *vistleInt) mutable { blocks[n++] = vistleInt->getValue(); });

    std::array<unsigned, numBlocks> partition;
    blockPartition(blocks.begin(), blocks.end(), partition.begin(), block);

    std::vector<unsigned> readStart(numBlocks);
    std::generate(readStart.begin(), readStart.end(), [n = 0, &partition, &dimVec]() mutable {
        auto i = n++;
        return partition[i] * dimVec[i] / numBlocks;
    });

    sendInfo("%s", defaultController->getDescriptor().c_str());

    return false;

    auto hdfController = XdmfHDF5Controller::New(
        defaultController->getFilePath(), defaultController->getName(), defaultController->getType(), readStart,
        defaultController->getStride(), dimVec, defaultController->getDataspaceDimensions());

    array->insert(hdfController);
    array->read();

    return true;
}

/**
  * @brief: Template for PseudoEnum which initializes map with an index like enum.
  *
  * @params: vector with values to store.
  */
template<class T>
void ReadSeisSol::DynamicPseudoEnum<T>::init(const std::vector<T> &params)
{
    std::for_each(params.begin(), params.end(), [n = 0, &indices = indices_map](auto &param) mutable {
        indices.insert({param, n++});
    });
}
