#ifndef READVTK_H
#define READVTK_H

#include <vistle/module/reader.h>

class vtkDataSet;
struct ReadVtkData;
struct VtkFile;

class ReadVtk: public vistle::Reader {
public:
    ReadVtk(const std::string &name, int moduleID, mpi::communicator comm);
    ~ReadVtk() override;

    // reader interface
    bool examine(const vistle::Parameter *param) override;
    bool read(vistle::Reader::Token &token, int timestep = -1, int block = -1) override;
    bool prepareRead() override;
    bool finishRead() override;

private:
    static const int NumPorts = 3;

    //bool changeParameter(const vistle::Parameter *p) override;
    //bool compute() override;

    bool load(Token &token, const std::string &filename, const vistle::Meta &meta = vistle::Meta(), int piece = -1,
              bool ghost = false, const std::string &part = std::string()) const;
    void setChoices(const VtkFile &fileinfo);

    vistle::StringParameter *m_filename;
    vistle::StringParameter *m_cellDataChoice[NumPorts], *m_pointDataChoice[NumPorts];
    vistle::Port *m_cellPort[NumPorts], *m_pointPort[NumPorts];
    vistle::IntParameter *m_readPieces, *m_ghostCells;

    ReadVtkData *m_d = nullptr;
};

#endif
