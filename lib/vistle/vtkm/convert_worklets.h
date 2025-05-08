#ifndef VISTLE_VTKM_CONVERT_WORKLETS_H
#define VISTLE_VTKM_CONVERT_WORKLETS_H

// returns the minimum and maximum dimension of the shapes stored in `shapes` using a Viskores worklet
std::tuple<int, int> getMinMaxDims(viskores::cont::ArrayHandle<viskores::UInt8> &shapes);

#endif
