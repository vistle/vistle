#include "Sample.h"
#include <core/object.h>

using namespace vistle;

MODULE_MAIN(Sample)

DEFINE_ENUM_WITH_STRING_CONVERSIONS(InterpolationMode,
      (First) // value of first vertex
      (Mean) // mean value of all vertices
      (Nearest) // value of nearest vertex
      (Linear) // barycentric/multilinear interpolation
      )

Sample::Sample(const std::string &name, int moduleID, mpi::communicator comm)
   : Module("MapDrape", name, moduleID, comm) {

    setSchedulingPolicy(message::SchedulingPolicy::Single);
    setReducePolicy(message::ReducePolicy::PerTimestepOrdered);

    createInputPort("data_in","object with data to be sampled", Port::Flags::NOCOMPUTE);
    createInputPort("ref_in","target grid");

    m_out = createOutputPort("data_out");

    m_mode = addIntParameter("mode", "interpolation mode", Linear, Parameter::Choice);
    m_createCelltree = addIntParameter("create_celltree", "create celltree", 0, Parameter::Boolean);

    V_ENUM_SET_CHOICES(m_mode, InterpolationMode);
}

Sample::~Sample() {
}

int Sample::SampleToStrG(StructuredGrid::const_ptr strG, DataBase::const_ptr inData, DataBase::ptr &sampled) {
    int found = 0;
    auto inObj =inData->grid();
    const GridInterface *inGrid = inObj->getInterface<GridInterface>();
    if (!inGrid) {
        std::cerr << "Failed to pass grid" << std::endl;
        return found;
    }
    Vec<Scalar>::const_ptr scal = Vec<Scalar>::as(inData);
    if (!scal) {
        std::cerr << "no scalar data received"<< std::endl;
        return found;
    }

    const Scalar *data = &scal->x()[0];

    Index numVert = strG->getNumVertices();
    Vec<Scalar>::ptr dataOut(new Vec<Scalar>(numVert));
    Scalar *ptrOnData = dataOut->x().data();

    for (unsigned int i=0; i < numVert; ++i) {
        Vector v = strG->getVertex(i);
        Index cellIdxIn = inGrid->findCell(v,InvalidIndex,useCelltree?GridInterface::NoFlags:GridInterface::NoCelltree);
        if (cellIdxIn != InvalidIndex) {
            GridInterface::Interpolator interp = inGrid->getInterpolator(cellIdxIn, v, DataBase::Vertex, mode);
            Scalar p = interp(data);
            ptrOnData[i] = p;
            found = 1;
        }else {
            ptrOnData[i] = NO_VALUE;
        }
    }

    sampled = DataBase::as(Object::ptr(dataOut));

    return found;
}

