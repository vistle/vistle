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

#ifndef _READSEISSOL_H
#define _READSEISSOL_H

#include "vistle/core/parameter.h"
#include <array>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <memory>
#include <vector>
#include <vistle/module/reader.h>
#include <vistle/core/unstr.h>
#include <vistle/core/scalar.h>

//forwarding cpp
class XdmfTime;
class XdmfTopology;
class XdmfGeometry;
class XdmfDomain;
class XdmfGridCollection;
class XdmfAttribute;
class XdmfAttributeCenter;
class XdmfHeavyDataController;
class XdmfArray;
class XdmfArrayType;
class XdmfUnstructuredGrid;

class ReadSeisSol final: public vistle::Reader {
public:
    //default constructor
    ReadSeisSol(const std::string &name, int moduleID, mpi::communicator comm);
    ~ReadSeisSol() override;

private:
    //templates
    //struct
    template<class T>
    struct DynamicPseudoEnum {
        DynamicPseudoEnum() {}
        DynamicPseudoEnum(const std::vector<T> &vec) { init(vec); }
        unsigned &operator[](const T &param) { return indices_map[param]; }
        const unsigned &operator[](const T &param) const { return indices_map[param]; }
        std::map<T, unsigned> &getIndexMap() { return indices_map; }
        const std::map<T, unsigned> &getIndexMap() const { return indices_map; }

    private:
        void init(const std::vector<T> &params);
        std::map<const T, unsigned> indices_map;
    };

    /* struct HDF5ControllerParameter { */
    /*     std::string path; */
    /*     std::string setPath; */
    /*     std::vector<unsigned> start{0, 0, 0}; */
    /*     std::vector<unsigned> stride{1, 1, 1}; */
    /*     std::vector<unsigned> count{0, 0, 0}; */
    /*     std::vector<unsigned> dataSize{0, 0, 0}; */

    /*     typedef decltype(XdmfArrayType::Float32()) ArrayTypePtr; */
    /*     ArrayTypePtr readType; */

    /*     HDF5ControllerParameter(const std::string &path, const std::string &setPath, */
    /*                             const std::vector<unsigned> &readStarts, const std::vector<unsigned> &readStrides, */
    /*                             const std::vector<unsigned> &readCounts, const std::vector<unsigned> &readDataSize, */
    /*                             const ArrayTypePtr &readType = XdmfArrayType::Float32()) */
    /*     : path(path) */
    /*     , setPath(setPath) */
    /*     , start(readStarts) */
    /*     , stride(readStrides) */
    /*     , count(readCounts) */
    /*     , dataSize(readDataSize) */
    /*     , readType(readType) */
    /*     {} */
    /* }; */

    //general
    template<class Ret, class... Args>
    auto switchSeisMode(std::function<Ret(Args...)> xdmfFunc, std::function<Ret(Args...)> hdfFunc, Args... args);
    template<class Ret, class... Args>
    auto callSeisModeFunction(Ret (ReadSeisSol::*xdmfFunc)(Args...), Ret (ReadSeisSol::*hdfFunc)(Args...),
                              Args... args);

    /* template<class InputBlockIt, class OutputBlockIt, class NumericType> */
    /* OutputBlockIt blockPartition(InputBlockIt first, InputBlockIt last, OutputBlockIt d_first, */
    /*                              const NumericType &blockNum); */

    void releaseXdmfObjects();
    bool checkBlocks();

    // Overengineered for this case => maybe for XdmfReader usefull
    /* template<class T, class O> */
    /* struct LinkedFunctionPtr { */
    /*     std::function<T(const O &)> func; */
    /*     LinkedFunctionPtr *nextFunc = nullptr; */
    /* }; */

    //Vistle functions
    bool read(Token &token, int timestep, int block) override;
    bool examine(const vistle::Parameter *param) override;
    bool prepareRead() override;
    bool finishRead() override;

    //hdf5
    bool hdfModeNotImplemented();
    bool hdfModeNotImplementedRead(Token &, int, int);

    //xdmf
    bool prepareReadXdmf();
    void clearChoice();
    bool finishReadXdmf();
    bool readXdmf(Token &token, int timestep, int block);
    bool readXdmfUnstrParallel(XdmfArray *arrayGeo, const XdmfHeavyDataController *defaultController, const int block);
    std::set<unsigned> readXdmfTopologyParallel(XdmfArray *xArrTopo,
                                                          const boost::shared_ptr<XdmfHeavyDataController> defaultControllerTopo,
                                                          const int block);

    void setArrayType(boost::shared_ptr<const XdmfArrayType> type);
    void setGridCenter(boost::shared_ptr<const XdmfAttributeCenter> type);
    void readXdmfHeavyController(XdmfArray *xArr, const boost::shared_ptr<XdmfHeavyDataController> &controller);

    vistle::Vec<vistle::Scalar>::ptr generateScalarFromXdmfAttribute(XdmfAttribute *xattribute, const int &timestep,
                                                                     const int &block);
    vistle::UnstructuredGrid::ptr generateUnstrGridFromXdmfGrid(XdmfUnstructuredGrid *xunstr, const int block);
    bool fillUnstrGridCoords(vistle::UnstructuredGrid::ptr unst, XdmfArray *xArrGeo);
    bool fillUnstrGridCoords(vistle::UnstructuredGrid::ptr unst, XdmfArray *xArrGeo,
                             const std::set<unsigned> &verticesToRead);
    bool fillUnstrGridConnectList(vistle::UnstructuredGrid::ptr unstr, XdmfArray *xArrConn);
    template<class T>
    void fillUnstrGridElemList(vistle::UnstructuredGrid::ptr unstr, const T &numCornerPerElem);
    void fillUnstrGridTypeList(vistle::UnstructuredGrid::ptr unstr, const vistle::UnstructuredGrid::Type &type);

    bool inspectXdmf(); //for parameter choice
    void inspectXdmfDomain(const XdmfDomain *item);
    void inspectXdmfGridCollection(const XdmfGridCollection *xgridCol);
    void inspectXdmfUnstrGrid(const XdmfUnstructuredGrid *xugrid);
    void inspectXdmfAttribute(const XdmfAttribute *xatt);
    void inspectXdmfGeometry(const XdmfGeometry *xgeo);
    void inspectXdmfTopology(const XdmfTopology *xtopo);
    void inspectXdmfTime(const XdmfTime *xtime);

    //vistle param
    /* vistle::IntParameter *m_ghost = nullptr; */
    vistle::IntParameter *m_seisMode = nullptr;
    vistle::IntParameter *m_parallelMode = nullptr;
    vistle::IntParameter *m_reuseGrid = nullptr;
    std::array<vistle::IntParameter *, 3> m_blocks{nullptr, nullptr};
    vistle::StringParameter *m_file = nullptr;
    vistle::StringParameter *m_xattributes = nullptr;

    //ports
    vistle::Port *m_gridOut = nullptr;
    vistle::Port *m_scalarOut = nullptr;

    //general
    std::vector<std::string> m_attChoiceStr;
    DynamicPseudoEnum<std::string> m_xAttSelect;

    //xdmf param
    boost::shared_ptr<XdmfGridCollection> xgridCollect = nullptr;
    XdmfArray *xArrGeo = nullptr;
    XdmfArray *xArrTopo = nullptr;
};
#endif
