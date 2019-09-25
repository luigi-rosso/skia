/*
 * Copyright 2019 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**************************************************************************************************
 *** This file was autogenerated from GrSaturateProcessor.fp; do not modify.
 **************************************************************************************************/
#ifndef GrSaturateProcessor_DEFINED
#define GrSaturateProcessor_DEFINED
#include "include/core/SkTypes.h"

#include "src/gpu/GrCoordTransform.h"
#include "src/gpu/GrFragmentProcessor.h"
class GrSaturateProcessor : public GrFragmentProcessor {
public:
    SkPMColor4f constantOutputForConstantInput(const SkPMColor4f& input) const override {
        return {SkTPin(input.fR, 0.f, 1.f), SkTPin(input.fG, 0.f, 1.f), SkTPin(input.fB, 0.f, 1.f),
                SkTPin(input.fA, 0.f, 1.f)};
    }
    static std::unique_ptr<GrFragmentProcessor> Make() {
        return std::unique_ptr<GrFragmentProcessor>(new GrSaturateProcessor());
    }
    GrSaturateProcessor(const GrSaturateProcessor& src);
    std::unique_ptr<GrFragmentProcessor> clone() const override;
    const char* name() const override { return "SaturateProcessor"; }

private:
    GrSaturateProcessor()
            : INHERITED(kGrSaturateProcessor_ClassID,
                        (OptimizationFlags)kConstantOutputForConstantInput_OptimizationFlag |
                                kPreservesOpaqueInput_OptimizationFlag) {}
    GrGLSLFragmentProcessor* onCreateGLSLInstance() const override;
    void onGetGLSLProcessorKey(const GrShaderCaps&, GrProcessorKeyBuilder*) const override;
    bool onIsEqual(const GrFragmentProcessor&) const override;
    GR_DECLARE_FRAGMENT_PROCESSOR_TEST
    typedef GrFragmentProcessor INHERITED;
};
#endif