int Sample::SampleToUniG(UniformGrid::const_ptr uni, DataBase::const_ptr inData, DataBase::ptr &sampled) {
    int found =0;
    auto inObj =inData->grid();
    const GridInterface *inGrid = inObj->getInterface<GridInterface>();
    if (!inGrid) {
        std::cerr << "Failed to pass grid" << std::endl;
        return found;
    }
    Vec<Scalar>::const_ptr scal = Vec<Scalar>::as(inData);
    if (!scal) {
        std::cerr << "no scalar data received"<< std::endl;
        return found;
    }

    const Scalar *data = &scal->x()[0];

    Index numVer[3] = {uni->getNumDivisions(0),uni->getNumDivisions(1),uni->getNumDivisions(2)};
    Index numCoords = numVer[0]*numVer[1]*numVer[2];

    std::vector<float> xR(numCoords);
    std::vector<float> yR(numCoords);
    std::vector<float> zR(numCoords);

    Vector3 min = Vector3(uni->min()[0],uni->min()[1],uni->min()[2]);
    Vector3 max = Vector3(uni->max()[0],uni->max()[1],uni->max()[2]);
    Vector3 d = Vector3((max[0] - min[0]) / (numVer[0]-1),(max[1] - min[1]) / (numVer[1]-1),(max[2] - min[2]) / (numVer[2]-1));
    for (unsigned int i = 0; i < numVer[0]; ++i) {
        for (unsigned int j = 0; j < numVer[1]; ++j) {
            for (unsigned int k = 0; k < numVer[2]; ++k) {
                const Index idx = UniformGrid::vertexIndex(i, j, k, numVer);

                xR[idx] =  min[0] + i * d[0];
                yR[idx] =  min[1] + j * d[1];
                zR[idx] =  min[2] + k * d[2];
            }
        }
    }

    Vec<Scalar>::ptr dataOut(new Vec<Scalar>(numCoords));
    Scalar *ptrOnData = dataOut->x().data();
    for (unsigned int i=0; i < numCoords; ++i) {
        Vector v(xR[i], yR[i], zR[i]);
        Index cellIdxIn = inGrid->findCell(v,InvalidIndex,useCelltree?GridInterface::NoFlags:GridInterface::NoCelltree);
        if (cellIdxIn != InvalidIndex) {
            GridInterface::Interpolator interp = inGrid->getInterpolator(cellIdxIn, v, DataBase::Vertex, mode);
            Scalar p = interp(data);
            ptrOnData[i] = p;
            found = 1;
        }else {
            ptrOnData[i] = NO_VALUE ;
        }
    }
    sampled = DataBase::as(Object::ptr(dataOut));

    return found;
}


int Sample::SampleToUstG(UnstructuredGrid::const_ptr ust, DataBase::const_ptr inData, DataBase::ptr &sampled) {
    int found =0;
    auto inObj =inData->grid();
    const GridInterface *inGrid = inObj->getInterface<GridInterface>();
    if (!inGrid) {
        std::cerr << "Failed to pass grid" << std::endl;
        return found;
    }
    Vec<Scalar>::const_ptr scal = Vec<Scalar>::as(inData);
    if (!scal) {
        std::cerr << "no scalar data received"<< std::endl;
        return found;
    }

    const Scalar *data = &scal->x()[0];

    Index numVert = ust->getNumVertices();
    Vec<Scalar>::ptr dataOut(new Vec<Scalar>(numVert));
    Scalar *ptrOnData = dataOut->x().data();

    for (unsigned int i=0; i < numVert; ++i) {
        Vector v = ust->getVertex(i);
        Index cellIdxIn = inGrid->findCell(v,InvalidIndex,useCelltree?GridInterface::NoFlags:GridInterface::NoCelltree);
        if (cellIdxIn != InvalidIndex) {
            GridInterface::Interpolator interp = inGrid->getInterpolator(cellIdxIn, v, DataBase::Vertex, mode);
            Scalar p = interp(data);
            ptrOnData[i] = p;
            found = 1;
        }else {
            ptrOnData[i] = NO_VALUE;
        }
    }
    sampled = DataBase::as(Object::ptr(dataOut));

    return found;
}


