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

//xdmf3
#include <XdmfAttribute.hpp>
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
#include <XdmfHDF5WriterDSM.hpp>
#include <XdmfHDF5ControllerDSM.hpp>
#include <XdmfHDF5Controller.hpp>

//std
#include <boost/smart_ptr/shared_ptr.hpp>
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

ReadSeisSol::~ReadSeisSol()
{}

bool ReadSeisSol::examine(const vistle::Parameter *param)
{
    setTimesteps(-1);
    examineXdmf();
    return true;
}

bool ReadSeisSol::examineXdmf()
{
    //TODO: initialize geometry and topology and create a map for parameter choice which user want to visualize

    /* const std::string &xfile = getStringParameter("xfile"); */

    /* //read xdmf */
    /* const auto xreader = XdmfReader::New(); */
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
      * - Timesteps
      * - Distribute across MPI processes
      */

    const std::string xfile = getStringParameter("xfile");

    //read xdmf
    const auto xreader = XdmfReader::New();
    const auto xdomain = shared_dynamic_cast<XdmfDomain>(xreader->read(xfile));
    XdmfDomain xdomainDebug = *xdomain;
    const auto &xgridCollect = xdomain->getGridCollection(0);

    //geometry and topology is the same for all grid
    const auto &xugrid = xgridCollect->getUnstructuredGrid(2);

    const auto &xtopology = xugrid->getTopology();
    const auto &xgeometry = xugrid->getGeometry();
    /* const auto &time = xugrid->getTime(); */

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
    const auto &xattribute = xugrid->getAttribute(1);
    sendInfo("num sets: %d", xugrid->getNumberSets());
    XdmfArray *xArrU = xattribute.get();
    sendInfo("init ArrU: %d", xArrU->isInitialized());


    /* unsigned int numCells =->GetNumberOfCells(); */

    /* std::vector<unsigned int> dims = xattribute->getDimensions(); */
    /* unsigned int ndims = static_cast<unsigned int>(dims.size()); */
    /* unsigned int nvals = 1; */
    /* for (unsigned int i = 0; i < dims.size(); i++) { */
    /*     nvals = nvals * dims[i]; */
    /* } */

    /* unsigned int ncomp = 1; */

    /* shared_ptr<const XdmfAttributeCenter> attrCenter = xattribute->getCenter(); */
    /* if (attrCenter == XdmfAttributeCenter::Grid()) { */
    /*     ncomp = dims[ndims - 1]; */
    /* } else if (attrCenter == XdmfAttributeCenter::Cell()) { */
    /*     ncomp = nvals / numCells; */
    /* } else if (attrCenter == XdmfAttributeCenter::Node()) { */
    /*     fieldData = dataSet->GetPointData(); */
    /*     ncomp = nvals / numPoints; */
    /* } else { */
    /*     cerr << "skipping " << attrName << " unrecognized association" << endl; */
    /*     continue; // unhandled. */
    /* } */
    /* vtkDataSetAttributes *fdAsDSA = vtkDataSetAttributes::SafeDownCast(fieldData); */

    /* shared_ptr<const XdmfAttributeType> attrType = xmfAttribute->getType(); */
    /* enum vAttType { NONE, SCALAR, VECTOR, TENSOR, MATRIX, TENSOR6, GLOBALID }; */
    /* int atype = NONE; */
    /* if (attrType == XdmfAttributeType::Scalar() && ncomp == 1) { */
    /*     atype = SCALAR; */
    /* } else if (attrType == XdmfAttributeType::Vector() && ncomp == 3) { */
    /*     atype = VECTOR; */
    /* } else if (attrType == XdmfAttributeType::Tensor() && ncomp == 9) { */
    /*     atype = TENSOR; */
    /* } else if (attrType == XdmfAttributeType::Matrix()) { */
    /*     atype = MATRIX; */
    /* } else if (attrType == XdmfAttributeType::Tensor6()) // && ncomp == 6) */
    /* { */
    /*     atype = TENSOR6; */
    /* } else if (attrType == XdmfAttributeType::GlobalId() && ncomp == 1) { */
    /*     atype = GLOBALID; */
    /* } */


    // std::vector<unsigned int> dims = xArrU->getDimensions();
    // unsigned int ndims = static_cast<unsigned int>(dims.size());
    // unsigned int ncomp = 1;
    // if (ndims > 1)
    //     ncomp = dims[ndims - 1];

    // unsigned int ntuples = xArrU->getSize() / ncomp;
    // sendInfo("size: %d", xArrU->getSize());
    // sendInfo("ndims: %d", ndims);
    // sendInfo("ncomp: %d", ncomp);
    // sendInfo("tuples: %d", ntuples);

    // sendInfo("ugrids: %d", xdomain->getNumberUnstructuredGrids());
    // sendInfo("ugrid atts: %d", xugrid->getNumberAttributes());
    // sendInfo("ugrid maps: %d", xugrid->getNumberMaps());
    // sendInfo("ugrid set: %d", xugrid->getNumberSets());
    // sendInfo("ugridCollect set: %d", xgridCollect->getNumberSets());

    /* auto internal = xArrU->getValuesInternal(); */
    xArrU->read();

    /* std::vector<unsigned> start{1, 0}; */
    /* std::vector<unsigned> stride{1, 1}; */
    /* std::vector<unsigned> count{1, xattribute->getSize()}; */
    /* std::vector<unsigned> dataSize{1, xattribute->getSize()}; */
    /* const auto controller = XdmfHDF5Controller::New( */
    /*     "/data/ChEESE/SeisSol/LMU_Sulawesi_example/Sulawesi-surface_cell.h5", "Sulawesi-surface_cell.h5:/mesh0/u", */
    /*     XdmfArrayType::Float64(), start, stride, count, dataSize); */
    /* xattribute->insert(controller); */
    /* xattribute->read(); */
    /* sendInfo("ItemType: %s", xattribute->getItemType().c_str()); */
    /* sendInfo("ItemTag: %s", xattribute->getItemTag().c_str()); */
    /* std::map<std::string, std::string> attTypeProp; */
    /* xattribute->getType()->getProperties(attTypeProp); */
    /* sendInfo("AttTypeProp:"); */
    /* for (auto [name, val]: attTypeProp) */
    /*     sendInfo("%s:%s", name.c_str(), val.c_str()); */

    /* sendInfo("Info:"); */
    /* for (int i = 0; i < xattribute->getNumberInformations(); i++) */
    /*     for (auto [name, val]: xattribute->getInformation(i)->getItemProperties()) */
    /*         sendInfo("%s:%s", name.c_str(), val.c_str()); */

    /* sendInfo("att prop:"); */
    /* for (auto [name, val]: xattribute->getItemProperties()) */
    /*     sendInfo("%s:%s", name.c_str(), val.c_str()); */

    /* sendInfo("size att: %d", xattribute->getSize()); */
    Vec<Scalar>::ptr att(new Vec<Scalar>(xattribute->getSize()));
    const auto arrSize = xattribute->getSize();
    for (int i = 0; i < 10; i++)
        /* sendInfo("%d: %f", i, xattribute->getValue<float>(i)); */
        sendInfo("%d: %f", i, xArrU->getValue<float>(i));

    auto ux = att->x().data();
    xattribute->getValues(0, ux, arrSize);

    for (int i = 0; i < 10; i++)
        sendInfo("%d: %f", i, ux[i]);

    /* xattribute->getValues(1, att->y().data(), arrSize, 4, 1); */
    /* xattribute->getValues(2, att->z().data(), arrSize, 4, 1); */
    /* xattribute->getValues(3, att->w().data(), arrSize, 4, 1); */

    //visualize att without hyperslap
    /* const shared_ptr<XdmfArray> xArrU(XdmfArray::New()); */

    /* xArrU->insert(xattribute->getHeavyDataController()); */
    /* xArrU->read(); */

    /* Vec<Scalar>::ptr att(new Vec<Scalar>(xArrU->getSize())); */
    /* const auto arrUSize = xArrU->getSize(); */

    /* for (int i = arrUSize; i < arrUSize + 10; i++) */
    /*     sendInfo("%d", xArrU->getValue<int>(i)); */

    /* auto ux = att->x().data(); */
    /* xArrU->getValues(0, ux, arrUSize); */

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
