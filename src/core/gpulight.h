/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    https://iland-model.org
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
********************************************************************************************/
#ifndef GPULIGHT_H
#define GPULIGHT_H
#ifdef ILAND_CUDA

#include <unordered_map>
#include <vector>
#include "../gpu/iland_gpu.h"

class Model;
class ResourceUnit;
class Stamp;
class Tree;

/** Runs the light competition of a year on the GPU (see gpu/iland_gpu.h).
  * applyPattern(): collects all trees, then the GPU initialises the light grid, builds
  * the height grid, applies the light influence patterns and reads every tree's raw
  * light sum; the light and height grids are copied back for the rest of the year.
  * readPattern(ru): finishes each tree's LRI on the CPU (species-specific correction,
  * addition to the resource unit), in parallel over resource units like the CPU path.
  * Torus mode is not supported (Model falls back to the CPU). */
class GpuLight
{
public:
    bool applyPattern(Model *model); ///< false on a GPU error (the caller then uses the CPU)
    void readPattern(ResourceUnit *ru) const;
private:
    int stampIndex(const Stamp *stamp);
    void validate(Model *model, const std::vector<struct HeightGridValue> &height_reset); ///< ILAND_GPU_VALIDATE=1: compare with the CPU
    std::unordered_map<const Stamp*, int> mStampIndex;
    std::vector<float> mStampData;
    std::vector<ilandgpu::StampInfo> mStampInfo;
    bool mStampsChanged = false;
    std::vector<ilandgpu::TreeLight> mTrees;
    std::vector<Tree*> mTreePtr;
    std::vector<float> mLriRaw;
    std::vector<int> mRuStart, mRuCount; ///< slice of mTreePtr per resource unit (by RU index)
    void *mPinned = nullptr;
};

#endif // ILAND_CUDA
#endif // GPULIGHT_H
