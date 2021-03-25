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

#include <boost/smart_ptr/shared_ptr.hpp>
#include <vistle/module/reader.h>
#include <vector>
/* #include <XdmfArrayType.hpp> */

//forwarding cpp
class XdmfArray;
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
    ~ReadSeisSol() override;

private:
    //Vistle functions
    bool read(Token &token, int timestep, int block) override;
    bool examine(const vistle::Parameter *param) override;

    //xdmf
    bool examineXdmf(); //for parameter choice
    /* bool readXDMF(shared_ptr<XdmfArray> &array, const HDF5ControllerParameter &param); */

    //hdf5
    bool readHDF5();
    bool openHDF5();

    /* boost::shared_ptr<XdmfUnstructuredGrid> xugrid; */
};
#endif
