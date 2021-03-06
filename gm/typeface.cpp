/*
 * Copyright 2012 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gm.h"
#include "sk_tool_utils.h"
#include "Resources.h"
#include "SkBlurTypes.h"
#include "SkCanvas.h"
#include "SkFontStyle.h"
#include "SkMaskFilter.h"
#include "SkString.h"
#include "SkSurfaceProps.h"
#include "SkTextBlob.h"
#include "SkTypeface.h"
#include "SkTypes.h"

static void getGlyphPositions(const SkFont& font, const uint16_t glyphs[],
                             int count, SkScalar x, SkScalar y, SkPoint pos[]) {
    SkAutoSTMalloc<128, SkScalar> widthStorage(count);
    SkScalar* widths = widthStorage.get();
    font.getWidths(glyphs, count, widths);

    for (int i = 0; i < count; ++i) {
        pos[i].set(x, y);
        x += widths[i];
    }
}

static void applyKerning(SkPoint pos[], const int32_t adjustments[], int count,
                         const SkFont& font) {
    SkScalar scale = font.getSize() / font.getTypeface()->getUnitsPerEm();

    SkScalar globalAdj = 0;
    for (int i = 0; i < count - 1; ++i) {
        globalAdj += adjustments[i] * scale;
        pos[i + 1].fX += globalAdj;
    }
}

static void drawKernText(SkCanvas* canvas, const void* text, size_t len,
                         SkScalar x, SkScalar y, const SkFont& font, const SkPaint& paint) {
    SkTypeface* face = font.getTypeface();
    if (!face) {
        canvas->drawSimpleText(text, len, kUTF8_SkTextEncoding, x, y, font, paint);
        return;
    }

    SkAutoSTMalloc<128, uint16_t> glyphStorage(len);
    uint16_t* glyphs = glyphStorage.get();
    int glyphCount = font.textToGlyphs(text, len, kUTF8_SkTextEncoding, glyphs, len);
    if (glyphCount < 1) {
        return;
    }

    SkAutoSTMalloc<128, int32_t> adjustmentStorage(glyphCount - 1);
    int32_t* adjustments = adjustmentStorage.get();
    if (!face->getKerningPairAdjustments(glyphs, glyphCount, adjustments)) {
        canvas->drawSimpleText(text, len, kUTF8_SkTextEncoding, x, y, font, paint);
        return;
    }


    SkTextBlobBuilder builder;
    auto rec = builder.allocRunPos(font, glyphCount);
    memcpy(rec.glyphs, glyphs, glyphCount * sizeof(SkGlyphID));
    getGlyphPositions(font, glyphs, glyphCount, x, y, rec.points());
    applyKerning(rec.points(), adjustments, glyphCount, font);

    canvas->drawTextBlob(builder.make(), 0, 0, paint);
}

static constexpr SkFontStyle gStyles[] = {
    SkFontStyle::Normal(),
    SkFontStyle::Bold(),
    SkFontStyle::Italic(),
    SkFontStyle::BoldItalic(),
};

constexpr int gStylesCount = SK_ARRAY_COUNT(gStyles);

class TypefaceStylesGM : public skiagm::GM {
    sk_sp<SkTypeface> fFaces[gStylesCount];
    bool fApplyKerning;

public:
    TypefaceStylesGM(bool applyKerning) : fApplyKerning(applyKerning) {}

protected:
    void onOnceBeforeDraw() override {
        for (int i = 0; i < gStylesCount; i++) {
            fFaces[i] = SkTypeface::MakeFromName(nullptr, gStyles[i]);
        }
    }

    SkString onShortName() override {
        SkString name("typefacestyles");
        if (fApplyKerning) {
            name.append("_kerning");
        }
        name.append(sk_tool_utils::platform_font_manager());
        return name;
    }

    SkISize onISize() override {
        return SkISize::Make(640, 480);
    }

    void onDraw(SkCanvas* canvas) override {
        SkFont font;
        font.setSize(30);

        const char* text = fApplyKerning ? "Type AWAY" : "Hamburgefons";
        const size_t textLen = strlen(text);

        SkScalar x = SkIntToScalar(10);
        SkScalar dy = font.getMetrics(nullptr);
        SkScalar y = dy;

        if (fApplyKerning) {
            font.setSubpixel(true);
        } else {
            font.setLinearMetrics(true);
        }

        SkPaint paint;
        for (int i = 0; i < gStylesCount; i++) {
            font.setTypeface(fFaces[i]);
            canvas->drawSimpleText(text, textLen, kUTF8_SkTextEncoding, x, y, font, paint);
            if (fApplyKerning) {
                drawKernText(canvas, text, textLen, x + 240, y, font, paint);
            }
            y += dy;
        }
    }

private:
    typedef skiagm::GM INHERITED;
};

DEF_GM( return new TypefaceStylesGM(false); )
DEF_GM( return new TypefaceStylesGM(true); )

////////////////////////////////////////////////////////////////////////////////

static void draw_typeface_rendering_gm(SkCanvas* canvas, sk_sp<SkTypeface> face,
                                       char character = 'A') {
    struct AliasType {
        bool antiAlias;
        bool subpixelAntitalias;
        bool inLayer;
    } constexpr aliasTypes[] {
#ifndef SK_BUILD_FOR_IOS
        // This gm crashes on iOS when drawing an embedded bitmap when requesting aliased rendering.
        // The crash looks like
        //   libTrueTypeScaler.dylib`<redacted> + 80
        //   stop reason = EXC_BAD_ACCESS (code=EXC_ARM_DA_ALIGN, address=...)
        //   ->  0x330b19d0 <+80>: strd   r2, r3, [r5, #36]
        //       0x330b19d4 <+84>: movs   r3, #0x0
        //       0x330b19d6 <+86>: add    r2, sp, #0x28
        //       0x330b19d8 <+88>: ldr    r0, [r4, #0x4]
        // Disable testing embedded bitmaps on iOS for now.
        // See https://bug.skia.org/5530 .
        { false, false, false },  // aliased
#endif
        { true,  false, false },  // anti-aliased
        { true,  true , false },  // subpixel anti-aliased
        { true,  false, true  },  // anti-aliased in layer (flat pixel geometry)
        { true,  true , true  },  // subpixel anti-aliased in layer (flat pixel geometry)
    };

    auto compute_edging = [](AliasType at) {
        if (at.antiAlias) {
            if (at.subpixelAntitalias) {
                return SkFont::Edging::kSubpixelAntiAlias;
            } else {
                return SkFont::Edging::kAntiAlias;
            }
        } else {
            return SkFont::Edging::kAlias;
        }
    };

    // The hintgasp.ttf is designed for the following sizes to be different.
    // GASP_DOGRAY                                      0x0002   0<=ppem<=10
    // GASP_SYMMETRIC_SMOOTHING                         0x0008   0<=ppem<=10
    // GASP_GRIDFIT                                     0x0001  11<=ppem<=12
    // GASP_SYMMETRIC_GRIDFIT                           0x0004  11<=ppem<=12
    // GASP_DOGRAY|GASP_GRIDFIT                         0x0003  13<=ppem<=14
    // GASP_SYMMETRIC_SMOOTHING|GASP_SYMMETRIC_GRIDFIT  0x000C  13<=ppem<=14
    // (neither)                                        0x0000  15<=ppem
    // Odd sizes have embedded bitmaps.
    constexpr SkScalar textSizes[] = { 9, 10, 11, 12, 13, 14, 15, 16 };

    constexpr SkFontHinting hintingTypes[] = {
        kNo_SkFontHinting,
        kSlight_SkFontHinting,
        kNormal_SkFontHinting,
        kFull_SkFontHinting
    };

    struct SubpixelType {
        bool requested;
        SkVector offset;
    } constexpr subpixelTypes[] = {
        { false, { 0.00, 0.00 } },
        { true , { 0.00, 0.00 } },
        { true , { 0.25, 0.00 } },
        { true , { 0.25, 0.25 } },
    };

    constexpr bool rotateABitTypes[] = { false, true };

    SkScalar y = 0;  // The baseline of the previous output
    {
        SkPaint paint;

        SkFont font(face);
        font.setEmbeddedBitmaps(true);

        SkScalar x = 0;
        SkScalar xMax = x;
        SkScalar xBase = 0;
        for (const SubpixelType subpixel : subpixelTypes) {
            y = 0;
            font.setSubpixel(subpixel.requested);

            for (const AliasType& alias : aliasTypes) {
                font.setEdging(compute_edging(alias));
                SkAutoCanvasRestore acr(canvas, false);
                if (alias.inLayer) {
                    canvas->saveLayer(nullptr, &paint);
                }

                for (const SkScalar& textSize : textSizes) {
                    x = xBase + 5;
                    font.setSize(textSize);

                    SkScalar dy = SkScalarCeilToScalar(font.getMetrics(nullptr));
                    y += dy;
                    for (const SkFontHinting& hinting : hintingTypes) {
                        font.setHinting(hinting);

                        for (const bool& rotateABit : rotateABitTypes) {
                            SkAutoCanvasRestore acr(canvas, true);
                            if (rotateABit) {
                                canvas->rotate(2, x + subpixel.offset.x(),
                                                  y + subpixel.offset.y());
                            }
                            canvas->drawSimpleText(&character, 1, kUTF8_SkTextEncoding,
                                                   x + subpixel.offset.x(),
                                                   y + subpixel.offset.y(), font, paint);

                            SkScalar dx = SkScalarCeilToScalar(
                                    font.measureText(&character, 1, kUTF8_SkTextEncoding)) + 5;
                            x += dx;
                            xMax = SkTMax(x, xMax);
                        }
                    }
                }
                y += 10;
            }
            xBase = xMax;
        }
    }

    constexpr struct StyleTests {
        SkPaint::Style style;
        SkScalar strokeWidth;
    } styleTypes[] = {
        { SkPaint::kFill_Style, 0.0f},
        { SkPaint::kStroke_Style, 0.0f},
        { SkPaint::kStroke_Style, 0.5f},
        { SkPaint::kStrokeAndFill_Style, 1.0f},
    };

    constexpr bool fakeBoldTypes[] = { false, true };

    {
        SkPaint paint;

        SkFont font(face, 16);

        SkScalar x = 0;
        for (const bool& fakeBold : fakeBoldTypes) {
            SkScalar dy = SkScalarCeilToScalar(font.getMetrics(nullptr));
            y += dy;
            x = 5;

            font.setEmbolden(fakeBold);
            for (const AliasType& alias : aliasTypes) {
                font.setEdging(compute_edging(alias));
                SkAutoCanvasRestore acr(canvas, false);
                if (alias.inLayer) {
                    canvas->saveLayer(nullptr, &paint);
                }
                for (const StyleTests& style : styleTypes) {
                    paint.setStyle(style.style);
                    paint.setStrokeWidth(style.strokeWidth);
                    canvas->drawSimpleText(&character, 1, kUTF8_SkTextEncoding, x, y, font, paint);

                    SkScalar dx = SkScalarCeilToScalar(font.measureText(&character, 1,
                                                                        kUTF8_SkTextEncoding)) + 5;
                    x += dx;
                }
            }
            y += 10;
        }
    }

    constexpr struct MaskTests {
        SkBlurStyle style;
        SkScalar sigma;
    } maskTypes[] = {
        { SkBlurStyle::kNormal_SkBlurStyle, 0.0f},
        { SkBlurStyle::kSolid_SkBlurStyle, 0.0f},
        { SkBlurStyle::kOuter_SkBlurStyle, 0.0f},
        { SkBlurStyle::kInner_SkBlurStyle, 0.0f},

        { SkBlurStyle::kNormal_SkBlurStyle, 0.5f},
        { SkBlurStyle::kSolid_SkBlurStyle, 0.5f},
        { SkBlurStyle::kOuter_SkBlurStyle, 0.5f},
        { SkBlurStyle::kInner_SkBlurStyle, 0.5f},

        { SkBlurStyle::kNormal_SkBlurStyle, 2.0f},
        { SkBlurStyle::kSolid_SkBlurStyle, 2.0f},
        { SkBlurStyle::kOuter_SkBlurStyle, 2.0f},
        { SkBlurStyle::kInner_SkBlurStyle, 2.0f},
    };

    {
        SkPaint paint;

        SkFont font(face, 16);

        SkScalar x = 0;
        {
            for (const AliasType& alias : aliasTypes) {
                SkScalar dy = SkScalarCeilToScalar(font.getMetrics(nullptr));
                y += dy;
                x = 5;

                font.setEdging(compute_edging(alias));
                SkAutoCanvasRestore acr(canvas, false);
                if (alias.inLayer) {
                    canvas->saveLayer(nullptr, &paint);
                }
                for (const MaskTests& mask : maskTypes) {
                    paint.setMaskFilter(SkMaskFilter::MakeBlur(mask.style, mask.sigma));
                    canvas->drawSimpleText(&character, 1, kUTF8_SkTextEncoding, x, y, font, paint);

                    SkScalar dx = SkScalarCeilToScalar(font.measureText(&character, 1,
                                                                        kUTF8_SkTextEncoding)) + 5;
                    x += dx;
                }
                paint.setMaskFilter(nullptr);
            }
            y += 10;
        }
    }
}

DEF_SIMPLE_GM_BG_NAME(typefacerendering, canvas, 640, 840, SK_ColorWHITE,
                      SkStringPrintf("typefacerendering%s",
                                     sk_tool_utils::platform_font_manager())) {
    if (sk_sp<SkTypeface> face = MakeResourceAsTypeface("fonts/hintgasp.ttf")) {
        draw_typeface_rendering_gm(canvas, std::move(face));
    }
}

// Type1 fonts don't currently work in Skia on Windows.
#ifndef SK_BUILD_FOR_WIN

DEF_SIMPLE_GM_BG_NAME(typefacerendering_pfa, canvas, 640, 840, SK_ColorWHITE,
                      SkStringPrintf("typefacerendering_pfa%s",
                                     sk_tool_utils::platform_font_manager())) {
    if (sk_sp<SkTypeface> face = MakeResourceAsTypeface("fonts/Roboto2-Regular.pfa")) {
        // This subsetted typeface doesn't have the character 'A'.
        draw_typeface_rendering_gm(canvas, std::move(face), 'O');
    }
}

DEF_SIMPLE_GM_BG_NAME(typefacerendering_pfb, canvas, 640, 840, SK_ColorWHITE,
                      SkStringPrintf("typefacerendering_pfb%s",
                                     sk_tool_utils::platform_font_manager())) {
    if (sk_sp<SkTypeface> face = MakeResourceAsTypeface("fonts/Roboto2-Regular.pfb")) {
        draw_typeface_rendering_gm(canvas, std::move(face), 'O');
    }
}

#endif
