#ifndef REPLICATE_H
#define REPLICATE_H

#include <vistle/module/module.h>
#include <vistle/core/vector.h>

class Replicate: public vistle::Module {
public:
    Replicate(const std::string &name, int moduleID, mpi::communicator comm);
    ~Replicate();

private:
    std::map<int, vistle::Object::const_ptr> m_objs;
    virtual bool compute();
};

#endif
