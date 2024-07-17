#ifndef READVTK_H
#define READVTK_H

#include <vistle/module/reader.h>

#include <vector>
#include <string>
#include <pnetcdf>
#include "src/dataset.h"
#include "src/block.h"

namespace maiapv {
class ReaderBase;
}

class ReadMaiaNetcdf: public vistle::Reader {
public:
    ReadMaiaNetcdf(const std::string &name, int moduleID, mpi::communicator comm);

    // reader interface
    bool examine(const vistle::Parameter *param) override;
    bool read(vistle::Reader::Token &token, int timestep = -1, int block = -1) override;
    bool prepareRead() override;
    bool finishRead() override;

private:
    static const int NumPorts = 3;
    std::array<vistle::Port*, NumPorts> m_cellPort, m_pointPort;
    std::array<vistle::StringParameter*, NumPorts> m_cellDataChoice, m_pointDataChoice;
    //bool changeParameter(const vistle::Parameter *p) override;
    //bool compute() override;
    vistle::StringParameter *m_filename;
    bool    m_isMemIncreasePossible = true;
    int m_noSolvers=-1;
    int m_nDim = -1;
    int m_visType = -1;
    int m_noVisNodes = 0;
    bool m_visualizeSbpData = false;
    int m_visRange[2]={-1,0};
    double m_minMaxBoxPoints[6]={-1,-1,-1,10,10,10};
    double m_boundingBox[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    MPI_Comm m_mpiComm = MPI_COMM_NULL;
    std::vector<std::string> solverName{};
    std::string m_gridFileName;
    std::string m_dataFileName;
    bool m_isDataFile = false;
    bool m_isGridFile = false;
    bool m_isParticleFile = false;
    bool m_isSolutionFile = false;
    bool m_isDgFile = false;
    int LevelRange[2]={-1,0};
    std::vector<maiapv::Dataset> m_datasets{};
    std::vector<maiapv::SolverPV> m_solvers{};
    double MemIncrease=-0.75;
    bool load(Token &token, const std::string &filename, const vistle::Meta &meta = vistle::Meta(), int piece = -1,
              bool ghost = false, const std::string &part = std::string()) const;
    void setChoices(const std::vector<maiapv::Dataset> &fileinfo);
    std::unique_ptr<maiapv::ReaderBase> createReader(int block);
};

#endif
