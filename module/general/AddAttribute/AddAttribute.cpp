#include <vistle/module/module.h>
#include <vistle/module/resultcache.h>
#include <vistle/core/object.h>
#include <sstream>

const int NumAttributes = 3;

using namespace vistle;

class AddAttribute: public vistle::Module {
public:
    AddAttribute(const std::string &name, int moduleID, mpi::communicator comm);
    ~AddAttribute();

private:
    bool compute() override;
    bool changeParameter(const Parameter *p) override;

    StringParameter *p_name[NumAttributes];
    StringParameter *p_nameChoice[NumAttributes];
    StringParameter *p_value[NumAttributes];
    std::vector<std::string> m_choices;

    ResultCache<Object::ptr> m_cache;
};

using namespace vistle;

static const char UserDefined[] = "User Defined";

AddAttribute::AddAttribute(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    Port *din = createInputPort("data_in", "input data", Port::MULTI);
    Port *dout = createOutputPort("data_out", "output data", Port::MULTI);
    linkPorts(din, dout);

    {
        using namespace vistle::attribute;
        m_choices.push_back(UserDefined);
#define A(v, n) m_choices.push_back(v);
#include <vistle/core/standardattributelist.h>
#undef A
    }

    for (int i = 0; i < NumAttributes; ++i) {
        std::string group = "attribute " + std::to_string(i);
        setCurrentParameterGroup(group);

        std::stringstream namechoice;
        namechoice << "choice" << i;
        p_nameChoice[i] = addStringParameter(namechoice.str(), "attribute name", "", Parameter::Choice);
        setParameterChoices(p_nameChoice[i], m_choices);

        std::stringstream name;
        name << "name" << i;
        p_name[i] = addStringParameter(name.str(), "attribute name", "");

        std::stringstream value;
        value << "value" << i;
        p_value[i] = addStringParameter(value.str(), "attribute value", "");
    }

    addResultCache(m_cache);
}

AddAttribute::~AddAttribute()
{}

bool AddAttribute::changeParameter(const Parameter *p)
{
    for (int i = 0; i < NumAttributes; ++i) {
        if (p == p_name[i]) {
            auto name = p_name[i]->getValue();
            auto it = std::find(m_choices.begin(), m_choices.end(), name);
            if (it == m_choices.end()) {
                setParameter(p_nameChoice[i], std::string(UserDefined));
            } else {
                setParameter(p_nameChoice[i], *it);
            }
            break;
        }
        if (p == p_nameChoice[i]) {
            if (p_nameChoice[i]->getValue() != UserDefined && p_nameChoice[i]->getValue() != "") {
                setParameter(p_name[i], p_nameChoice[i]->getValue());
            }
            break;
        }
    }
    if (!p || p == p_name[0] || p == p_value[0] || p == p_nameChoice[0]) {
        auto str = p_name[0]->getValue() + "=" + p_value[0]->getValue();
        setItemInfo(str);
    }
    return Module::changeParameter(p);
}

bool AddAttribute::compute()
{
    Object::const_ptr obj = expect<Object>("data_in");
    if (!obj)
        return true;

    Object::ptr out = obj->clone();
    if (auto entry = m_cache.getOrLock(obj->getName(), out)) {
        out = obj->clone();

        for (int i = 0; i < NumAttributes; ++i) {
            if (!p_name[i]->getValue().empty()) {
                out->addAttribute(p_name[i]->getValue(), p_value[i]->getValue());
            }
        }
        updateMeta(out);
        m_cache.storeAndUnlock(entry, out);
    }

    addObject("data_out", out);

    return true;
}

MODULE_MAIN(AddAttribute)
