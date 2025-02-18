// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/playground/backend/gles/playground_impl_gles.h"

#define GLFW_INCLUDE_NONE
#import "third_party/glfw/include/GLFW/glfw3.h"

#include "flutter/fml/build_config.h"
#include "impeller/entity/gles/entity_shaders_gles.h"
#include "impeller/fixtures/gles/fixtures_shaders_gles.h"
#include "impeller/playground/imgui/gles/imgui_shaders_gles.h"
#include "impeller/renderer/backend/gles/context_gles.h"
#include "impeller/renderer/backend/gles/surface_gles.h"

namespace impeller {

void PlaygroundImplGLES::DestroyWindowHandle(WindowHandle handle) {
  if (!handle) {
    return;
  }
  ::glfwDestroyWindow(reinterpret_cast<GLFWwindow*>(handle));
}

PlaygroundImplGLES::PlaygroundImplGLES()
    : handle_(nullptr, &DestroyWindowHandle) {
  ::glfwDefaultWindowHints();

#if FML_OS_MACOSX
  // ES Profiles are not supported on Mac.
  ::glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
#else   // FML_OS_MACOSX
  ::glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  ::glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  ::glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif  // FML_OS_MACOSX
  ::glfwWindowHint(GLFW_RED_BITS, 8);
  ::glfwWindowHint(GLFW_GREEN_BITS, 8);
  ::glfwWindowHint(GLFW_BLUE_BITS, 8);
  ::glfwWindowHint(GLFW_ALPHA_BITS, 8);
  ::glfwWindowHint(GLFW_DEPTH_BITS, 0);    // no depth buffer
  ::glfwWindowHint(GLFW_STENCIL_BITS, 8);  // 8 bit stencil buffer
  ::glfwWindowHint(GLFW_SAMPLES, 4);       // 4xMSAA

  ::glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

  auto window = ::glfwCreateWindow(1, 1, "Test", nullptr, nullptr);

  ::glfwMakeContextCurrent(window);

  handle_.reset(window);
}

PlaygroundImplGLES::~PlaygroundImplGLES() = default;

static std::vector<std::shared_ptr<fml::Mapping>>
ShaderLibraryMappingsForPlayground() {
  return {
      std::make_shared<fml::NonOwnedMapping>(
          impeller_entity_shaders_gles_data,
          impeller_entity_shaders_gles_length),
      std::make_shared<fml::NonOwnedMapping>(
          impeller_fixtures_shaders_gles_data,
          impeller_fixtures_shaders_gles_length),
      std::make_shared<fml::NonOwnedMapping>(
          impeller_imgui_shaders_gles_data, impeller_imgui_shaders_gles_length),
  };
}

// |PlaygroundImpl|
std::shared_ptr<Context> PlaygroundImplGLES::GetContext() const {
  auto resolver = [](const char* name) -> void* {
    return reinterpret_cast<void*>(::glfwGetProcAddress(name));
  };
  auto gl = std::make_unique<ProcTableGLES>(resolver);
  if (!gl->IsValid()) {
    FML_LOG(ERROR) << "Proc table when creating a playground was invalid.";
    return nullptr;
  }

  return ContextGLES::Create(std::move(gl),
                             ShaderLibraryMappingsForPlayground());
}

// |PlaygroundImpl|
PlaygroundImpl::WindowHandle PlaygroundImplGLES::GetWindowHandle() {
  return handle_.get();
}

// |PlaygroundImpl|
std::unique_ptr<Surface> PlaygroundImplGLES::AcquireSurfaceFrame(
    std::shared_ptr<Context> context) {
  auto window = reinterpret_cast<GLFWwindow*>(GetWindowHandle());
  int width = 0;
  int height = 0;
  ::glfwGetFramebufferSize(window, &width, &height);
  if (width <= 0 || height <= 0) {
    return nullptr;
  }
  SurfaceGLES::SwapCallback swap_callback = [window]() -> bool {
    ::glfwSwapBuffers(window);
    return true;
  };
  return SurfaceGLES::WrapFBO(std::move(context),              //
                              swap_callback,                   //
                              0u,                              //
                              PixelFormat::kR8G8B8A8UNormInt,  //
                              ISize::MakeWH(width, height)     //
  );
}

}  // namespace impeller