int Sample::SampleToRectG(RectilinearGrid::const_ptr rec, DataBase::const_ptr inData, DataBase::ptr &sampled ) {
    int found = 0;
    auto inObj =inData->grid();
    const GridInterface *inGrid = inObj->getInterface<GridInterface>();
    if (!inGrid) {
        std::cerr << "Failed to pass grid" << std::endl;
        return found;
    }
    Vec<Scalar>::const_ptr scal = Vec<Scalar>::as(inData);
    if (!scal) {
        std::cerr << "no scalar data received"<< std::endl;
        return found;
    }

    const Scalar *data = &scal->x()[0];

    unsigned int numCoords = 0;
    Index numDiv[3] = {rec->getNumDivisions(0),rec->getNumDivisions(1),rec->getNumDivisions(2)};
    numCoords = numDiv[0]*numDiv[1]*numDiv[2];
    std::vector<float> xR(numCoords);
    std::vector<float> yR(numCoords);
    std::vector<float> zR(numCoords);
    for (unsigned int i = 0; i < numDiv[0]; ++i) {
        for (unsigned int j = 0; j < numDiv[1]; ++j) {
            for (unsigned int k = 0; k < numDiv[2]; ++k) {
                const Index idx = UniformGrid::vertexIndex(i, j, k, numDiv);

                xR[idx] =  rec->coords(0)[i];
                yR[idx] =  rec->coords(1)[j];
                zR[idx] =  rec->coords(2)[k];
            }
        }
    }
    Vec<Scalar>::ptr dataOut(new Vec<Scalar>(numCoords));
    Scalar *ptrOnData = dataOut->x().data();

    for (unsigned int i=0; i < numCoords; ++i) {
        Vector v(xR[i], yR[i], zR[i]);
        Index cellIdxIn = inGrid->findCell(v,InvalidIndex,useCelltree?GridInterface::NoFlags:GridInterface::NoCelltree);
        if (cellIdxIn != InvalidIndex) {
            GridInterface::Interpolator interp = inGrid->getInterpolator(cellIdxIn, v, DataBase::Vertex, mode);
            Scalar p = interp(data);
            ptrOnData[i] = p;
            found = 1;
        }else {
            ptrOnData[i] = NO_VALUE;
        }
    }
    sampled = DataBase::as(Object::ptr(dataOut));

    return found;
}

bool Sample::reduce(int timestep) {
    int nProcs = comm().size();
    int numLocalObjs = 0;
    numLocalObjs = objListLocal.size();
    std::vector<int> objPerRank;

    mpi::all_gather(comm(), numLocalObjs, objPerRank);

    int maxTimesteps = -1;
    int numDataTimesteps = 1;
    int numLocalDT =  -1;
    if (!dataList.empty()) {
        numLocalDT = dataList.at(0)->getNumTimesteps();
    }

    maxTimesteps = mpi::all_reduce(comm(),numLocalDT, boost::mpi::maximum<int>());

    if (maxTimesteps > 0)
        numDataTimesteps = maxTimesteps;

    for (int r=0; r<nProcs; ++r) {
        if (objPerRank[r]<1)
             continue;
        for (int n=0; n<objPerRank[r];++n) {

            Object::const_ptr objAtIdx;
            if (r == rank()) {
                objAtIdx = objListLocal.at(n);
            }
            broadcastObject(objAtIdx, r);

            const GridInterface *refGrid = objAtIdx->getInterface<GridInterface>();

            for (int dT = 0; dT < numDataTimesteps; ++dT) {
                if (maxTimesteps < 1)
                     dT = -1;

                DataBase::ptr sampledData;
                std::vector<int> foundList;
                std::vector<Object::const_ptr> sampledDataList;

                int found = 0;
                if (!dataList.empty()) {
                    DataBase::const_ptr data = dataList.at(0);

                    for (unsigned long dIdx = 0; dIdx < dataList.size(); ++dIdx) {
                        data = dataList.at(dIdx);
                        if (data->getTimestep()!=dT)
                            continue;
                        int foundSample = 0;
                        if (auto uni = UniformGrid::as(refGrid->object())) {
                            foundSample += SampleToUniG(uni, data, sampledData);
                        }else if (auto rec = RectilinearGrid::as(refGrid->object())) {
                            foundSample += SampleToRectG(rec, data, sampledData);
                        }else if (auto str = StructuredGrid::as(refGrid->object())) {
                            foundSample += SampleToStrG(str, data, sampledData);
                        }else if (auto ust = UnstructuredGrid::as(refGrid->object())) {
                            foundSample += SampleToUstG(ust, data, sampledData);
                        }else {
                            std::cerr << "Target grid type not supported" << std::endl;
                            return true;
                        }
                        if (foundSample > 0)
                            sampledDataList.push_back(sampledData);
                        found += foundSample;
                    }
                }

                mpi::gather(comm(),found,foundList,r);

                if (rank() == r) {
                    for (unsigned long fr=0; fr<foundList.size();++fr) {
                        int nFound = foundList.at(fr);
                        if ((nFound>0) && (fr!=r)) {
                            for (int fIdx = 0; fIdx < nFound; ++fIdx) {
                                auto sampledElem = receiveObject(comm(),fr);
                                sampledDataList.push_back(sampledElem);
                            }
                        }
                    }

                    Vec<Scalar>::ptr outData(new Vec<Scalar>(refGrid->getNumVertices()));
                    auto globDatVec = outData->x().data();

                    for (unsigned int bIdx = 0; bIdx < refGrid->getNumVertices() ; ++bIdx) {
                        globDatVec[bIdx] = NO_VALUE ;
                    }

                    if (!sampledDataList.empty()) {
                        for (auto elem : sampledDataList) {
                            auto locDat = Vec<Scalar,1>::as(elem);
                            auto locDatVec = &locDat->x()[0];

                            for (unsigned int bIdx = 0; bIdx <  locDat->getSize(); ++bIdx) {
                               if (locDatVec[bIdx] != NO_VALUE) {
                                    globDatVec[bIdx] = locDatVec[bIdx];
                               }
                            }
                        }
                    }
                    Object::const_ptr outGrid = objAtIdx;
                    outData->setTimestep(dT);
                    outData->updateInternals();
                    outData->setBlock(blockIdx.at(n));
                    outData->setNumTimesteps(maxTimesteps);
                    outData->setGrid(outGrid);
                    outData->setMapping(DataBase::Vertex);
                    outData->addAttribute("_species","scalar");
                    addObject(m_out, outData);
                }else {
                    if (found > 0) {
                        for (auto elem : sampledDataList) {
                            sendObject(comm(),elem,r);
                        }
                    }
                }
                sampledDataList.clear();
                foundList.clear();

                if (dT < 0) break;
            }
        }
     }

     comm().barrier();
     objPerRank.clear();

     if (objListLocal.empty() || (timestep == objListLocal.at(0)->getNumTimesteps() - 1) || (objListLocal.at(0)->getNumTimesteps() < 2)) {
         dataList.clear();
         blockIdx.clear();
         objListLocal.clear();
     }
     return true;
}

