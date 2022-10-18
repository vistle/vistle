#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/triangles.h>
#include <vistle/core/geometry.h>
#include <vistle/core/vec.h>
#include <vistle/core/unstr.h>
#include <vistle/util/math.h>

#include "MetaData.h"

DEFINE_ENUM_WITH_STRING_CONVERSIONS(MetaAttribute,
                                    (MpiRank)(BlockNumber)(TimestepNumber)(VertexIndex)(ElementIndex)(ElementType))

using namespace vistle;

MODULE_MAIN(MetaData)

MetaData::MetaData(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    createInputPort("grid_in", "input grid or geometry");

    createOutputPort("data_out", "geometry with mapped block attribute");

    m_kind = addIntParameter("attribute", "attribute to map to vertices", (Integer)BlockNumber, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_kind, MetaAttribute);
    m_range = addIntVectorParameter("range", "range to which data shall be clamped",
                                    IntParamVector(0, std::numeric_limits<Index>::max()));
    m_modulus = addIntParameter("modulus", "wrap around output value", -1);
}

MetaData::~MetaData()
{}

bool MetaData::compute()
{
    auto obj = expect<Object>("grid_in");
    if (!obj)
        return true;
    Object::const_ptr grid;
    if (obj->getInterface<GeometryInterface>())
        grid = obj;
    DataBase::const_ptr data;
    if (grid) {
        data = DataBase::as(obj);
    } else {
        data = DataBase::as(obj);
        if (!data) {
            return true;
        }
        if (data->grid() && data->grid()->getInterface<GeometryInterface>())
            grid = data->grid();
        if (!grid) {
            return true;
        }
    }

    auto indexed = Indexed::as(grid);
    auto unstr = UnstructuredGrid::as(grid);
    auto geoif = grid->getInterface<GeometryInterface>();
    assert(geoif);

    std::string species;
    const Byte *tl = nullptr;
    const Index kind = m_kind->getValue();
    DataBase::Mapping mapping = DataBase::Vertex;
    switch (kind) {
    case MpiRank:
        species = "rank";
        break;
    case BlockNumber:
        species = "block";
        break;
    case TimestepNumber:
        species = "timestep";
        break;
    case VertexIndex:
        species = "vertex";
        break;
    case ElementIndex:
        species = "elem_index";
        if (indexed) {
            mapping = DataBase::Element;
        } else {
            sendError("ElementIndex requires Indexed as input");
            return true;
        }
        break;
    case ElementType:
        species = "elem_type";
        if (unstr) {
            mapping = DataBase::Element;
            tl = &unstr->tl()[0];
        } else {
            sendError("ElementType requires UnstructuredGrid as input");
            return true;
        }
        break;
    default:
        species = "zero";
        break;
    }

    Index N = mapping == DataBase::Element ? indexed->getNumElements() : geoif->getNumVertices();

    Vec<Index>::ptr out(new Vec<Index>(N));
    auto val = out->x().data();

    const Index block = data ? data->getBlock() : grid->getBlock();
    const Index timestep = getTimestep(obj);

    const Index min = m_range->getValue()[0];
    const Index max = m_range->getValue()[1];
    const Index mod = m_modulus->getValue();
    for (Index i = 0; i < N; ++i) {
        switch (kind) {
        case MpiRank:
            val[i] = rank();
            break;
        case BlockNumber:
            val[i] = block;
            break;
        case TimestepNumber:
            val[i] = timestep;
            break;
        case VertexIndex:
            val[i] = i;
            break;
        case ElementIndex:
            val[i] = i;
            break;
        case ElementType:
            val[i] = tl[i];
            break;

        default:
            val[i] = 0;
            break;
        }
        if (mod > 0)
            val[i] %= mod;
        val[i] = clamp(val[i], min, max);
    }


    out->addAttribute("_species", species);
    if (data)
        out->setMeta(data->meta());
    out->setMapping(mapping);
    out->setGrid(grid);
    updateMeta(out);
    addObject("data_out", out);

    return true;
}
