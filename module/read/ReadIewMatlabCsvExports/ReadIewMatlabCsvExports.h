#ifndef VISTLE_READIEWMATLABCSVEXPORTS_READIEWMATLABCSVEXPORTS_H
#define VISTLE_READIEWMATLABCSVEXPORTS_READIEWMATLABCSVEXPORTS_H

#include <vistle/module/reader.h>
#include <vistle/core/structuredgrid.h>
#include <memory>

#include <rapidcsv.h>

struct VtkFile;

class Transversalflussmaschine: public vistle::Reader {
public:
    Transversalflussmaschine(const std::string &name, int moduleID, mpi::communicator comm);

private:
    // Ports
    vistle::Port *m_gridPort = nullptr;
    vistle::Port *m_dataPort = nullptr;
    // Parameters
    vistle::StringParameter *m_filePathParam = nullptr;
    vistle::IntParameter *m_format = nullptr;
    vistle::Object::ptr m_grid;
    std::array<std::unique_ptr<rapidcsv::Document>, 3> m_data;
    std::unique_ptr<rapidcsv::Document> m_connectivity;
    std::string m_species;
    virtual bool examine(const vistle::Parameter *param = nullptr) override;
    virtual bool read(Token &token, int timestep = -1, int block = -1) override;
};

#endif
