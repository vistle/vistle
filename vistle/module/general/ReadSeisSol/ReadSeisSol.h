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

/* #include <boost/smart_ptr/shared_ptr.hpp> */
#include <memory>
#include <vector>
#include <vistle/module/reader.h>
#include <vistle/core/unstr.h>

//forwarding cpp
class XdmfItem;
class XdmfDomain;
class XdmfGridCollection;
class XdmfAttribute;
class XdmfAttributeCenter;
class XdmfHeavyDataController;
class XdmfArray;
class XdmfArrayType;
class XdmfUnstructuredGrid;

/* namespace { */
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
/* } // namespace */


class ReadSeisSol: public vistle::Reader {
public:
    //default constructor
    ReadSeisSol(const std::string &name, int moduleID, mpi::communicator comm);
    /* ~ReadSeisSol() override; */
    /* ReadSeisSol(const ReadSeisSol&) = delete; //rule of three */
    /* ReadSeisSol& operator=(const ReadSeisSol&) = delete; //rule of three */

private:
    //Vistle functions
    bool read(Token &token, int timestep, int block) override;
    bool examine(const vistle::Parameter *param) override;

    //xdmf
    void setArrayType(boost::shared_ptr<const XdmfArrayType> type);
    void setGridCenter(boost::shared_ptr<const XdmfAttributeCenter> type);

    void readXdmfHeavyController(XdmfArray *xArr, const boost::shared_ptr<XdmfHeavyDataController> &controller);
    bool fillUnstrGridCoords(vistle::UnstructuredGrid::ptr unst, XdmfArray *xArrGeo);
    bool fillUnstrGridConnectList(vistle::UnstructuredGrid::ptr unstr, XdmfArray *xArrConn);
    template<class T>
    void fillUnstrGridElemList(vistle::UnstructuredGrid::ptr unstr, const T &numCornerPerElem);
    void fillUnstrGridTypeList(vistle::UnstructuredGrid::ptr unstr, const vistle::UnstructuredGrid::Type &type);
    vistle::UnstructuredGrid::ptr generateUnstrGridFromXdmfGrid(XdmfUnstructuredGrid *xunstr);

    bool examineXdmf(); //for parameter choice
    bool XAttributeInSet(const std::string &name);
    void clearChoice();
    void iterateXdmfDomain(const boost::shared_ptr<XdmfDomain> &item);
    void iterateXdmfGridCollection(const boost::shared_ptr<XdmfGridCollection> &xgridCol);
    void iterateXdmfUnstrGrid(const boost::shared_ptr<XdmfUnstructuredGrid> &xugrid);
    void iterateXdmfAttribute(const boost::shared_ptr<XdmfAttribute> &xatt);
    /* bool readXDMF(shared_ptr<XdmfArray> &array, const HDF5ControllerParameter &param); */

    //hdf5
    //implement later maybe
    /* bool readHDF5(); */
    /* bool openHDF5(); */
    std::unordered_set<vistle::IntParameter *> m_attChoice;

    //vistle param
    vistle::StringParameter *m_xfile = nullptr;
};
#endif
