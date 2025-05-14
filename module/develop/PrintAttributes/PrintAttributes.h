#ifndef VISTLE_PRINTATTRIBUTES_PRINTATTRIBUTES_H
#define VISTLE_PRINTATTRIBUTES_PRINTATTRIBUTES_H

#include <vistle/module/module.h>

class PrintAttributes: public vistle::Module {
public:
    PrintAttributes(const std::string &name, int moduleID, mpi::communicator comm);
    ~PrintAttributes();

private:
    bool compute() override;
    void print(vistle::Object::const_ptr obj);

    vistle::IntParameter *m_mode = nullptr;
    vistle::StringParameter *m_blocks = nullptr;
    vistle::StringParameter *m_timesteps = nullptr;
    vistle::StringParameter *m_iterations = nullptr;
};

#endif
