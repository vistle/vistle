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
#include <memory>
#include <vector>
#include <vistle/module/reader.h>
#include <vistle/core/unstr.h>
#include <boost/smart_ptr/shared_ptr.hpp>

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

constexpr int NUM_ATTRIBUTES{3};

class ReadSeisSol final: public vistle::Reader {
public:
    //default constructor
    ReadSeisSol(const std::string &name, int moduleID, mpi::communicator comm);
    ~ReadSeisSol() override;

private:
    /*************** template **************/
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

    //general
    template<class Ret, class... Args>
    auto switchSeisMode(std::function<Ret(Args...)> xdmfFunc, std::function<Ret(Args...)> hdfFunc, Args... args);
    template<class Ret, class... Args>
    auto callSeisModeFunction(Ret (ReadSeisSol::*xdmfFunc)(Args...), Ret (ReadSeisSol::*hdfFunc)(Args...),
                              Args... args);

    void releaseXdmfObjects();
    bool checkElemVolumeInverted(vistle::UnstructuredGrid::ptr unstr, XdmfArray *geo);
    /* bool checkBlocks(); */
    void clearChoice();
    /* void initBlocks(); */
    void initScalar();
    void initObserveParameter();

    //Vistle functions
    bool read(Token &token, int timestep, int block) override;
    bool examine(const vistle::Parameter *param) override;
    bool prepareRead() override;
    bool finishRead() override;

    //hdf5
    bool hdfModeNotImplemented();
    bool hdfModeNotImplementedRead(Token &, int, int);

    //xdmf
    vistle::UnstructuredGrid::ptr reuseGrid(XdmfUnstructuredGrid *xUgrid, int block);
    bool prepareReadXdmf();
    bool finishReadXdmf();
    bool readXdmfUnstrParallel(XdmfArray *arrayGeo, const XdmfHeavyDataController *defaultController, const int block);
    bool readXdmfHDF5Data(Token &token, int timestep, int block);
    void readXdmfHDF5TopologyParallel(XdmfArray *xArrTopo,
                                      const boost::shared_ptr<XdmfHeavyDataController> &defaultControllerTopo,
                                      const int block);
    void readXdmfHDF5AttributeParallel(XdmfArray *xArrAtt, unsigned &attDim,
                                       const boost::shared_ptr<XdmfHeavyDataController> &defaultController,
                                       const int block, const int timestep);

    void setArrayType(boost::shared_ptr<const XdmfArrayType> type);
    void setGridCenter(const boost::shared_ptr<const XdmfAttributeCenter> &type);
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
    /* std::array<vistle::IntParameter *, 3> m_blocks; */
    std::array<vistle::StringParameter *, NUM_ATTRIBUTES> m_attributes;
    vistle::IntParameter *m_block = nullptr;
    vistle::IntParameter *m_seisMode = nullptr;
    vistle::IntParameter *m_parallelMode = nullptr;
    vistle::IntParameter *m_reuseGrid = nullptr;
    vistle::StringParameter *m_file = nullptr;

    //vistle param ptr
    vistle::UnstructuredGrid::ptr m_vugrid_ptr = nullptr;

    //vistle ports
    vistle::Port *m_gridOut = nullptr;
    std::array<vistle::Port *, NUM_ATTRIBUTES> m_scalarsOut;

    //general
    std::vector<std::string> m_attChoiceStr;
    DynamicPseudoEnum<std::string> m_xAttSelect;

    //xdmf param
    boost::shared_ptr<XdmfGridCollection> xgridCollect = nullptr;
};
#endif
