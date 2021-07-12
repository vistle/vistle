#include "slowMpi.h"
#include <vistle/util/sleep.h>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/exception.hpp>
#include <boost/mpi/collectives.hpp>
#include <mpi.h>
#include <thread>
#include <chrono>

constexpr auto interval = std::chrono::microseconds(500);
namespace detail
{
template<typename T>
void broadcast(boost::mpi::communicator &c, T&t, int root)
{
    const int tag = 37;
    if (c.rank() == root) {
        for (int i = 0; i < c.size(); i++) {
            if (i != c.rank()) {
                c.send(i, tag, t);
            }
        }

    } else {
        while (true) {
            auto flag = c.iprobe(root, tag);
            if (flag) {
                c.recv(root, tag, t);
                return;
            }
            std::this_thread::sleep_for(interval);
        }
    }
}

}

void vistle::insitu::waitForRank(boost::mpi::communicator &c, int rank)
{
    int i = 2139;
    detail::broadcast(c, i, rank);
    assert(i == 2139);
}

void vistle::insitu::broadcast(boost::mpi::communicator &c, int val, int root)
{
    detail::broadcast(c, val, root);
}

void vistle::insitu::broadcast(boost::mpi::communicator &c, bool val, int root)
{
    detail::broadcast(c, val, root);
}
