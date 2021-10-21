#include <vistle/insitu/core/slowMpi.h>
#include <mpi.h>
#include <boost/mpi.hpp>
#include <thread>
#include <chrono>
namespace mpi = boost::mpi;
//! state shared among all views
struct GlobalState {
    unsigned timestep;

    GlobalState(): timestep(0) {}

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar &timestep;
    }
};

struct GlobalState g_state;

int main(int argc, char **argv)
{
    int prov;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &prov);
    mpi::communicator c{};

    for (int i = 0; i < 2; i++) {
        if (c.rank() == 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } else
            std::cerr << "rank " << c.rank() << " is waiting for master" << std::endl;

        vistle::insitu::waitForRank(c, 0);
        if (c.rank() == 0) {
            std::cerr << "-----------------------------------" << std::endl;
        }
    }
    while (true) {
        int m = 92138;
        std::string message;
        if (c.rank() == 0) {
            std::cin >> m >> message;
        }
        vistle::insitu::broadcast(c, m, 0);
        std::cerr << c.rank() << ": m = " << m << std::endl;
        if (!m) {
            continue;
        }
        boost::mpi::broadcast(c, message, 0);
        std::cerr << c.rank() << ": message = " << message << std::endl;
        if (m < 0) {
            break;
        }
    }

    MPI_Finalize();
    return 0;
}
