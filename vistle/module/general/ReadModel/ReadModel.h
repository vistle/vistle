#ifndef READMODEL_H
#define READMODEL_H

#include <string>
#include <utility> // std::pair
#include <vector>

#include <vistle/module/reader.h>
#include <vistle/util/sysdep.h>

class ReadModel: public vistle::Reader {
public:
    ReadModel(const std::string &name, int moduleID, mpi::communicator comm);
    ~ReadModel() override;

    // reader interface
    bool examine(const vistle::Parameter *param) override;
    bool read(vistle::Reader::Token &token, int timestep = -1, int block = -1) override;
    bool prepareRead() override;

private:
    vistle::Object::ptr load(const std::string &filename) const;

    vistle::Integer m_firstBlock, m_lastBlock;
};

#endif
