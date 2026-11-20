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
#ifdef ILAND_CUDA
#include "global.h"
#include "gpulight.h"
#include "model.h"
#include "resourceunit.h"
#include "stamp.h"
#include "tree.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

static bool validationRequested()
{
    const char *env = std::getenv("ILAND_GPU_VALIDATE");
    return env && std::strcmp(env, "0") != 0;
}

/// index of a stamp in the GPU stamp table; stamps are registered the first time a tree uses them
int GpuLight::stampIndex(const Stamp *stamp)
{
    auto it = mStampIndex.find(stamp);
    if (it != mStampIndex.end())
        return it->second;
    ilandgpu::StampInfo info;
    info.dataOffset = static_cast<int>(mStampData.size());
    info.dataSize = stamp->dataSize();
    info.offset = stamp->offset();
    const float *data = stamp->data(0, 0);
    mStampData.insert(mStampData.end(), data, data + stamp->dataSize() * stamp->dataSize());
    int index = static_cast<int>(mStampInfo.size());
    mStampInfo.push_back(info);
    mStampIndex[stamp] = index;
    mStampsChanged = true;
    return index;
}

bool GpuLight::applyPattern(Model *model)
{
    const QList<ResourceUnit*> &rus = model->ruList();
    int max_index = 0;
    for (const ResourceUnit *ru : rus)
        max_index = std::max(max_index, ru->index());
    mRuStart.assign(max_index + 1, 0);
    mRuCount.assign(max_index + 1, 0);
    mTrees.clear();
    mTreePtr.clear();

    // collect the trees, resource unit by resource unit
    for (ResourceUnit *ru : rus) {
        mRuStart[ru->index()] = static_cast<int>(mTreePtr.size());
        for (Tree &tree : ru->trees()) {
            if (!tree.mStamp || !tree.mStamp->reader())
                continue; // the CPU code skips such trees too
            ilandgpu::TreeLight t;
            t.x = tree.mPositionIndex.x();
            t.y = tree.mPositionIndex.y();
            t.height = tree.mHeight;
            t.opacity = tree.mOpacity;
            t.writer = stampIndex(tree.mStamp);
            t.reader = stampIndex(tree.mStamp->reader());
            mTrees.push_back(t);
            mTreePtr.push_back(&tree);
        }
        mRuCount[ru->index()] = static_cast<int>(mTreePtr.size()) - mRuStart[ru->index()];
    }

    if (mStampsChanged) {
        const Grid<float> *dist = Stamp::distanceGrid();
        ilandgpu::lightSetStamps(mStampData.data(), static_cast<int>(mStampData.size()),
                                 mStampInfo.data(), static_cast<int>(mStampInfo.size()),
                                 dist->begin(), dist->sizeX(), dist->sizeY());
        mStampsChanged = false;
    }

    FloatGrid *lif = model->grid();
    HeightGrid *hg = model->heightGrid();
    if (mPinned != lif->begin()) {
        // page-lock the light grid once: the GPU copies it back every year
        ilandgpu::pinHostMemory(lif->begin(), sizeof(float) * static_cast<size_t>(lif->count()));
        mPinned = lif->begin();
    }
    mLriRaw.resize(mTrees.size());
    const bool do_validate = validationRequested();
    std::vector<HeightGridValue> height_reset;
    if (do_validate)
        height_reset.assign(hg->begin(), hg->end());
    bool ok = ilandgpu::lightRun(mTrees.data(), static_cast<int>(mTrees.size()),
                                 hg->begin(), hg->sizeX(), hg->sizeY(),
                                 lif->begin(), lif->sizeX(), lif->sizeY(), cPxPerHeight,
                                 mLriRaw.data());
    if (ok && do_validate)
        validate(model, height_reset);
    return ok;
}

/** Check the GPU against the CPU for the current year (slow; for testing only):
  * (a) CPU with the same order as the GPU: complete height grid, then all patterns;
  * (b) CPU in the order of the upstream code: resource unit by resource unit (even, then odd
  *     units, as ThreadRunner), height grid and patterns interleaved.
  * Reports the differences of the light grid, height grid and raw light sums, then restores
  * the GPU results so the simulation continues as without the check. */
