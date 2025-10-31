#ifndef VISTLE_READSEISSOL_READSEISSOL_H
#define VISTLE_READSEISSOL_READSEISSOL_H

/**************************************************************************\
 **                                                                      **
 ** Description: Read module for ChEESE Seismic HDF5/XDMF-files  	     **
 **                                                                      **
 ** Author:    Marko Djuric                                              **
 **                                                                      **
 ** Date:  04.03.2021                                                    **
\**************************************************************************/

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

    void releaseXdmfObjects();
    bool checkGeoElemVolume(vistle::UnstructuredGrid::ptr unstr, XdmfArray *xdmfArrGeo);
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

    //xdmf
    vistle::UnstructuredGrid::ptr reuseGrid(XdmfUnstructuredGrid *xdmfUGrid, int block);
    bool readXdmfUnstrParallel(XdmfArray *xmdfArrGeo, const XdmfHeavyDataController *defaultController,
                               const int block);
    bool readXdmfHDF5Data(Token &token, int timestep, int block);
    void readXdmfHDF5TopologyParallel(XdmfArray *xdmfArrTopo,
                                      const boost::shared_ptr<XdmfHeavyDataController> &defaultControllerTopo,
                                      const int block);
    void readXdmfHDF5AttributeParallel(XdmfArray *xdmfArrAtt, unsigned &attDim,
                                       const boost::shared_ptr<XdmfHeavyDataController> &defaultController,
                                       const int block, const int timestep);

    void setArrayType(boost::shared_ptr<const XdmfArrayType> type);
    void setGridCenter(const boost::shared_ptr<const XdmfAttributeCenter> &type);
    void readXdmfHeavyController(XdmfArray *xdmfArr, const boost::shared_ptr<XdmfHeavyDataController> &controller);

    vistle::Vec<vistle::Scalar>::ptr generateScalarFromXdmfAttribute(XdmfAttribute *xdmfAttribute, const int &timestep,
                                                                     const int &block);
    vistle::UnstructuredGrid::ptr generateUnstrGridFromXdmfGrid(XdmfUnstructuredGrid *xdmfUGrid, const int block);
    bool fillUnstrGridCoords(vistle::UnstructuredGrid::ptr unst, XdmfArray *xdmfArrGeo);
    bool fillUnstrGridCoords(vistle::UnstructuredGrid::ptr unst, XdmfArray *xdmfArrGeo,
                             const std::set<unsigned> &verticesToRead);
    bool fillUnstrGridConnectList(vistle::UnstructuredGrid::ptr unstr, XdmfArray *xdmfArrConn);
    template<class T>
    void fillUnstrGridElemList(vistle::UnstructuredGrid::ptr unstr, const T &numCornerPerElem);
    void fillUnstrGridTypeList(vistle::UnstructuredGrid::ptr unstr, const vistle::UnstructuredGrid::Type &type);

    bool inspectXdmf(); //for parameter choice
    void inspectXdmfDomain(const XdmfDomain *item);
    void inspectXdmfGridCollection(const XdmfGridCollection *xdmfGridCol);
    void inspectXdmfUnstrGrid(const XdmfUnstructuredGrid *xdmfUGrid);
    void inspectXdmfAttribute(const XdmfAttribute *xdmfAtt);
    void inspectXdmfGeometry(const XdmfGeometry *xdmfGeo);
    void inspectXdmfTopology(const XdmfTopology *xdmfTopo);
    void inspectXdmfTime(const XdmfTime *xdmfTime);

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
