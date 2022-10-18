#ifndef _READDFG_H
#define _READDFG_H

#include <cstddef>
#include <vistle/core/unstr.h>
#include <vistle/core/polygons.h>
#include <vistle/module/reader.h>

#include <netcdf.h>
#ifdef NETCDF_PARALLEL
#include <netcdf_par.h>
#endif

using namespace vistle;

class ReadIagNetcdf: public vistle::Reader {
public:
    ReadIagNetcdf(const std::string &name, int moduleID, mpi::communicator comm);
    ~ReadIagNetcdf() override;

private:
    static const unsigned NUMPORTS = 3;
    enum FileType { GridFile, DataFile };
    bool prepareRead() override;
    bool read(Token &token, int timestep = -1, int block = -1) override;
    bool examine(const vistle::Parameter *param) override;
    bool finishRead() override;

    bool setVariableList(FileType type, int ncid);
    bool validateFile(std::string fullFileName, std::string &redFileName, FileType type);
    bool emptyValue(vistle::StringParameter *ch) const;
    void readElemType(vistle::Index *cl, vistle::Index *el, const std::string &elem, vistle::Index &idx_cl,
                      vistle::Index &idx_el, const int ncid, vistle::Byte *tl, vistle::Byte type);
    void countElements(const std::string &elem, const int ncid, size_t &numElems, size_t &numCorners);

    StringParameter *m_ncFile, *m_dataFile;
    StringParameter *m_variables[NUMPORTS];
    Port *m_dataOut[NUMPORTS], *m_gridOut;
    StringParameter *m_bvariables[NUMPORTS];
    Port *m_boundaryDataOut[NUMPORTS], *m_boundaryOut, *m_boundaryVarOut;
    std::vector<std::string> m_gridVariables, m_dataVariables;
    std::string m_gridFileName, m_dataFileName;
    std::vector<std::string> m_dataFiles;

    Index m_numVertices = 0;
    UnstructuredGrid::ptr m_grid;
    Polygons::ptr m_boundary;
    Vec<Index>::ptr m_markerData;
};

#endif
