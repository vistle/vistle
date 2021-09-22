#include <vistle/module/module.h>
#include <vistle/core/points.h>

using namespace vistle;

class AttachObject: public vistle::Module {
public:
    AttachObject(const std::string &name, int moduleID, mpi::communicator comm);
    ~AttachObject();

private:
    virtual bool compute();
};

using namespace vistle;

AttachObject::AttachObject(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    Port *din = createInputPort("data_in", "input data", Port::MULTI);
    Port *dout = createOutputPort("data_out", "output data", Port::MULTI);
    din->link(dout);
}

AttachObject::~AttachObject()
{}

bool AttachObject::compute()
{
    //std::cerr << "AttachObject: compute: execcount=" << m_executionCount << std::endl;

    Object::const_ptr obj = expect<Object>("data_in");
    if (!obj)
        return false;

    Object::ptr out = std::const_pointer_cast<Object>(obj);
    Object::ptr att(new Points(4));
    obj->addAttachment("test", att);

    addObject("data_out", out);

    return true;
}

MODULE_MAIN(AttachObject)
