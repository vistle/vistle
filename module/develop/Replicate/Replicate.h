#ifndef VISTLE_REPLICATE_REPLICATE_H
#define VISTLE_REPLICATE_REPLICATE_H

#include <vistle/module/module.h>
#include <vistle/core/vector.h>

class Replicate: public vistle::Module {
public:
    Replicate(const std::string &name, int moduleID, mpi::communicator comm);

private:
    std::map<int, vistle::Object::const_ptr> m_objs;
    bool compute() override;
};

#endif
