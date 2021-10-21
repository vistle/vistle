#include <boost/mpi.hpp>

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

int main()
{
    mpi::environment env;

    mpi::broadcast(mpi::communicator(), g_state, 0);
}
