#ifndef VISTLE_READIAGTECPLOT_READIAGTECPLOT_H
#define VISTLE_READIAGTECPLOT_READIAGTECPLOT_H

#include <vistle/module/reader.h>

class ReadIagTecplot: public vistle::Reader {
public:
    ReadIagTecplot(const std::string &name, int moduleID, mpi::communicator comm);

private:
    static const int NumPorts = 3;

    bool examine(const vistle::Parameter *param) override;
    bool read(Token &token, int timestep, int block) override;

    vistle::StringParameter *m_filename;
    vistle::Port *m_grid = nullptr;
    vistle::Port *m_p = nullptr;
    vistle::Port *m_rho = nullptr;
    vistle::Port *m_n = nullptr;
    vistle::Port *m_u = nullptr;
    vistle::Port *m_v = nullptr;
};
#endif
