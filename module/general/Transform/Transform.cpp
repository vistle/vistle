#define _USE_MATH_DEFINES // for M_PI on windows

#include <vistle/module/module.h>
#include <vistle/core/points.h>
#include <vistle/util/enum.h>
#include <vistle/alg/objalg.h>
#include <vistle/module/resultcache.h>

using namespace vistle;

class Transform: public vistle::Module {
public:
    Transform(const std::string &name, int moduleID, mpi::communicator comm);

private:
    static const int NumPorts = 5;

    bool compute() override;
    bool changeParameter(const Parameter *param) override;

    Port *data_in[NumPorts], *data_out[NumPorts];

    VectorParameter *p_rotation_axis_angle, *p_scale, *p_translate;
    IntParameter *p_keep_original, *p_repetitions, *p_animation, *p_mirror;

    mutable vistle::ResultCache<vistle::Object::ptr> m_cache;
};

DEFINE_ENUM_WITH_STRING_CONVERSIONS(MirrorMode, (Original)(Mirror_X)(Mirror_Y)(Mirror_Z))
DEFINE_ENUM_WITH_STRING_CONVERSIONS(AnimationMode,
                                    (Keep)(Deanimate)(Animate)(TimestepAsRepetitionCount)(TimestepAsPower))

using namespace vistle;

Transform::Transform(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    for (int i = 0; i < NumPorts; ++i) {
        std::string in = "data_in" + std::to_string(i);
        std::string out = "data_out" + std::to_string(i);
        if (i == 0) {
            in = "data_in";
            out = "data_out";
        }
        data_in[i] = createInputPort(in, "input data");
        data_out[i] = createOutputPort(out, "output data");
        linkPorts(data_in[i], data_out[i]);
    }
    p_translate = addVectorParameter("translate", "translation vector", ParamVector(0., 0., 0.));
    p_scale = addVectorParameter("scale", "scaling factors", ParamVector(1., 1., 1.));
    p_rotation_axis_angle =
        addVectorParameter("rotation_axis_angle", "axis and angle of rotation", ParamVector(1., 0., 0., 0.));

    p_keep_original = addIntParameter("keep_original", "whether to keep input", 0, Parameter::Boolean);
    p_repetitions = addIntParameter("repetitions", "how often the transformation should be repeated", 1);
    p_animation = addIntParameter("animation", "animate repetitions", 0, Parameter::Choice);
    V_ENUM_SET_CHOICES(p_animation, AnimationMode);

    p_mirror = addIntParameter("mirror", "enable mirror", Original, Parameter::Choice);
    V_ENUM_SET_CHOICES(p_mirror, MirrorMode);

    addResultCache(m_cache);
}

bool Transform::changeParameter(const Parameter *param)
{
    int count = 0;
    std::string kind = "";

    if (p_rotation_axis_angle->getValue()[3] != 0) {
        ++count;
        kind = "Rotate";
    }

    auto scal = p_scale->getValue();
    bool scale = false;
    for (int c = 0; c < 3; ++c) {
        if (scal[c] != 1) {
            if (!scale)
                ++count;
            scale = true;
            kind = "Scale";
        }
    }

    bool translate = false;
    auto trans = p_translate->getValue();
    for (int c = 0; c < 3; ++c) {
        if (trans[c] != 0) {
            if (!translate)
                ++count;
            translate = true;
            kind = "Translate";
        }
    }

    auto mirr = p_mirror->getValue();
    if (mirr != Original) {
        ++count;
        kind = "Mirror";
    }

    auto anim = p_animation->getValue();
    if (anim != Keep) {
        ++count;
        kind = "Animate";
    }

    if (count == 1) {
        setItemInfo(kind);
    } else {
        setItemInfo(std::string());
    }

    return Module::changeParameter(param);
}

template<class M>
M pow(const M &m, unsigned e)
{
    assert(e >= 1);
    if (e == 1)
        return m;
    if (e % 2 == 1)
        return m * pow(m, e - 1);
    M tmp_exp = pow(m, e / 2);
    return tmp_exp * tmp_exp;
}

