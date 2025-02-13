#ifndef VISTLE_READITLRFS3D_READITLRBIN_H
#define VISTLE_READITLRFS3D_READITLRBIN_H

#include <vistle/core/rectilineargrid.h>
#include <vistle/core/vec.h>
#include <vistle/module/reader.h>
#include <vistle/util/filesystem.h>

#include <string>
#include <vector>

class ReadItlrBin: public vistle::Reader {
public:
    ReadItlrBin(const std::string &name, int moduleID, mpi::communicator comm);
    ~ReadItlrBin();

    // reader interface
    bool examine(const vistle::Parameter *param) override;
    bool prepareRead() override;
    bool read(vistle::Reader::Token &token, int timestep = -1, int block = -1) override;
    bool finishRead() override;

private:
    static const int NumPorts = 3;
    int SplitDim = 0;

    std::vector<vistle::RectilinearGrid::ptr> readGridBlocks(const std::string &filename, int npart = 1);
    std::vector<std::string> readListFile(const std::string &filename) const;
    vistle::DataBase::ptr readFieldBlock(const std::string &filename, int part) const;

    vistle::StringParameter *m_gridFilename, *m_filename[NumPorts];
    vistle::IntParameter *m_numPartitions;
    vistle::IntParameter *m_incrementFilename;
    std::string m_species[NumPorts];
#if 0
   vistle::IntParameter *m_distributeTimesteps;
   vistle::IntParameter *m_firstStep, *m_lastStep, *m_stepSkip;
#endif
    vistle::Port *m_dataOut[NumPorts];

    int m_nparts;
    vistle::Index m_dims[3];

    struct Block {
        int part = 0;
        int begin = 0, end = 0;
        int ghost[2] = {0, 0};
    };

    Block computeBlock(int part) const;

    bool m_haveListFile = false;
    bool m_haveFileList = false;
    std::string m_scalarFile[NumPorts];
    vistle::filesystem::path m_directory[NumPorts];
    std::vector<std::string> m_fileList[NumPorts];
    std::vector<vistle::RectilinearGrid::ptr> m_grids;
};
#endif
