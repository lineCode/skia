/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gm.h"
#include "SkBlurImageFilter.h"
#include "SkColorMatrixFilter.h"
#include "SkImage.h"
#include "SkImageFilter.h"
#include "SkSurface.h"

/**
 *  Test drawing a primitive w/ an imagefilter (in this case, just matrix w/ identity) to see
 *  that we apply the xfermode *after* the image has been created and filtered, and not during
 *  the creation step (i.e. before it is filtered).
 *
 *  see https://bug.skia.org/3741
 */
static void do_draw(SkCanvas* canvas, SkXfermode::Mode mode, SkImageFilter* imf) {
        SkAutoCanvasRestore acr(canvas, true);
        canvas->clipRect(SkRect::MakeWH(220, 220));

        // want to force a layer, so modes like DstIn can combine meaningfully, but the final
        // image can still be shown against our default (opaque) background. non-opaque GMs
        // are a lot more trouble to compare/triage.
        canvas->saveLayer(nullptr, nullptr);
        canvas->drawColor(SK_ColorGREEN);

        SkPaint paint;
        paint.setAntiAlias(true);

        SkRect r0 = SkRect::MakeXYWH(10, 60, 200, 100);
        SkRect r1 = SkRect::MakeXYWH(60, 10, 100, 200);

        paint.setColor(SK_ColorRED);
        canvas->drawOval(r0, paint);

        paint.setColor(0x660000FF);
        paint.setImageFilter(imf);
        paint.setXfermodeMode(mode);
        canvas->drawOval(r1, paint);
}

DEF_SIMPLE_GM(imagefilters_xfermodes, canvas, 480, 480) {
        canvas->translate(10, 10);

        // just need an imagefilter to trigger the code-path (which creates a tmp layer)
        SkAutoTUnref<SkImageFilter> imf(SkImageFilter::CreateMatrixFilter(SkMatrix::I(),
                                                                          kNone_SkFilterQuality));

        const SkXfermode::Mode modes[] = {
            SkXfermode::kSrcATop_Mode, SkXfermode::kDstIn_Mode
        };

        for (size_t i = 0; i < SK_ARRAY_COUNT(modes); ++i) {
            canvas->save();
            do_draw(canvas, modes[i], nullptr);
            canvas->translate(240, 0);
            do_draw(canvas, modes[i], imf);
            canvas->restore();

            canvas->translate(0, 240);
        }
}

static SkImage* make_image(SkCanvas* canvas) {
    const SkImageInfo info = SkImageInfo::MakeN32Premul(100, 100);
    SkAutoTUnref<SkSurface> surface(canvas->newSurface(info));
    if (!surface) {
        surface.reset(SkSurface::NewRaster(info));
    }
    surface->getCanvas()->drawRect(SkRect::MakeXYWH(25, 25, 50, 50), SkPaint());
    return surface->newImageSnapshot();
}

// Compare blurs when we're tightly clipped (fast) and not as tightly (slower)
//
// Expect the two to draw the same (modulo the extra border of pixels when the clip is larger)
//
DEF_SIMPLE_GM(fast_slow_blurimagefilter, canvas, 620, 260) {
    SkAutoTUnref<SkImage> image(make_image(canvas));
    const SkRect r = SkRect::MakeIWH(image->width(), image->height());

    canvas->translate(10, 10);
    for (SkScalar sigma = 8; sigma <= 128; sigma *= 2) {
        SkPaint paint;
        SkAutoTUnref<SkImageFilter> blur(SkBlurImageFilter::Create(sigma, sigma));
        paint.setImageFilter(blur);

        canvas->save();
        // we outset the clip by 1, to fall out of the fast-case in drawImage
        // i.e. the clip is larger than the image
        for (SkScalar outset = 0; outset <= 1; ++outset) {
            canvas->save();
            canvas->clipRect(r.makeOutset(outset, outset));
            canvas->drawImage(image, 0, 0, &paint);
            canvas->restore();
            canvas->translate(0, r.height() + 20);
        }
        canvas->restore();
        canvas->translate(r.width() + 20, 0);
    }
}
