#ifndef VISTLE_READDUISBURG_READDUISBURG_H
#define VISTLE_READDUISBURG_READDUISBURG_H

#include <cstddef>
#include <vistle/module/reader.h>
#include <vistle/core/triangles.h>
#include <vistle/netcdf/ncwrap.h>


using namespace vistle;

class ReadDuisburg: public vistle::Reader {
public:
    ReadDuisburg(const std::string &name, int moduleID, mpi::communicator comm);

private:
    bool prepareRead() override;
    bool read(Token &token, int timestep = -1, int block = -1) override;
    bool examine(const vistle::Parameter *param) override;
    bool finishRead() override;

    bool cellIsWater(const std::vector<double> &h, int i, int j, int dimX, int dimY) const;
    std::unique_ptr<vistle::NcFile> m_ncFile;

    Object::ptr m_grid;
    vistle::Index m_dim[2] = {0, 0};
    Port *m_gridOut = nullptr;
    StringParameter *m_gridFile;
};

#endif