bool Transform::compute()
{
    //std::cerr << "Transform: compute: generation=" << m_generation << std::endl;
    Object::const_ptr obj[NumPorts];
    DataBase::const_ptr data[NumPorts];
    Object::const_ptr geo;

    int timestep = -1;
    for (int port = 0; port < NumPorts; ++port) {
        obj[port] = accept<Object>(data_in[port]);
        if (!obj[port])
            continue;

        auto split = splitContainerObject(obj[port]);
        if (!geo) {
            geo = split.geometry;
        } else if (geo->getHandle() != split.geometry->getHandle()) {
            sendError("differing geometry on multiple ports not supported");
            return true;
        }

        Object::const_ptr geo = split.geometry;
        if (!geo)
            return true;
        data[port] = split.mapped;

        if (timestep == -1) {
            timestep = split.timestep;
        }
    }

    Matrix4 mirrorMat(Matrix4::Identity());
    switch (p_mirror->getValue()) {
    case Mirror_X:
        mirrorMat = Vector4(-1, 1, 1, 1).asDiagonal();
        break;
    case Mirror_Y:
        mirrorMat = Vector4(1, -1, 1, 1).asDiagonal();
        break;
    case Mirror_Z:
        mirrorMat = Vector4(1, 1, -1, 1).asDiagonal();
        break;
    }

    Vector4 rot_axis_angle = p_rotation_axis_angle->getValue();
    Vector3 rot_axis = rot_axis_angle.block<3, 1>(0, 0);
    rot_axis.normalize();
    Scalar rot_angle = rot_axis_angle[3];
    Quaternion qrot(AngleAxis(M_PI / 180. * rot_angle, rot_axis));
    Matrix4 rotMat(Matrix4::Identity());
    Matrix3 rotMat3(qrot.toRotationMatrix());
    rotMat.block<3, 3>(0, 0) = rotMat3;

    Vector3 scale3 = p_scale->getValue();
    Vector4 scale4;
    scale4 << scale3, 1;
    Matrix4 scaleMat = scale4.asDiagonal();

    Vector3 translate = p_translate->getValue();
    Matrix4 translateMat(Matrix4::Identity());
    translateMat.col(3) << translate, 1;

    Matrix4 transform(Matrix4::Identity());
    transform *= translateMat;
    transform *= rotMat;
    transform *= mirrorMat;
    transform *= scaleMat;

    int repetitions = p_repetitions->getValue();
    AnimationMode animation = (AnimationMode)p_animation->getValue();
    switch (animation) {
    case Animate:
        timestep = -1;
        break;
    case Deanimate:
        timestep = -1;
        break;
    case TimestepAsRepetitionCount:
        repetitions = timestep;
        break;
    case TimestepAsPower:
        repetitions = 1;
        if (timestep > 0)
            transform = pow(transform, timestep);
        else
            transform = Matrix4::Identity();
        break;
    case Keep:
        break;
    }

    bool keep_original = p_keep_original->getValue();
    if (keep_original) {
        if (animation != Keep && animation != TimestepAsRepetitionCount) {
            Object::ptr outGeo;
            if (auto entry = m_cache.getOrLock(geo->getName(), outGeo)) {
                outGeo = geo->clone();
                outGeo->setTimestep(timestep);
                if (animation == Deanimate)
                    outGeo->setNumTimesteps(-1);
                updateMeta(outGeo);
                m_cache.storeAndUnlock(entry, outGeo);
            }

            for (int port = 0; port < NumPorts; ++port) {
                if (!obj[port])
                    continue;
                if (data) {
                    auto dataOut = data[port]->clone();
                    dataOut->setGrid(outGeo);
                    updateMeta(dataOut);
                    addObject(data_out[port], dataOut);
                } else {
                    addObject(data_out[port], outGeo);
                }
            }
        } else {
            for (int port = 0; port < NumPorts; ++port) {
                if (!obj[port])
                    continue;
                Object::ptr nobj;
                if (auto entry = m_cache.getOrLock(obj[port]->getName(), nobj)) {
                    nobj = obj[port]->clone();
                    updateMeta(nobj);
                    m_cache.storeAndUnlock(entry, nobj);
                }
                addObject(data_out[port], nobj);
            }
        }
    }

    switch (animation) {
    case Animate:
        timestep = 0;
        break;
    default:
        break;
    }

    Matrix4 t = geo->getTransform();
    for (int i = 0; i < repetitions; ++i) {
        t *= transform;
        std::string key = std::to_string(animation == TimestepAsPower ? timestep : i) + ":" + geo->getName();
        Object::ptr outGeo;
        if (auto entry = m_cache.getOrLock(key, outGeo)) {
            outGeo = geo->clone();
            outGeo->setTransform(t);
            outGeo->setTimestep(timestep);
            if (animation == Deanimate)
                outGeo->setNumTimesteps(-1);
            updateMeta(outGeo);
            m_cache.storeAndUnlock(entry, outGeo);
        }

        for (int port = 0; port < NumPorts; ++port) {
            if (!obj[port])
                continue;
            if (data[port]) {
                auto dataOut = data[port]->clone();
                dataOut->setGrid(outGeo);
                updateMeta(dataOut);
                addObject(data_out[port], dataOut);
            } else {
                addObject(data_out[port], outGeo);
            }
        }

        if (animation != Keep && animation != TimestepAsRepetitionCount) {
            if (animation != Deanimate)
                ++timestep;
        }
    }

    return true;
}

MODULE_MAIN(Transform)
