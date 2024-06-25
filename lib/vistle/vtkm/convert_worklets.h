#ifndef VISTLE_CONVERT_WORKLETS_H
#define VISTLE_CONVERT_WORKLETS_H

// returns the minimum and maximum dimension of the shapes stored in `shapes` using a vtk-m worklet
std::tuple<int, int> getMinMaxDims(vtkm::cont::ArrayHandle<vtkm::UInt8> &shapes);

#endif //VISTLE_CONVERT_WORKLETS_H
