#include <vistle/module/module.h>
#include <vistle/module/resultcache.h>
#include <vistle/core/points.h>

using namespace vistle;

class ColorAttribute: public vistle::Module {
public:
    ColorAttribute(const std::string &name, int moduleID, mpi::communicator comm);
    ~ColorAttribute();

private:
    virtual bool compute();

    StringParameter *p_color;
    ResultCache<Object::ptr> m_cache;
};

using namespace vistle;

ColorAttribute::ColorAttribute(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    Port *din = createInputPort("data_in", "input data", Port::MULTI);
    Port *dout = createOutputPort("data_out", "output data", Port::MULTI);
    din->link(dout);

    p_color = addStringParameter("color", "hexadecimal RGB/RGBA values (#rrggbb or #rrggbbaa)", "#ff00ff");

    addResultCache(m_cache);
}

ColorAttribute::~ColorAttribute()
{}

bool ColorAttribute::compute()
{
    //std::cerr << "ColorAttribute: compute: generation=" << m_generation << std::endl;

    auto color = p_color->getValue();
    Object::const_ptr obj = expect<Object>("data_in");
    if (!obj)
        return true;

    Object::ptr out = obj->clone();
    if (auto entry = m_cache.getOrLock(obj->getName(), out)) {
        out = obj->clone();
        out->addAttribute(attribute::Color, color);
        updateMeta(out);
        m_cache.storeAndUnlock(entry, out);
    }
    addObject("data_out", out);

    return true;
}

MODULE_MAIN(ColorAttribute)
