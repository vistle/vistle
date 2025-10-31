#ifndef VISTLE_WRITEPOINTSCSV_WRITEPOINTSCSV_H
#define VISTLE_WRITEPOINTSCSV_WRITEPOINTSCSV_H

#include <vistle/module/module.h>

#include <ostream>
#include <fstream>

class WritePointsCsv: public vistle::Module {
public:
    WritePointsCsv(const std::string &name, int moduleID, mpi::communicator comm);

    static const int NumPorts = 3;
    std::array<vistle::Port *, NumPorts> m_port;
    vistle::StringParameter *m_filename = nullptr;

private:
    bool compute() override;
    bool prepare() override;
    bool reduce(int t) override;

    vistle::IntParameter *m_writeHeader = nullptr;
    vistle::IntParameter *m_writeCoordinates = nullptr;
    vistle::IntParameter *m_writeData = nullptr;
    vistle::IntParameter *m_writeTime = nullptr;
    vistle::IntParameter *m_writeBlock = nullptr;
    vistle::IntParameter *m_writeIndex = nullptr;

    std::ofstream m_csv;
};

#endif
