/*
 * Copyright (c) 2019, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.

 *
 */

//!
//! \file     RenderManager.cpp
//! \brief    Implement class for RenderManager.
//!

#include "RenderManager.h"
#include <chrono>
#include "CubeMapRender.h"
#include "CubeMapRenderTarget.h"
#include "EGLRenderContext.h"
#include "ERPRender.h"
#include "ERPRenderTarget.h"
#include "GLFWRenderContext.h"

VCD_NS_BEGIN

RenderManager::RenderManager(struct RenderConfig config) {
  this->m_renderConfig = config;
  this->m_mediaSource = nullptr;
  this->m_renderTarget = nullptr;
  this->m_rsFactory = nullptr;
  this->m_viewPortManager = nullptr;
  this->m_renderContext = nullptr;
  m_surfaceRender = nullptr;
}

RenderManager::~RenderManager() {
  SAFE_DELETE(m_renderTarget);
  SAFE_DELETE(m_surfaceRender);
  SAFE_DELETE(m_viewPortManager);
  SAFE_DELETE(m_renderContext);
}

RenderStatus RenderManager::Render(int64_t pts) {
  uint32_t width = m_renderConfig.windowWidth;
  uint32_t height = m_renderConfig.windowHeight;
  std::chrono::high_resolution_clock clock;
  uint64_t start1 = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count();
  if (RENDER_STATUS_OK != m_mediaSource->UpdateFrames(pts)) {
    LOG(INFO) << "UpdateFrames error!" << endl;
    return RENDER_NO_FRAME;
  }
  uint64_t end1 = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count();
  LOG(INFO) << "UpdateFrames cost time:" << (end1 - start1) << endl;
  // 3.tile copy and render to FBO from m_renderTarget
  float yaw = 0;
  float pitch = 0;
  GetViewport(&yaw, &pitch);
  uint64_t start2 = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count();
  RenderStatus renderTargetStatus =
      m_renderTarget->Update(yaw, pitch, m_renderConfig.viewportHFOV, m_renderConfig.viewportVFOV);
  if (RENDER_ERROR == renderTargetStatus) {
    LOG(INFO) << "Update error!" << endl;
    return RENDER_ERROR;
  }
  uint64_t end2 = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count();
  LOG(INFO) << "Update cost time:" << (end2 - start2) << endl;
  m_surfaceRender->SetTransformTypeToMesh(m_renderTarget->GetTransformType());
  uint64_t start3 = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count();
  if (RENDER_STATUS_OK != m_surfaceRender->Render(m_renderTarget->GetTextureOfR2S(), width, height,
                                                  m_renderContext->GetProjectionMatrix(),
                                                  m_renderContext->GetViewModelMatrix())) {
    LOG(INFO) << "Render error!" << endl;
    return RENDER_ERROR;
  }
  uint64_t end3 = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now().time_since_epoch()).count();
  LOG(INFO) << "Render cost time:" << (end3 - start3) << endl;
  return RENDER_STATUS_OK;
}

RenderStatus RenderManager::Initialize(MediaSource *source, RenderSourceFactory *rsFactory,
                                       RenderContext *context)  // should input the decoderManager
{
  if (m_renderConfig.url.empty()) {
    LOG(ERROR) << "Source URL is empty!" << std::endl;
    return RENDER_ERROR;
  }

  this->m_renderContext = context;

  RenderStatus ret = RENDER_STATUS_OK;
  // initial ViewPortManager
  m_viewPortManager = new ViewPortManager();
  m_mediaSource = source;
  MediaInfo mediaInfo = m_mediaSource->GetMediaInfo();
  VideoInfo vi;
  mediaInfo.GetActiveVideoInfo(vi);

  m_renderConfig.projFormat = vi.mProjFormat;
  m_renderConfig.renderInterval = 1000 * vi.framerate_den / vi.framerate_num;
  // initial SurfaceRender and shaders
  if (CreateRender(m_renderConfig.projFormat) != RENDER_STATUS_OK) {
    return RENDER_ERROR;
  }
  // set uniform frameTex
  m_surfaceRender->SetUniformFrameTex();

  // initial renderTarget
  if (CreateRenderTarget(m_renderConfig.projFormat) != RENDER_STATUS_OK) {
    return RENDER_ERROR;
  }
  m_rsFactory = rsFactory;
  ret = m_renderTarget->Initialize(m_rsFactory);
  if (RENDER_STATUS_OK != ret) {
    LOG(ERROR) << "failed to initial render target!" << std::endl;
    return ret;
  }
  if (m_renderTarget->CreateRenderTarget() != RENDER_STATUS_OK) {
    return RENDER_ERROR;
  }

  return RENDER_STATUS_OK;
}

RenderStatus RenderManager::CreateRenderTarget(int32_t projFormat) {
  switch (projFormat) {
    case VCD::OMAF::PF_ERP:
    {
      m_renderTarget = new ERPRenderTarget();
      if (nullptr == m_renderTarget) {
        LOG(ERROR) << "ERP render target creation failed" << std::endl;
        return RENDER_ERROR;
      }
      break;
    }
    case VCD::OMAF::PF_CUBEMAP:
    {
      m_renderTarget = new CubeMapRenderTarget();
      if (nullptr == m_renderTarget) {
        LOG(ERROR) << "CubeMap render target creation failed" << std::endl;
        return RENDER_ERROR;
      }
      break;
    }

    default:
      return RENDER_ERROR;
  }

  return RENDER_STATUS_OK;
}

RenderStatus RenderManager::CreateRender(int32_t projFormat) {
  switch (projFormat) {
    case VCD::OMAF::PF_ERP:
    {
      m_surfaceRender = new ERPRender();
      if (nullptr == m_surfaceRender) {
        LOG(ERROR) << "ERPRender creation failed" << std::endl;
        return RENDER_ERROR;
      }
      break;
    }
    case VCD::OMAF::PF_CUBEMAP:
    {
      m_surfaceRender = new CubeMapRender();
      if (nullptr == m_surfaceRender) {
        LOG(ERROR) << "CubeMapRender creation failed" << std::endl;
        return RENDER_ERROR;
      }
      break;
    }

    default:
      return RENDER_ERROR;
  }

  return RENDER_STATUS_OK;
}

bool RenderManager::IsEOS() { return m_mediaSource->IsEOS() || !(m_renderContext->isRunning()); }

RenderStatus RenderManager::ChangeViewport(float yaw, float pitch) {
  m_mediaSource->ChangeViewport(yaw, pitch);
  return RENDER_STATUS_OK;
}

RenderStatus RenderManager::SetViewport(float yaw, float pitch) {
  struct Pose pose;
  pose.yaw = yaw;
  pose.pitch = pitch;
  ScopeLock lock(m_poseLock);
  m_viewPortManager->SetViewPort(pose);

  return RENDER_STATUS_OK;
}

RenderStatus RenderManager::GetViewport(float *yaw, float *pitch) {
  ScopeLock lock(m_poseLock);
  struct Pose pose = m_viewPortManager->GetViewPort();
  *yaw = pose.yaw;
  *pitch = pose.pitch;
  return RENDER_STATUS_OK;
}

struct RenderConfig RenderManager::GetRenderConfig() {
  return m_renderConfig;
}

void RenderManager::GetStatusAndPose(float *yaw, float *pitch, uint32_t *status) {
  m_renderContext->GetStatusAndPose(yaw, pitch, status);
}

VCD_NS_END
