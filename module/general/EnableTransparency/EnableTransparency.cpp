#include <vistle/module/module.h>
#include <vistle/module/resultcache.h>
#include <vistle/core/object.h>
#include <sstream>

using namespace vistle;

class EnableTransparency: public vistle::Module {
public:
    EnableTransparency(const std::string &name, int moduleID, mpi::communicator comm);
    ~EnableTransparency();

private:
    virtual bool compute();

    IntParameter *p_transparency = nullptr;
    IntParameter *p_numPrimitives = nullptr;

    ResultCache<Object::ptr> m_cache;
};

using namespace vistle;

EnableTransparency::EnableTransparency(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    Port *din = createInputPort("data_in", "input data", Port::MULTI);
    Port *dout = createOutputPort("data_out", "output data", Port::MULTI);

    din->link(dout);

    p_transparency = addIntParameter("transparency", "put objects into TRANSPARENT_BIN", 1, Parameter::Boolean);
    p_numPrimitives = addIntParameter("num_primitives", "number of primitives to put into one block", 0);
    setParameterMinimum<Integer>(p_numPrimitives, -1);

    addResultCache(m_cache);
}

EnableTransparency::~EnableTransparency()
{}

bool EnableTransparency::compute()
{
    Object::const_ptr obj = expect<Object>("data_in");
    if (!obj)
        return true;

    Object::ptr out;
    if (auto ent = m_cache.getOrLock(obj->getName(), out)) {
        out = obj->clone();

        if (p_transparency->getValue()) {
            out->addAttribute("_transparent", "true");
        }

        if (p_numPrimitives->getValue() != 0) {
            out->addAttribute("_bin_num_primitives", std::to_string(p_numPrimitives->getValue()));
        }
        m_cache.storeAndUnlock(ent, out);
    }


    addObject("data_out", out);

    return true;
}

MODULE_MAIN(EnableTransparency)
