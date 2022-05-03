#include "slowMpi.h"
#include <vistle/util/sleep.h>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/exception.hpp>
#include <boost/mpi/collectives.hpp>
#include <mpi.h>
#include <thread>
#include <chrono>
constexpr auto interval = std::chrono::microseconds(500);
namespace detail {
template<typename T>
void broadcast(const boost::mpi::communicator &c, T &t, int root, const T &defaultValue = T{},
               const std::atomic_bool &abort = false)
{
    const int tag = 37;
    // Make proc 0 the root if it isn't already
    if (root != 0) {
        if (c.rank() == root) // only original root does this
            c.send(0, tag, t);
        if (c.rank() == 0) // only new root (zero) does this
            c.recv(root, tag, t);
        root = 0; // everyone does this
    }

    // Compute who the executing proc. will receive its message from.
    int srcProc = 0;
    for (int i = 0; i < 31; i++) {
        int mask = 0x00000001;
        int bit = (c.rank() >> i) & mask;
        if (bit == 1) {
            int mask1 = ~(0x00000001 << i);
            srcProc = (c.rank() & mask1);
            break;
        }
    }
    // Polling Phase
    if (c.rank() != 0) {
        // Everyone posts a non-blocking receive
        auto bcastRecv = c.irecv(srcProc, tag, t);
        // Main polling loop
        vistle::adaptive_wait(true);
        while (!bcastRecv.test()) {
            vistle::adaptive_wait(false);
            if (abort) {
                t = defaultValue;
                return;
            }
        }
    }
    // Send on to other processors phase

    // Determine highest rank proc above the executing proc
    // that it is responsible to send a message to.
    int deltaProc = (c.rank() - srcProc) >> 1;
    if (c.rank() == 0) {
        deltaProc = 1;
        while ((deltaProc << 1) < c.size())
            deltaProc = deltaProc << 1;
    }

    // Send message to other procs the executing proc is responsible for
    while (deltaProc > 0) {
        if (c.rank() + deltaProc < c.size())
            c.send(c.rank() + deltaProc, tag, t);
        deltaProc = deltaProc >> 1;
    }
}

} // namespace detail

void vistle::insitu::waitForRank(const boost::mpi::communicator &c, int rank)
{
    int i = 2139;
    detail::broadcast(c, i, rank);
    assert(i == 2139);
}

void vistle::insitu::broadcast(const boost::mpi::communicator &c, int &val, int root)
{
    detail::broadcast(c, val, root);
}

void vistle::insitu::broadcast(const boost::mpi::communicator &c, bool &val, int root, bool defaultValue,
                               const std::atomic_bool &abort)
{
    detail::broadcast(c, val, root, defaultValue, abort);
}
