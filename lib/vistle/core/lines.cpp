#include "lines.h"
#include "lines_impl.h"
#include "archives.h"

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

bool Lines::checkImpl() const
{
    return true;
}

void Lines::print(std::ostream &os) const
{
    Base::print(os);
}

void Lines::AddLine(std::array<Scalar, 3> point1, std::array<Scalar, 3> point2)
{
    if (this->el().size() == 0)
        this->el().push_back(0);

    auto size = this->x().size();

    this->cl().push_back(size++);

    this->x().push_back(point1[0]);
    this->y().push_back(point1[1]);
    this->z().push_back(point1[2]);

    this->cl().push_back(size++);

    this->x().push_back(point2[0]);
    this->y().push_back(point2[1]);
    this->z().push_back(point2[2]);

    this->el().push_back(size);
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
