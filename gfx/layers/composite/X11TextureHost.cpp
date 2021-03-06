/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "X11TextureHost.h"
#include "mozilla/layers/BasicCompositor.h"
#include "mozilla/layers/X11TextureSourceBasic.h"
#ifdef GL_PROVIDER_GLX
#include "mozilla/layers/CompositorOGL.h"
#include "mozilla/layers/X11TextureSourceOGL.h"
#endif
#include "gfxXlibSurface.h"
#include "gfx2DGlue.h"

using namespace mozilla;
using namespace mozilla::layers;
using namespace mozilla::gfx;

X11TextureHost::X11TextureHost(TextureFlags aFlags,
                               const SurfaceDescriptorX11& aDescriptor)
 : TextureHost(aFlags)
{
  nsRefPtr<gfxXlibSurface> surface = aDescriptor.OpenForeign();
  mSurface = surface.get();

  // The host always frees the pixmap.
  MOZ_ASSERT(!(aFlags & TEXTURE_DEALLOCATE_CLIENT));
  mSurface->TakePixmap();
}

bool
X11TextureHost::Lock()
{
  if (!mCompositor) {
    return false;
  }

  if (!mTextureSource) {
    switch (mCompositor->GetBackendType()) {
      case LayersBackend::LAYERS_BASIC:
        mTextureSource =
          new X11TextureSourceBasic(static_cast<BasicCompositor*>(mCompositor),
                                    mSurface);
        break;
#ifdef GL_PROVIDER_GLX
      case LayersBackend::LAYERS_OPENGL:
        mTextureSource =
          new X11TextureSourceOGL(static_cast<CompositorOGL*>(mCompositor),
                                  mSurface);
        break;
#endif
      default:
        return false;
    }
  }

  return true;
}

void
X11TextureHost::SetCompositor(Compositor* aCompositor)
{
  mCompositor = aCompositor;
  if (mTextureSource) {
    mTextureSource->SetCompositor(aCompositor);
  }
}

SurfaceFormat
X11TextureHost::GetFormat() const
{
  gfxContentType type = mSurface->GetContentType();
#ifdef GL_PROVIDER_GLX
  if (mCompositor->GetBackendType() == LayersBackend::LAYERS_OPENGL) {
    return X11TextureSourceOGL::ContentTypeToSurfaceFormat(type);
  }
#endif
  return X11TextureSourceBasic::ContentTypeToSurfaceFormat(type);
}

IntSize
X11TextureHost::GetSize() const
{
  return ToIntSize(mSurface->GetSize());
}
