#include "lines.h"
#include "lines_impl.h"
#include "archives.h"
#include "validate.h"

namespace vistle {

Lines::Lines(const size_t numElements, const size_t numCorners, const size_t numVertices, const Meta &meta)
: Lines::Base(Lines::Data::create("", numElements, numCorners, numVertices, meta))
{
    refreshImpl();
}

void Lines::refreshImpl() const
{}

bool Lines::isEmpty()
{
    return Base::isEmpty();
}

bool Lines::isEmpty() const
{
    return Base::isEmpty();
}

bool Lines::checkImpl(std::ostream &os, bool quick) const
{
    return true;
}

void Lines::print(std::ostream &os, bool verbose) const
{
    Base::print(os, verbose);
}

void Lines::Data::initData()
{}

Lines::Data::Data(const Data &other, const std::string &name): Lines::Base::Data(other, name)
{
    initData();
}

Lines::Data::Data(const size_t numElements, const size_t numCorners, const size_t numVertices, const std::string &name,
                  const Meta &meta)
: Lines::Base::Data(numElements, numCorners, numVertices, Object::LINES, name, meta)
{
    initData();
}


Lines::Data *Lines::Data::create(const std::string &objId, const size_t numElements, const size_t numCorners,
                                 const size_t numVertices, const Meta &meta)
{
    const std::string name = Shm::the().createObjectId(objId);
    Data *l = shm<Data>::construct(name)(numElements, numCorners, numVertices, name, meta);
    publish(l);

    return l;
}

V_OBJECT_TYPE(Lines, Object::LINES)
V_OBJECT_CTOR(Lines)
V_OBJECT_IMPL(Lines)

} // namespace vistle
