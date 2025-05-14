#ifndef VISTLE_PRINTOBJECTS_PRINTOBJECTS_H
#define VISTLE_PRINTOBJECTS_PRINTOBJECTS_H

#include <vistle/module/module.h>

class PrintObjects: public vistle::Module {
public:
    PrintObjects(const std::string &name, int moduleID, mpi::communicator comm);
    ~PrintObjects();

private:
    bool compute() override;
    void print(vistle::Object::const_ptr obj);

    vistle::IntParameter *m_mode = nullptr;
    vistle::StringParameter *m_blocks = nullptr;
    vistle::StringParameter *m_timesteps = nullptr;
    vistle::StringParameter *m_iterations = nullptr;
};

#endif
