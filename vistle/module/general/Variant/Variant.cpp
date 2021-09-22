#include <vistle/module/module.h>
#include <vistle/core/points.h>
#include <vistle/util/enum.h>

using namespace vistle;

class Variant: public vistle::Module {
public:
    Variant(const std::string &name, int moduleID, mpi::communicator comm);
    ~Variant();

private:
    virtual bool compute();

    StringParameter *p_variant;
    IntParameter *p_visible;

    IntParameter *p_fromAttribute;
    StringParameter *p_attribute;
};


DEFINE_ENUM_WITH_STRING_CONVERSIONS(Visibility, (DontChange)(Visible)(Hidden))

using namespace vistle;

Variant::Variant(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    Port *din = createInputPort("data_in", "input data", Port::MULTI);
    Port *dout = createOutputPort("data_out", "output data", Port::MULTI);
    din->link(dout);

    p_variant = addStringParameter("variant", "variant name", "NULL");
    p_visible = addIntParameter("visibility_default", "control visibility default", Visible, Parameter::Choice);
    V_ENUM_SET_CHOICES(p_visible, Visibility);

    p_fromAttribute =
        addIntParameter("from_attribute", "use another attribute as variant name", false, Parameter::Boolean);
    p_attribute = addStringParameter("attribute", "name of attribute to copy to variant", "_part");
}

Variant::~Variant()
{}

bool Variant::compute()
{
    //std::cerr << "Variant: compute: execcount=" << m_executionCount << std::endl;

    Object::const_ptr obj = expect<Object>("data_in");
    if (!obj)
        return true;

    std::string variant = p_variant->getValue();
    if (p_fromAttribute->getValue()) {
        auto attr = p_attribute->getValue();
        if (attr.empty())
            variant.clear();
        variant = obj->getAttribute(attr);
    }

    if (variant.empty()) {
        auto nobj = obj->clone();
        addObject("data_out", nobj);
        return true;
    }

    switch (p_visible->getValue()) {
    case Visible:
        variant += "_on";
        break;
    case Hidden:
        variant += "_off";
        break;
    }

    Object::ptr out = obj->clone();
    out->addAttribute("_variant", variant);
    if (!variant.empty()) {
        out->addAttribute("_plugin", "Variant");
    }
    addObject("data_out", out);

    return true;
}

MODULE_MAIN(Variant)
