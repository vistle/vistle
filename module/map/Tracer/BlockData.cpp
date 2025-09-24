#include "BlockData.h"
#include "Integrator.h"
#include <vistle/core/vec.h>
#include <mutex>


using namespace vistle;

BlockData::BlockData(Index i, Object::const_ptr grid, Vec<Scalar, 3>::const_ptr vdata, DataBase::const_ptr pdata[])
: m_grid(grid)
, m_gridInterface(m_grid->getInterface<GridInterface>())
, m_vecfld(vdata)
, m_vecmap(DataBase::Vertex)
, m_vx(nullptr)
, m_vy(nullptr)
, m_vz(nullptr)
{
    m_transform = m_grid->getTransform();
    m_invTransform = m_transform.inverse();
    m_velocityTransform = m_transform.block<3, 3>(0, 0);

    if (m_vecfld) {
        m_vx = m_vecfld->x().data();
        m_vy = m_vecfld->y().data();
        m_vz = m_vecfld->z().data();

        m_vecmap = m_vecfld->guessMapping();
        if (m_vecmap == DataBase::Unspecified)
            m_vecmap = DataBase::Vertex;
    }

    for (int i = 0; i < NumFields; ++i) {
        m_field[i] = pdata[i];
        if (i == 0)
            continue; // skip first field, which is the velocity field
        const auto &f = m_field[i];
        if (!f)
            continue;

        auto m = f->guessMapping();
        if (m == DataBase::Unspecified) {
            m = DataBase::Vertex;
        }

        if (auto sca = Vec<Scalar>::as(f)) {
            m_scalmap.push_back(m);
            m_scal.push_back(sca->x().data());
        } else if (auto vec = Vec<Scalar, 3>::as(f)) {
            for (int dim = 0; dim < 3; ++dim) {
                m_scalmap.push_back(m);
                m_scal.push_back(vec->x(dim).data());
            }
        }
    }
}

BlockData::~BlockData()
{}

const GridInterface *BlockData::getGrid()
{
    return m_gridInterface;
}

Vec<Scalar, 3>::const_ptr BlockData::getVecFld()
{
    return m_vecfld;
}

DataBase::Mapping BlockData::getVecMapping() const
{
    return m_vecmap;
}

const Matrix4 &BlockData::transform() const
{
    return m_transform;
}

const Matrix4 &BlockData::invTransform() const
{
    return m_invTransform;
}

const Matrix3 &BlockData::velocityTransform() const
{
    return m_velocityTransform;
}
