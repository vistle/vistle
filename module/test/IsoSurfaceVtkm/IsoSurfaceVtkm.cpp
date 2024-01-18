#include <vistle/alg/objalg.h>
#include <vistle/core/triangles.h>
#include <vistle/core/unstr.h>
#include <vistle/core/structuredgridbase.h>
#include <vistle/util/stopwatch.h>

#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/filter/contour/Contour.h>

#include "IsoSurfaceVtkm.h"
#include "VtkmUtils.h"

using namespace vistle;

MODULE_MAIN(IsoSurfaceVtkm)

/*
TODO:
    - support input other than per-vertex mapped data on unstructured grid
    - add second input port from original isosurface module (mapped data)
*/

IsoSurfaceVtkm::IsoSurfaceVtkm(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm)
{
    setDefaultCacheMode(ObjectCache::CacheDeleteLate);

    createInputPort("data_in", "input gird or geometry with scalar data");
    m_mapDataIn = createInputPort("mapdata_in", "additional mapped field");
    m_dataOut = createOutputPort("data_out", "surface with mapped data");

    m_isovalue = addFloatParameter("isovalue", "isovalue", 0.0);

    setReducePolicy(message::ReducePolicy::OverAll);

    m_computeNormals =
        addIntParameter("compute_normals", "compute normals (structured grids only)", false, Parameter::Boolean);
}

IsoSurfaceVtkm::~IsoSurfaceVtkm()
{}

bool IsoSurfaceVtkm::compute(const std::shared_ptr<vistle::BlockTask> &task) const
{
    // make sure input data is supported
    auto isoData = task->expect<Vec<Scalar>>("data_in");
    if (!isoData) {
        sendError("need scalar data on data_in");
        return true;
    }
    auto splitIso = splitContainerObject(isoData);
    auto grid = splitIso.geometry;
    if (!grid) {
        sendError("no grid on scalar input data");
        return true;
    }

    auto isoField = splitIso.mapped;
    if (isoField->guessMapping() != DataBase::Vertex) {
        sendError("need per-vertex mapping on data_in");
        return true;
    }


    auto mapData = task->accept<Object>(m_mapDataIn);
    auto splitMap = splitContainerObject(mapData);
    auto mapField = splitMap.mapped;
    if (mapField) {
        auto &mg = splitMap.geometry;
        if (!mg) {
            sendError("no grid on mapped data");
            return true;
        }
        if (mg->getHandle() != grid->getHandle()) {
            sendError("grids on mapped data and iso-data do not match");
            std::cerr << "grid mismatch: mapped: " << *mapField << ",\n    isodata: " << *isoField << std::endl;
            return true;
        }
    }

    // transform vistle dataset to vtkm dataset
    vtkm::cont::DataSet vtkmDataSet;
    auto status = vtkmSetGrid(vtkmDataSet, grid);
    if (status == VtkmTransformStatus::UNSUPPORTED_GRID_TYPE) {
        sendError("Currently only supporting unstructured grids");
        return true;
    } else if (status == VtkmTransformStatus::UNSUPPORTED_CELL_TYPE) {
        sendError("Can only transform these cells from vistle to vtkm: point, bar, triangle, polygon, quad, tetra, "
                  "hexahedron, pyramid");
        return true;
    }

    std::string isospecies = isoField->getAttribute("_species");
    if (isospecies.empty())
        isospecies = "isodata";
    status = vtkmAddField(vtkmDataSet, isoField, isospecies);
    if (status == VtkmTransformStatus::UNSUPPORTED_FIELD_TYPE) {
        sendError("Unsupported iso field type");
        return true;
    }

    std::string mapspecies;
    if (mapField) {
        mapspecies = mapField->getAttribute("_species");
        if (mapspecies.empty())
            mapspecies = "mapped";
        status = vtkmAddField(vtkmDataSet, mapField, mapspecies);
        if (status == VtkmTransformStatus::UNSUPPORTED_FIELD_TYPE) {
            sendError("Unsupported mapped field type");
            return true;
        }
    }

    // apply vtkm isosurface filter
    vtkm::filter::contour::Contour isosurfaceFilter;
    isosurfaceFilter.SetActiveField(isospecies);
    isosurfaceFilter.SetIsoValue(m_isovalue->getValue());
    isosurfaceFilter.SetMergeDuplicatePoints(false);
    isosurfaceFilter.SetGenerateNormals(m_computeNormals->getValue() != 0);
    std::string normalsName("_normals");
    isosurfaceFilter.SetNormalArrayName(normalsName);
    auto isosurface = isosurfaceFilter.Execute(vtkmDataSet);

    // transform result back into vistle format
    Object::ptr geoOut = vtkmIsosurfaceToVistleTriangles(isosurface);
    if (geoOut) {
        updateMeta(geoOut);
        geoOut->copyAttributes(isoField);
        geoOut->copyAttributes(grid, false);
        geoOut->setTransform(grid->getTransform());
        if (geoOut->getTimestep() < 0) {
            geoOut->setTimestep(grid->getTimestep());
            geoOut->setNumTimesteps(grid->getNumTimesteps());
        }
        if (geoOut->getBlock() < 0) {
            geoOut->setBlock(grid->getBlock());
            geoOut->setNumBlocks(grid->getNumBlocks());
        }

        if (m_computeNormals->getValue() != 0) {
            Object::ptr nobj = vtkmGetField(isosurface, normalsName);
            if (auto n = Vec<Scalar, 3>::as(nobj)) {
                auto normals = std::make_shared<vistle::Normals>(0);
                normals->d()->x[0] = n->d()->x[0];
                normals->d()->x[1] = n->d()->x[1];
                normals->d()->x[2] = n->d()->x[2];
                updateMeta(normals);
                if (auto coords = Coords::as(geoOut)) {
                    coords->setNormals(normals);
                } else if (auto str = StructuredGridBase::as(geoOut)) {
                    str->setNormals(normals);
                }
            }
        }
    }

    DataBase::ptr mapped;
    if (mapField) {
        mapped = vtkmGetField(isosurface, mapspecies);
    }
    if (mapped) {
        std::cerr << "mapped data: " << *mapped << std::endl;
        mapped->copyAttributes(mapField);
        mapped->setGrid(geoOut);
        updateMeta(mapped);
        task->addObject(m_dataOut, mapped);
    } else {
        task->addObject(m_dataOut, geoOut);
    }

    return true;
}
