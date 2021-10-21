#ifndef VISTLE_INSITU_MINISIM_H
#define VISTLE_INSITU_MINISIM_H
#include <cstddef>
#include <string>
#include <boost/mpi/communicator.hpp>
#include <atomic>
class MiniSimModule;

namespace minisim {

struct Parameter {
    int threads = 1; //number of threads to use
    float velocity_scale = 50.0f;
    int ghostCells = 2;
    int numberOfParticles = 0;
    std::string config_file;
    std::string out_prefix = "";
    bool verbose = true;
    bool sync = false;
    int nblocks = 0; //number of threads to use
    float t_end = 10;
    float dt = .01;
};

class MiniSim {
public:
    void run(MiniSimModule &mod, size_t numTimesteps, const std::string &inputFile,
             const boost::mpi::communicator &comm, const Parameter &param = Parameter{});
    void terminate();

private:
    std::atomic_bool m_terminate{false};
};
} // namespace minisim

#endif //!VISTLE_INSITU_MINISIM_H
