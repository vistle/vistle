#ifndef VISTLE_SPHERESOVERLAP_ALGO_WORKLET_POINTLOCATORCELLLISTS_H
#define VISTLE_SPHERESOVERLAP_ALGO_WORKLET_POINTLOCATORCELLLISTS_H

#include <vtkm/Version.h>

// Note that the version macros are only defined if find_package is called, which is
// not the the case if Vistle builds VTK-m, i.e., VISTLE_INTERNAL_VTKM is defined.
#if defined(VISTLE_EXTERNAL_VTKM) && (VTKM_VERSION_MAJOR < 2 || (VTKM_VERSION_MAJOR == 2 && VTKM_VERSION_MINOR < 2) || \
                                      (VTKM_VERSION_MAJOR == 2 && VTKM_VERSION_MINOR == 2 && VTKM_VERSION_PATCH < 90))
#define VTKM_INTERNAL_POINTLOCATORBASE
#endif

#ifdef VTKM_INTERNAL_POINTLOCATORBASE
#include <vtkm/cont/internal/PointLocatorBase.h>
#else
#include <vtkm/cont/PointLocatorBase.h>
#endif

#include <vistle/core/scalar.h>

#include "OverlapDetector.h"
#include "../ThicknessDeterminer.h"

#ifdef VTKM_INTERNAL_POINTLOCATORBASE
class PointLocatorCellLists: public vtkm::cont::internal::PointLocatorBase<PointLocatorCellLists> {
    using Superclass = vtkm::cont::internal::PointLocatorBase<PointLocatorCellLists>;
#else
class PointLocatorCellLists: public vtkm::cont::PointLocatorBase {
    using Superclass = vtkm::cont::PointLocatorBase;
#endif

public:
    void SetRadii(const vtkm::cont::UnknownArrayHandle &radii)
    {
        assert(radii.CanConvert<vtkm::cont::ArrayHandle<vtkm::FloatDefault>>());
        this->Radii = radii.AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::FloatDefault>>();
    }

    void SetSearchRadius(const vtkm::FloatDefault searchRadius)
    {
        if (this->SearchRadius != searchRadius) {
            assert(searchRadius > 0);
            this->SearchRadius = searchRadius;
            this->SetModified();
        }
    }

    void SetThicknessDeterminer(ThicknessDeterminer determiner)
    {
        this->Determiner = determiner;
    }

    const vtkm::Vec3f &GetMinPoint() const
    {
        return this->Min;
    }
    const vtkm::Vec3f &GetMaxPoint() const
    {
        return this->Max;
    }
    const vtkm::Id3 &GetNumberOfBins() const
    {
        return this->Dims;
    }
    const vtkm::FloatDefault &GetSearchRadius() const
    {
        return this->SearchRadius;
    }

    VTKM_CONT
    OverlapDetector PrepareForExecution(vtkm::cont::DeviceAdapterId device, vtkm::cont::Token &token) const;

private:
    // Search grid properties
    vtkm::Vec3f Min, Max;
    vtkm::Id3 Dims;
    vtkm::FloatDefault SearchRadius;

    // Cell lists
    // a list of all point ids (grouped by the cells that contain them)
    vtkm::cont::ArrayHandle<vtkm::Id> PointIds;
    // entry `i` stores the first index into `PointsIds` which corresponds to a point lying in cell `i`
    vtkm::cont::ArrayHandle<vtkm::Id> CellLowerBounds;
    // entry `i` stores the last index into `PointsIds` which corresponds to a point lying in cell `i`
    vtkm::cont::ArrayHandle<vtkm::Id> CellUpperBounds;

    // point field containing sphere radii
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> Radii;

    ThicknessDeterminer Determiner = OverlapRatio;

    vtkm::cont::ArrayHandle<vtkm::Int8> OffsetsToNeighbors;

    // create the search grid
    VTKM_CONT void Build();

    friend Superclass;
};

#endif
