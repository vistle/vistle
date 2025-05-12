#ifndef VISTLE_SPHERESOVERLAP_ALGO_WORKLET_POINTLOCATORCELLLISTS_H
#define VISTLE_SPHERESOVERLAP_ALGO_WORKLET_POINTLOCATORCELLLISTS_H

#include <viskores/cont/PointLocatorBase.h>

#include <vistle/core/scalar.h>

#include "OverlapDetector.h"
#include "../ThicknessDeterminer.h"

class PointLocatorCellLists: public viskores::cont::PointLocatorBase {
    using Superclass = viskores::cont::PointLocatorBase;

public:
    void SetRadii(const viskores::cont::UnknownArrayHandle &radii)
    {
        assert(radii.CanConvert<viskores::cont::ArrayHandle<viskores::FloatDefault>>());
        this->Radii = radii.AsArrayHandle<viskores::cont::ArrayHandle<viskores::FloatDefault>>();
    }

    void SetSearchRadius(const viskores::FloatDefault searchRadius)
    {
        if (this->SearchRadius != searchRadius) {
            assert(searchRadius > 0);
            this->SearchRadius = searchRadius;
            this->SetModified();
        }
    }

    void SetThicknessDeterminer(ThicknessDeterminer determiner) { this->Determiner = determiner; }

    const viskores::Vec3f &GetMinPoint() const { return this->Min; }
    const viskores::Vec3f &GetMaxPoint() const { return this->Max; }
    const viskores::Id3 &GetNumberOfBins() const { return this->Dims; }
    const viskores::FloatDefault &GetSearchRadius() const { return this->SearchRadius; }

    VISKORES_CONT
    OverlapDetector PrepareForExecution(viskores::cont::DeviceAdapterId device, viskores::cont::Token &token) const;

private:
    // Search grid properties
    viskores::Vec3f Min, Max;
    viskores::Id3 Dims;
    viskores::FloatDefault SearchRadius;

    // Cell lists
    // a list of all point ids (grouped by the cells that contain them)
    viskores::cont::ArrayHandle<viskores::Id> PointIds;
    // entry `i` stores the first index into `PointsIds` which corresponds to a point lying in cell `i`
    viskores::cont::ArrayHandle<viskores::Id> CellLowerBounds;
    // entry `i` stores the last index into `PointsIds` which corresponds to a point lying in cell `i`
    viskores::cont::ArrayHandle<viskores::Id> CellUpperBounds;

    // point field containing sphere radii
    viskores::cont::ArrayHandle<viskores::FloatDefault> Radii;

    ThicknessDeterminer Determiner = OverlapRatio;

    viskores::cont::ArrayHandle<viskores::Int8> OffsetsToNeighbors;

    // create the search grid
    VISKORES_CONT void Build();

    friend Superclass;
};

#endif
