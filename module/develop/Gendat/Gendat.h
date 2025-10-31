#ifndef VISTLE_GENDAT_GENDAT_H
#define VISTLE_GENDAT_GENDAT_H

#include <vistle/module/reader.h>

class Gendat: public vistle::Reader {
public:
    Gendat(const std::string &name, int moduleID, mpi::communicator comm);

private:
    bool examine(const vistle::Parameter *param) override;
    bool read(Token &token, int timestep, int block) override;

    void block(Token &token, vistle::Index bx, vistle::Index by, vistle::Index bz, vistle::Index b,
               vistle::Index time) const;

    // parameters
    vistle::IntParameter *m_geoMode;
    vistle::IntParameter *m_dataModeScalar;
    vistle::IntParameter *m_dataMode[3];
    vistle::FloatParameter *m_dataScaleScalar;
    vistle::FloatParameter *m_dataScale[3];
    vistle::IntParameter *m_size[3];
    vistle::IntParameter *m_blocks[3];
    vistle::IntParameter *m_ghostLayerWidth;
    vistle::VectorParameter *m_min, *m_max;
    vistle::IntParameter *m_steps;
    vistle::IntParameter *m_animData;
    vistle::IntParameter *m_elementData = nullptr;
    vistle::FloatParameter *m_delay = nullptr;
};

#endif
