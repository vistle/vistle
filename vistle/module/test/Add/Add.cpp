#include <sstream>
#include <iomanip>

#include <boost/mpl/for_each.hpp>

#include <core/vec.h>
#include <module/module.h>
#include <core/scalars.h>


using namespace vistle;

class Add: public vistle::Module {

 public:
   Add(const std::string &shmname, const std::string &name, int moduleID);
   ~Add();

 private:
   bool handled;

   virtual bool compute();

   template<int Dim>
   friend struct Compute;

   template<int Dim>
   struct Compute {
      Object::const_ptr object;
      Add *module;
      Compute(Object::const_ptr obj, Add *module)
         : object(obj)
         , module(module)
      {
      }
      template<typename S> void operator()(S) {

         typedef Vec<S,Dim> V;
         typename V::const_ptr in(V::as(object));
         if (!in)
            return;

         module->handled = true;

         size_t size = in->getSize();
         typename V::ptr out(new V(size));
         for (int c=0; c<Dim; ++c) {
            const S *i = &in->x(c)[0];
            S *o = out->x(c).data();
            for (unsigned int index = 0; index < size; index ++) {
               o[index] = i[index] + module->rank() + 1;
            }
         }
         out->copyAttributes(in);
         module->addObject("data_out", out);
      }
   };

};

using namespace vistle;

Add::Add(const std::string &shmname, const std::string &name, int moduleID)
   : Module("Add", shmname, name, moduleID) {

   createInputPort("data_in");
   createOutputPort("data_out");
}

Add::~Add() {

}

bool Add::compute() {

   Object::const_ptr obj = expect<Object>("data_in");
   if (!obj)
      return false;

   handled = false;
   boost::mpl::for_each<Scalars>(Compute<1>(obj, this));
   boost::mpl::for_each<Scalars>(Compute<3>(obj, this));

   if (!handled) {
      std::cerr << "Add: could not handle input" << std::endl;
   }

   return true;
}

MODULE_MAIN(Add)

