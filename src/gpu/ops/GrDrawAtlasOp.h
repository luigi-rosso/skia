/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GrDrawAtlasOp_DEFINED
#define GrDrawAtlasOp_DEFINED

#include "GrColor.h"
#include "GrDefaultGeoProcFactory.h"
#include "GrMeshDrawOp.h"
#include "GrSimpleMeshDrawOpHelper.h"

class GrDrawAtlasOp final : public GrMeshDrawOp {
private:
    using Helper = GrSimpleMeshDrawOpHelper;

public:
    DEFINE_OP_CLASS_ID

    static std::unique_ptr<GrDrawOp> Make(GrContext* context,
                                          GrPaint&& paint,
                                          const SkMatrix& viewMatrix,
                                          GrAAType aaType,
                                          int spriteCount,
                                          const SkRSXform* xforms,
                                          const SkRect* rects,
                                          const SkColor* colors) {
        return Helper::FactoryHelper<GrDrawAtlasOp>(context, std::move(paint), viewMatrix, aaType,
                                                    spriteCount, xforms, rects, colors);
    }

    GrDrawAtlasOp(const Helper::MakeArgs& helperArgs, const SkPMColor4f& color,
                  const SkMatrix& viewMatrix, GrAAType, int spriteCount, const SkRSXform* xforms,
                  const SkRect* rects, const SkColor* colors);

    const char* name() const override { return "DrawAtlasOp"; }

    void visitProxies(const VisitProxyFunc& func, VisitorType) const override {
        fHelper.visitProxies(func);
    }

#ifdef SK_DEBUG
    SkString dumpInfo() const override;
#endif

    FixedFunctionFlags fixedFunctionFlags() const override;

    GrProcessorSet::Analysis finalize(const GrCaps& caps, const GrAppliedClip* clip) override;

private:
    void onPrepareDraws(Target*) override;

    const SkPMColor4f& color() const { return fColor; }
    const SkMatrix& viewMatrix() const { return fViewMatrix; }
    bool hasColors() const { return fHasColors; }
    int quadCount() const { return fQuadCount; }

    CombineResult onCombineIfPossible(GrOp* t, const GrCaps&) override;

    struct Geometry {
        SkPMColor4f fColor;
        SkTArray<uint8_t, true> fVerts;
    };

    SkSTArray<1, Geometry, true> fGeoData;
    Helper fHelper;
    SkMatrix fViewMatrix;
    SkPMColor4f fColor;
    int fQuadCount;
    bool fHasColors;

    typedef GrMeshDrawOp INHERITED;
};

#endif
