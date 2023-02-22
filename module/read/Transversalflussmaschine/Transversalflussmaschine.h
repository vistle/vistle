#ifndef READCSV_H
#define READCSV_H

#include <vistle/module/reader.h>
#include <vistle/core/structuredgrid.h>
#include <memory>

#include <rapidcsv.h>

struct VtkFile;

class Transversalflussmaschine: public vistle::Reader {
public:
    Transversalflussmaschine(const std::string &name, int moduleID, mpi::communicator comm);
    ~Transversalflussmaschine() override;

private:
    // Ports
    vistle::Port *m_gridPort = nullptr;
    vistle::Port *m_dataPort = nullptr;
    // Parameters
    vistle::StringParameter *m_filePathParam = nullptr;
    vistle::Object::ptr m_grid;
    std::unique_ptr<rapidcsv::Document> m_data;
    std::unique_ptr<rapidcsv::Document> m_connectivity;

    virtual bool examine(const vistle::Parameter *param = nullptr) override;
    virtual bool read(Token &token, int timestep = -1, int block = -1) override;
};

#endif
