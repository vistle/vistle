#include <vistle/module/module.h>

using namespace vistle;
namespace mpi = boost::mpi;

class Distribute: public vistle::Module {
public:
    Distribute(const std::string &name, int moduleID, mpi::communicator comm);
    ~Distribute();

private:
    virtual bool compute() override;
};

using namespace vistle;

Distribute::Distribute(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    setSchedulingPolicy(message::SchedulingPolicy::LazyGang);

    createInputPort("data_in", "input data");
    createOutputPort("data_out", "output data");
}

Distribute::~Distribute()
{}

bool Distribute::compute()
{
    auto obj = accept<Object>("data_in");
    int have = obj ? 1 : 0;
    std::vector<int> haveObject;
    mpi::all_gather(comm(), have, haveObject);
    for (int r = 0; r < size(); ++r) {
        if (haveObject[r]) {
            Object::const_ptr xmit;
            if (r == rank()) {
                auto nobj = obj->clone();
                xmit = nobj;
                updateMeta(nobj);
                broadcastObject(xmit, r);
                addObject("data_out", nobj);
            } else {
                broadcastObject(xmit, r);
                passThroughObject("data_out", xmit);
            }
        }
    }

    return true;
}

MODULE_MAIN(Distribute)
