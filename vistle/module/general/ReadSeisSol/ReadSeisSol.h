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

#include <vistle/module/reader.h>
#include <vistle/core/parameter.h>

#include <vtk/vtkXdmf3Reader.h>
#include <vtk/vtkSmartPointer.h>
#include <Xdmf.hpp>
/* #include <XdmfHDF5Controller.hpp> */
#include <XdmfArrayType.hpp>
#include <XdmfArray.hpp>

#include <memory>
#include <vector>

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

    /* bool readXDMF(shared_ptr<XdmfArray> &array, const HDF5ControllerParameter &param); */
    /* bool openHDF5(); */

    //vtk
    bool openXDMF(vtkSmartPointer<vtkXdmf3Reader> &xreader);
    bool readXDMF(const vtkSmartPointer<vtkXdmf3Reader> &xreader);

    //parameter
    vistle::StringParameter *p_h5Dir = nullptr;
    vistle::StringParameter *p_xdmfDir = nullptr;
    vistle::IntParameter *p_dsm = nullptr;
    vistle::IntParameter *p_cores = nullptr;
};
#endif
