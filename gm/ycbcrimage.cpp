/*
 * Copyright 2020 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gm/gm.h"

// This test only works with the Vulkan backend.
#ifdef SK_VULKAN

#include "include/core/SkCanvas.h"
#include "include/core/SkImage.h"
#include "include/core/SkPaint.h"
#include "include/core/SkSize.h"
#include "include/core/SkString.h"
#include "include/gpu/GrContext.h"
#include "src/gpu/GrContextPriv.h"
#include "tools/gpu/vk/VkYcbcrSamplerHelper.h"

static void release_ycbcrhelper(void* releaseContext) {
    VkYcbcrSamplerHelper* ycbcrHelper = reinterpret_cast<VkYcbcrSamplerHelper*>(releaseContext);
    delete ycbcrHelper;
}

namespace skiagm {

// This GM exercises the native YCbCr image format on Vulkan
class YCbCrImageGM : public GpuGM {
public:
    YCbCrImageGM() {
        this->setBGColor(0xFFCCCCCC);
    }

protected:

    SkString onShortName() override {
        return SkString("ycbcrimage");
    }

    SkISize onISize() override {
        return SkISize::Make(2*kPad+kImageSize, 2*kPad+kImageSize);
    }

    DrawResult createYCbCrImage(GrContext* context, SkString* errorMsg) {
        std::unique_ptr<VkYcbcrSamplerHelper> ycbcrHelper(new VkYcbcrSamplerHelper(context));

        if (!ycbcrHelper->isYCbCrSupported()) {
            *errorMsg = "YCbCr sampling not supported.";
            return skiagm::DrawResult::kSkip;
        }

        if (!ycbcrHelper->createBackendTexture(kImageSize, kImageSize)) {
            *errorMsg = "Failed to create I420 backend texture.";
            return skiagm::DrawResult::kFail;
        }

        SkASSERT(!fYCbCrImage);
        fYCbCrImage = SkImage::MakeFromTexture(context, ycbcrHelper->backendTexture(),
                                               kTopLeft_GrSurfaceOrigin, kRGB_888x_SkColorType,
                                               kPremul_SkAlphaType, nullptr,
                                               release_ycbcrhelper, ycbcrHelper.get());
        if (!fYCbCrImage) {
            *errorMsg = "Failed to create I420 image.";
            return DrawResult::kFail;
        }

        ycbcrHelper.release();
        return DrawResult::kOk;
    }

    DrawResult onGpuSetup(GrContext* context, SkString* errorMsg) override {
        if (!context || context->abandoned()) {
            return DrawResult::kSkip;
        }

        SkASSERT(context->priv().asDirectContext());

        if (context->backend() != GrBackendApi::kVulkan) {
            *errorMsg = "This GM requires a Vulkan context.";
            return DrawResult::kSkip;
        }

        DrawResult result = this->createYCbCrImage(context, errorMsg);
        if (result != DrawResult::kOk) {
            return result;
        }

        return DrawResult::kOk;
    }

    DrawResult onDraw(GrContext*, GrRenderTargetContext*, SkCanvas* canvas,  SkString*) override {
        SkASSERT(fYCbCrImage);

        SkPaint paint;
        paint.setFilterQuality(kLow_SkFilterQuality);

        canvas->drawImage(fYCbCrImage, kPad, kPad, &paint);
        return DrawResult::kOk;
    }

private:
    static const int kImageSize = 112;
    static const int kPad = 8;

    sk_sp<SkImage> fYCbCrImage;

    typedef GpuGM INHERITED;
};

//////////////////////////////////////////////////////////////////////////////

DEF_GM(return new YCbCrImageGM;)

} // skiagm

#endif // SK_VULKAN
