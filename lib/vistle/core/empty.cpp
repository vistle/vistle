#include "empty.h"
#include "empty_impl.h"
#include "archives.h"

namespace vistle {

void Empty::refreshImpl() const
{}

bool Empty::isEmpty()
{
    return true;
}

bool Empty::isEmpty() const
{
    return true;
}

bool Empty::checkImpl() const
{
    return true;
}

void Empty::Data::initData()
{}

Empty::Data::Data(const Empty::Data &o, const std::string &n): Empty::Base::Data(o, n)
{}

Empty::Data::Data(const std::string &name, const Meta &m): Empty::Base::Data(Object::EMPTY, name, m)
{}

Empty::Data::~Data()
{}

Empty::Data *Empty::Data::create(const std::string &objId)
{
    const std::string name = Shm::the().createObjectId(objId);
    Data *p = shm<Data>::construct(name)(name);
    publish(p);

    return p;
}

V_OBJECT_TYPE(Empty, Object::EMPTY)
V_OBJECT_CTOR(Empty)
V_OBJECT_IMPL(Empty)

} // namespace vistle
