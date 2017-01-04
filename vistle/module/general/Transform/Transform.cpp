#include <module/module.h>
#include <core/points.h>
#include <util/enum.h>

using namespace vistle;

class Transform: public vistle::Module {

 public:
   Transform(const std::string &shmname, const std::string &name, int moduleID);
   ~Transform();

 private:
   virtual bool compute();

   Port *data_in, *data_out;

   VectorParameter *p_rotation_axis_angle, *p_scale, *p_translate;
   IntParameter *p_keep_original, *p_repetitions, *p_animation, *p_mirror;
};

DEFINE_ENUM_WITH_STRING_CONVERSIONS(MirrorMode, (Original)(Mirror_X)(Mirror_Y)(Mirror_Z))
DEFINE_ENUM_WITH_STRING_CONVERSIONS(AnimationMode, (Keep)(Deanimate)(Animate)(TimestepAsRepetitionCount))

using namespace vistle;

Transform::Transform(const std::string &shmname, const std::string &name, int moduleID)
: Module("add color attribute", shmname, name, moduleID)
{

   data_in = createInputPort("data_in", "input data");
   data_out = createOutputPort("data_out", "output data");

   p_rotation_axis_angle = addVectorParameter("rotation_axis_angle", "axis and angle of rotation", ParamVector(1., 0., 0., 0.));
   p_scale = addVectorParameter("scale", "scaling factors", ParamVector(1., 1., 1.));
   p_translate = addVectorParameter("translate", "translation vector", ParamVector(0., 0., 0.));

   p_keep_original = addIntParameter("keep_original", "whether to keep input", 0, Parameter::Boolean);
   p_repetitions = addIntParameter("repetitions", "how often the transformation should be repeated", 1);
   p_animation = addIntParameter("animation", "animate repetitions", 0, Parameter::Choice);
   V_ENUM_SET_CHOICES(p_animation, AnimationMode);

   p_mirror = addIntParameter("mirror", "enable mirror", Original, Parameter::Choice);
   V_ENUM_SET_CHOICES(p_mirror, MirrorMode);
}

Transform::~Transform() {

}

bool Transform::compute() {

   //std::cerr << "Transform: compute: execcount=" << m_executionCount << std::endl;

   Object::const_ptr obj = expect<Object>(data_in);
   if (!obj)
      return true;

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
   Vector3 rot_axis = rot_axis_angle.block<3,1>(0,0);
   rot_axis.normalize();
   Scalar rot_angle = rot_axis_angle[3];
   Quaternion qrot(AngleAxis(M_PI/180.*rot_angle, rot_axis));
   Matrix4 rotMat(Matrix4::Identity());
   Matrix3 rotMat3(qrot.toRotationMatrix());
   rotMat.block<3,3>(0,0) = rotMat3;

   Vector3 scale3 = p_scale->getValue();
   Vector4 scale4;
   scale4 << scale3, 1;
   Matrix4 scaleMat = scale4.asDiagonal();

   Vector3 translate = p_translate->getValue();
   Matrix4 translateMat(Matrix4::Identity());
   translateMat.col(3) << translate, 1;

   Matrix4 transform(Matrix4::Identity());
   transform *= scaleMat;
   transform *= mirrorMat;
   transform *= rotMat;
   transform *= translateMat;

   bool keep_original = p_keep_original->getValue();
   int repetitions = p_repetitions->getValue();
   AnimationMode animation = (AnimationMode)p_animation->getValue();

   int timestep = animation==Deanimate ? -1 : 0;
   if (keep_original) {
       if (animation != Keep) {
           Object::ptr out = obj->clone();
           out->setTimestep(timestep);
           addObject(data_out, out);
           if (animation != Deanimate)
               ++timestep;
       } else {
           passThroughObject(data_out, obj);
       }
   }

   Matrix4 t = obj->getTransform();
   for (int i=0; i<repetitions; ++i) {
       Object::ptr out = obj->clone();
       t *= transform;
       out->setTransform(t);
       if (animation != Keep) {
           out->setTimestep(timestep);
           if (animation != Deanimate)
               ++timestep;
       }
       addObject(data_out, out);
   }

   return true;
}

MODULE_MAIN(Transform)

