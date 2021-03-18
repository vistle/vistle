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

#include "vistle/core/port.h"
#include <vistle/module/reader.h>
#include <vistle/core/parameter.h>

#include <XdmfArrayType.hpp>

//forwarding cpp
class XdmfArray;

namespace {
struct HDF5ControllerParameter {
    std::string path;
    std::string setPath;
    std::vector<unsigned> start{0, 0, 0};
    std::vector<unsigned> stride{1, 1, 1};
    std::vector<unsigned> count{0, 0, 0};
    std::vector<unsigned> dataSize{0, 0, 0};

    typedef decltype(XdmfArrayType::Float32()) ArrayTypePtr;
    ArrayTypePtr readType;

    HDF5ControllerParameter(const std::string &path, const std::string &setPath,
                            const std::vector<unsigned> &readStarts, const std::vector<unsigned> &readStrides,
                            const std::vector<unsigned> &readCounts, const std::vector<unsigned> &readDataSize,
                            const ArrayTypePtr &readType = XdmfArrayType::Float32())
    : path(path)
    , setPath(setPath)
    , start(readStarts)
    , stride(readStrides)
    , count(readCounts)
    , dataSize(readDataSize)
    , readType(readType)
    {}
};
} // namespace


class ReadSeisSol: public vistle::Reader {
public:
    //default constructor
    ReadSeisSol(const std::string &name, int moduleID, mpi::communicator comm);
    ~ReadSeisSol() override;

private:
    //Vistle functions
    bool read(Token &token, int timestep, int block) override;
    bool examine(const vistle::Parameter *param) override;

    bool readXDMF(shared_ptr<XdmfArray> &array, const HDF5ControllerParameter &param);

    //hdf5
    bool readHDF5();
    bool openHDF5();

    //vtk
    /* vtkDataSet* openXdmfVTK2(); */
    /* bool openXdmfVTK(vtkSmartPointer<vtkXdmf3Reader> &xreader); */
    /* bool readXdmfVTK(const vtkSmartPointer<vtkXdmf3Reader> &xreader); */

    //parameter
    vistle::StringParameter *p_h5Dir = nullptr;
    vistle::StringParameter *p_xdmfDir = nullptr;
    vistle::IntParameter *p_ghost = nullptr;

    vistle::Port *p_ugrid = nullptr;
};
#endif
