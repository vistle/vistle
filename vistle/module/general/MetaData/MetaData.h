#ifndef METADATA_H
#define METADATA_H

#include <vistle/module/module.h>
#include <vistle/core/vector.h>

class MetaData: public vistle::Module {
public:
    MetaData(const std::string &name, int moduleID, mpi::communicator comm);
    ~MetaData();

private:
    std::map<int, vistle::Object::const_ptr> m_objs;
    virtual bool compute();

    vistle::IntParameter *m_kind;
    vistle::IntParameter *m_modulus;
    vistle::IntVectorParameter *m_range;
};

#endif