void GpuLight::validate(Model *model, const std::vector<HeightGridValue> &height_reset)
{
    FloatGrid *lif = model->grid();
    HeightGrid *hg = model->heightGrid();
    const std::vector<float> lif_gpu(lif->begin(), lif->end());
    const std::vector<HeightGridValue> hg_gpu(hg->begin(), hg->end());

    auto compare_lif = [&](const std::vector<float> &a, double &max_abs, double &mean_abs) {
        max_abs = 0.; double sum = 0.;
        for (size_t i = 0; i < a.size(); ++i) {
            double d = std::fabs(static_cast<double>(a[i]) - (*lif)[static_cast<int>(i)]);
            max_abs = std::max(max_abs, d); sum += d;
        }
        mean_abs = sum / std::max<size_t>(1, a.size());
    };
    auto compare_lri = [&](const std::vector<double> &a, double &max_rel) {
        max_rel = 0.;
        for (size_t i = 0; i < a.size(); ++i) {
            double cpu = mTreePtr[i]->lightSum();
            double d = std::fabs(a[i] - cpu) / std::max(1e-12, std::fabs(cpu));
            max_rel = std::max(max_rel, d);
        }
    };

    // (a) CPU, same order as the GPU
    std::copy(height_reset.begin(), height_reset.end(), hg->begin());
    model->initializeGrid();
    for (Tree *t : mTreePtr) t->heightGrid();
    for (Tree *t : mTreePtr) t->applyLIP();
    int height_diff = 0, count_diff = 0;
    for (int i = 0; i < hg->count(); ++i) {
        if (hg->begin()[i].height != hg_gpu[i].height) ++height_diff;
        if (hg->begin()[i].count() != hg_gpu[i].count() || hg->begin()[i].stemHeight() != hg_gpu[i].stemHeight()) ++count_diff;
    }
    double lif_max, lif_mean, lri_max;
    compare_lif(lif_gpu, lif_max, lif_mean);
    std::vector<double> lri_gpu(mLriRaw.begin(), mLriRaw.end());
    compare_lri(lri_gpu, lri_max);
    const std::vector<float> lif_cpu(lif->begin(), lif->end());
    std::vector<double> lri_cpu(mTreePtr.size());
    for (size_t i = 0; i < mTreePtr.size(); ++i) lri_cpu[i] = mTreePtr[i]->lightSum();

    // (b) CPU, upstream order (interleaved by resource unit); compared with (a)
    std::copy(height_reset.begin(), height_reset.end(), hg->begin());
    model->initializeGrid();
    const QList<ResourceUnit*> &rus = model->ruList();
    for (int pass = 0; pass < 2; ++pass)
        for (int r = pass; r < rus.size(); r += 2) {
            int start = mRuStart[rus[r]->index()], n = mRuCount[rus[r]->index()];
            for (int i = start; i < start + n; ++i) mTreePtr[i]->heightGrid();
            for (int i = start; i < start + n; ++i) mTreePtr[i]->applyLIP();
        }
    double up_max, up_mean, up_lri;
    compare_lif(lif_cpu, up_max, up_mean);
    compare_lri(lri_cpu, up_lri);

    // restore the GPU results
    std::copy(lif_gpu.begin(), lif_gpu.end(), lif->begin());
    std::copy(hg_gpu.begin(), hg_gpu.end(), hg->begin());
    qWarning().noquote() << QString("GPU check: %1 trees | GPU vs CPU (same order): light grid max %2 mean %3, "
                                    "raw LRI max rel %4, height cells differing %5 (count/stem %6) | "
                                    "CPU upstream order vs CPU same order: light grid max %7 mean %8, raw LRI max rel %9")
                            .arg(mTreePtr.size()).arg(lif_max, 0, 'g', 3).arg(lif_mean, 0, 'g', 3).arg(lri_max, 0, 'g', 3)
                            .arg(height_diff).arg(count_diff)
                            .arg(up_max, 0, 'g', 3).arg(up_mean, 0, 'g', 3).arg(up_lri, 0, 'g', 3);
}

void GpuLight::readPattern(ResourceUnit *ru) const
{
    int start = mRuStart[ru->index()], n = mRuCount[ru->index()];
    for (int i = start; i < start + n; ++i)
        mTreePtr[i]->setLightResourceIndex(mLriRaw[i]);
}

#endif // ILAND_CUDA
