#pragma once

// hnll
#include <graphics/engine.hpp>
#include <imgui/renderer.hpp>

// basic header
#include <imgui.h>

// api-specific  header
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

// lib
#include <GLFW/glfw3.h>

// std
#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort

// debug
#ifndef NDEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
#endif

// shared objects
// Vulkan Instance
// Vulkan Physical Device
// Vulkan Logical Device
// Renderer Queue
// Present Queue

// imgui-specific objects
// window (config window)
// surface
// swap chain
// depth image
// render pass
// frame buffer
// descriptor pool
// semaphores
// command pool
// command buffer

namespace hnll {

class Hie
{
public:
  Hie(HveWindow& hveWindow, HveDevice& hveDevice);
  ~Hie();
  Hie(const Hie&) = delete;
  Hie& operator=(const Hie&) = delete;
  Hie(Hie&&) = default;
  Hie& operator=(Hie&&) = default;

  void beginImGui();
  void render();
  void frameRender();
  void update(glm::vec3& translation);

  const u_ptr<HieRenderer>& upHieRenderer() const { return upHieRenderer_; }
  HveRenderer* pHieRenderer() const { return upHieRenderer_.get(); }
  
private:
  // set up ImGui context
  void setupImGui(HveDevice& hveDevice, GLFWwindow* window);
  // share the basic graphics object with hve, so there is nothing to do for now
  void setupSpecificVulkanObjects();
  void uploadFonts();
  void cleanupVulkan();
  void createDescriptorPool();

  static void glfw_error_callback(int error, const char* description)
  { fprintf(stderr, "Glfw Error %d: %s\n", error, description); }

  VkDevice device_;
  VkDescriptorPool descriptorPool_;
  VkQueue graphicsQueue_;

  u_ptr<HieRenderer> upHieRenderer_;

  ImGui_ImplVulkanH_Window mainWindowData_;
  // TODO : make it consistent with hve
  int minImageCount_ = 2;
  bool swapChainRebuild_ = false;
  bool isHieRunning_ = false;

  float vec_[3] = {0, 0, 0};
};

} // namespace hnll 