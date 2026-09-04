#include <vistle/module/module.h>
#include <vistle/module/resultcache.h>
#include <vistle/core/object.h>
#include <vistle/util/enum.h>
#include <sstream>

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(AnimationFillMode, (Nothing)(Last)(Cycle))


class AnimationFilling: public vistle::Module {
public:
    AnimationFilling(const std::string &name, int moduleID, mpi::communicator comm);
    ~AnimationFilling();

private:
    bool compute() override;

    IntParameter *p_fillMode = nullptr;

    ResultCache<Object::ptr> m_cache;
};

using namespace vistle;

AnimationFilling::AnimationFilling(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    Port *din = createInputPort("data_in", "input data", Port::MULTI);
    Port *dout = createOutputPort("data_out", "output data", Port::MULTI);
    linkPorts(din, dout);

    p_fillMode = addIntParameter("fill_mode", "how to handle animations of differing lengths", 0, Parameter::Choice);
    V_ENUM_SET_CHOICES(p_fillMode, AnimationFillMode);

    addResultCache(m_cache);
}

AnimationFilling::~AnimationFilling() = default;

bool AnimationFilling::compute()
{
    Object::const_ptr obj = expect<Object>("data_in");
    if (!obj)
        return true;

    auto fillMode = static_cast<AnimationFillMode>(p_fillMode->getValue());

    Object::ptr out;
    if (auto ent = m_cache.getOrLock(obj->getName(), out)) {
        out = obj->clone();

        std::string mode = "nothing";
        switch (fillMode) {
        case Last:
            mode = "last";
            break;
        case Cycle:
            mode = "cycle";
            break;
        case Nothing:
            break;
        }
        out->addAttribute(attribute::AnimationFill, mode.c_str());

        updateMeta(out);
        m_cache.storeAndUnlock(ent, out);
    }


    addObject("data_out", out);

    return true;
}

MODULE_MAIN(AnimationFilling)
