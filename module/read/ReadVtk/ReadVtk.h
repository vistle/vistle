#ifndef READVTK_H
#define READVTK_H

#include <vistle/module/reader.h>

#include <vector>
#include <string>

class vtkDataSet;
class vtkDataObject;

struct ReadVtkData;

const double ConstantTime = -std::numeric_limits<double>::max();

struct VtkFile {
    std::string filename;
    std::string part;
    std::string index;
    int partNum = -1;
    int pieces = 1;
    double realtime = ConstantTime;
    vtkSmartPointer<vtkDataObject> dataset;
    std::vector<std::string> pointfields;
    std::vector<std::string> cellfields;
};

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

    VtkFile getDataSetMeta(const std::string &filename);
    int getNumPieces(const std::string &filename);
    std::map<double, std::vector<VtkFile>> readXmlCollection(const std::string &filename, bool piecesAsBlocks = false);

    vistle::StringParameter *m_filename;
    vistle::StringParameter *m_cellDataChoice[NumPorts], *m_pointDataChoice[NumPorts];
    vistle::Port *m_cellPort[NumPorts], *m_pointPort[NumPorts];
    vistle::IntParameter *m_readPieces, *m_ghostCells;

    ReadVtkData *m_d = nullptr;
    std::map<std::string, VtkFile> m_files;
};

#endif
