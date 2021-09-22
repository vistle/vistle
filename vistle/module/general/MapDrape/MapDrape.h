#ifndef MAPDRAPE_H
#define MAPDRAPE_H

#include <vistle/module/module.h>
#include <vistle/core/vector.h>
#include <vistle/core/coords.h>

using namespace vistle;

class MapDrape: public vistle::Module {
    static const unsigned NumPorts = 5;

public:
    MapDrape(const std::string &name, int moduleID, mpi::communicator comm);
    ~MapDrape() override;

private:
    bool compute() override;
    bool prepare() override;
    bool reduce(int timestep) override;
    StringParameter *p_mapping_from_, *p_mapping_to_;
    IntParameter *p_permutation;
    VectorParameter *p_offset;
    float offset[3];
    Port *data_in[NumPorts], *data_out[NumPorts];

    std::map<std::string, Coords::ptr> m_alreadyMapped;
};


#endif