bool Sample::compute() {
    mode = (GridInterface::InterpolationMode)m_mode->getValue();

    if (m_createCelltree->getValue()) {
        auto unstr = UnstructuredGrid::as(objListLocal.back());
        auto str = StructuredGrid::as(objListLocal.back());

        if (str) {
            useCelltree = true;
            if (!str->hasCelltree()) {
                str->getCelltree();
                if (!str->validateCelltree()) {
                    useCelltree = false;
                }
            }
        }else if (unstr) {
            useCelltree = true;
            if (!unstr->hasCelltree()) {
                unstr->getCelltree();
                if (!unstr->validateCelltree()) {
                    useCelltree = false;
                }
            }
        }
    }
    return true;
}

bool Sample::objectAdded(int sender, const std::string &senderPort, const Port *port) {
    auto object = port->objects().back();

    if (port->getName() == "data_in") {
        auto data = DataBase::as(object);
        if (data && data->grid()) {
            dataList.push_back(data);
        }
    }
    else if (port->getName() == "ref_in") {
        if (object->getTimestep()<1) {
            UnstructuredGrid::const_ptr unstr = UnstructuredGrid::as(object);
            StructuredGridBase::const_ptr str = StructuredGridBase::as(object);
            if (!str && !unstr) {
                DataBase::const_ptr data = DataBase::as(object);
                if (data && data->grid()) {
                    unstr =  UnstructuredGrid::as(data->grid());
                    str = StructuredGridBase::as(data->grid());
                    if (!str && !unstr)
                        return true;
                    objListLocal.push_back(data->grid());
                    blockIdx.push_back(object->getBlock());
                }else {
                    return true;
                }
            }else {
            objListLocal.push_back(object);
            blockIdx.push_back(object->getBlock());
            }
        }
    }
    return true;
}

bool Sample::changeParameter(const Parameter *p) {

  return true;
}
