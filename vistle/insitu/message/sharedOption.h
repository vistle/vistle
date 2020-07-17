#ifndef VISTLE_INSITU_MESSAGE_SHAREDOPTION
#define VISTLE_INSITU_MESSAGE_SHAREDOPTION
#include "InSituMessage.h"

namespace vistle {
namespace insitu {
namespace message {

//Helper class to store the value of int option of the insitu module in the simulation interface

struct IntOptionBase
{
    virtual void setVal(insitu::message::Message& msg) {};
    int val = 0;

};
template<typename T>
struct IntOption : public IntOptionBase {
    IntOption(int initialVal, std::function<void()> cb = nullptr) :callback(cb) {
        val = initialVal;
    }
    virtual void setVal(insitu::message::Message& msg) override
    {
        auto m = msg.unpackOrCast<T>();
        val = static_cast<typename T::value_type>(m.value);
        if (callback) {
            callback();
        }
    }
    std::function<void()> callback = nullptr;
};

}
}
}



#endif // !VISTLE_INSITU_MESSAGE_SHAREDOPTION





