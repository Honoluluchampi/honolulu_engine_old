#pragma once

// hnll
#include <graphics/device.hpp>

// graphics headers
#include <vulkan/vulkan.h>

// std lib headers
#include <string>
#include <vector>
#include <memory>

namespace hnll::graphics {

// TODO : configure renderer count in a systematic way
#define RENDERER_COUNT 2
#define HVE_RENDER_PASS_ID 0
#define GUI_RENDER_PASS_ID 1

class image_resource {
  public:
    // getter
    [[nodiscard]] VkImage           get_image()        const { return image_; }
    [[nodiscard]] VkImageView       get_image_view()   const { return view_; }
    [[nodiscard]] VkDeviceMemory    get_memory()       const { return memory_; }
    [[nodiscard]] VkImageLayout     get_image_layout() const { return layout_; }
    [[nodiscard]] const VkExtent2D& get_extent()       const { return extent_; }
    [[nodiscard]] VkImageSubresourceRange get_sub_resource_range() const { return sub_resource_range_; }

    const VkDescriptorImageInfo *get_descriptor(VkSampler sampler = VK_NULL_HANDLE)
    {
      descriptor_.imageView = view_;
      descriptor_.imageLayout = layout_;
      descriptor_.sampler = sampler;
      return &descriptor_;
    }

    // setter
    void set_image(const VkImage image)                 { image_ = image; }
    void set_image_view(const VkImageView view)         { view_ = view; }
    void set_extent(const VkExtent2D& extent)           { extent_ = extent; }
    void set_device_memory(const VkDeviceMemory memory) { memory_ = memory; }
    void set_image_layout_barrier_state(VkCommandBuffer command, VkImageLayout new_layout);

  private:
    VkImage               image_ = VK_NULL_HANDLE;
    VkImageView           view_ = VK_NULL_HANDLE;
    VkDeviceMemory        memory_ = VK_NULL_HANDLE;
    VkImageLayout         layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
    VkExtent2D            extent_;
    VkDescriptorImageInfo descriptor_ = {};

    VkImageSubresourceRange sub_resource_range_ = {
      VK_IMAGE_ASPECT_COLOR_BIT,
      0, // base mip level
      1, // level count
      0, // base array layer
      1, // layer count
    };
};

class swap_chain {
 public:
  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

  swap_chain(device &device, VkExtent2D window_extent);
  swap_chain(device &device, VkExtent2D window_extent, std::unique_ptr<swap_chain> previous);
  ~swap_chain();

  swap_chain(const swap_chain &) = delete;
  swap_chain& operator=(const swap_chain &) = delete;

#ifdef IMGUI_DISABLED
  VkFramebuffer get_frame_buffer(int index) { return swap_chain_frame_buffers_[index]; }
  VkRenderPass get_render_pass() { return render_pass_; }
#else
  VkFramebuffer get_frame_buffer(int render_pass_id, int index) { return multiple_frame_buffers_[render_pass_id][index]; }
  VkRenderPass get_render_pass(int render_pass_id) { return multiple_render_pass_[render_pass_id]; }
#endif

  VkImage            get_image(int index)      { return swap_chain_images_[index]; }
  VkImageView        get_image_view(int index) { return swap_chain_image_views_[index]; }
  size_t             get_image_count() { return swap_chain_images_.size(); }
  VkFormat           get_swap_chain_images_format() { return swap_chain_image_format_; }
  VkExtent2D         get_swap_chain_extent() { return swap_chain_extent_; }
  VkSurfaceFormatKHR get_surface_format() { return surface_format_; }
  uint32_t get_width() { return swap_chain_extent_.width; }
  uint32_t get_height() { return swap_chain_extent_.height; }
  const VkSemaphore& get_current_image_available_semaphore() const { return image_available_semaphores_[current_frame_]; }
  const VkSemaphore& get_current_render_finished_semaphore() const { return render_finished_semaphores_[current_frame_]; }
  const VkFence& get_current_in_flight_fence() const { return in_flight_fences_[current_frame_]; }
  const VkFence& get_current_images_in_flight_fence() const { return images_in_flight_[current_frame_]; }

  float extent_aspect_ratio() { return static_cast<float>(swap_chain_extent_.width) / static_cast<float>(swap_chain_extent_.height); }

  VkFormat find_depth_format();
  VkResult acquire_next_image(uint32_t *image_index);
  VkResult submit_command_buffers(const VkCommandBuffer *buffers, uint32_t *image_index);

  // swap chain validation whether its compatible with the render_pass
  bool compare_swap_chain_formats(const swap_chain& swap_chain) const 
  { return (swap_chain.swap_chain_depth_format_ == this->swap_chain_depth_format_) && (swap_chain.swap_chain_image_format_ == this->swap_chain_image_format_); }

#ifndef IMGUI_DISABLED
  void set_render_pass(VkRenderPass render_pass, int render_pass_id)
  { 
    reset_render_pass(render_pass_id);
    multiple_render_pass_[render_pass_id] = render_pass; 
  }

  // VFB : std::vector<VkFramebuffer>
  template <class VFB>
  void set_frame_buffers(VFB&& frame_buffers, int render_pass_id)
  { 
    reset_frame_buffers(render_pass_id);
    multiple_frame_buffers_[render_pass_id] = std::forward<VFB>(frame_buffers); 
  }
#endif

 private:
  void init();
  void create_swap_chain();
  void create_image_views();
  void create_depth_resources();
  void create_sync_objects();

  VkRenderPass create_render_pass();
  std::vector<VkFramebuffer> create_frame_buffers(VkRenderPass render_pass);

#ifndef IMGUI_DISABLED
  void create_multiple_frame_buffers();
  void create_multiple_render_pass();
  void reset_frame_buffers(int render_pass_id);
  void reset_render_pass(int render_pass_id);
#endif

  // Helper functions
  VkSurfaceFormatKHR choose_swap_surface_format(
    const std::vector<VkSurfaceFormatKHR> &available_formats);
  // most important settings for swap chain
  VkPresentModeKHR choose_swap_present_mode(
    const std::vector<VkPresentModeKHR> &available_present_modes);
  // choose resolution of output
  VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR &capabilities);


  // --------- variables ---------------------------------------------------
  VkFormat swap_chain_image_format_;
  VkFormat swap_chain_depth_format_;
  VkSurfaceFormatKHR surface_format_;
  VkExtent2D swap_chain_extent_;

  // tell Vulkan about the framebuffer attachments that will be used while rendering
  // how many color and depth buffers there will be
  // how many samples to use for each of them
#ifdef IMGUI_DISABLED
  std::vector<VkFramebuffer> swap_chain_frame_buffers_;
  VkRenderPass render_pass_;
#else
  std::vector<std::vector<VkFramebuffer>> multiple_frame_buffers_;
  std::vector<VkRenderPass> multiple_render_pass_;
#endif

  std::vector<VkImage> depth_images_;
  std::vector<VkDeviceMemory> depth_image_memories_;
  std::vector<VkImageView> depth_image_views_;
  std::vector<VkImage> swap_chain_images_;
  std::vector<VkImageView> swap_chain_image_views_;

  device &device_;
  VkExtent2D window_extent_;

  VkSwapchainKHR swap_chain_;
  std::unique_ptr<swap_chain> old_swap_chain_;

  // an image has been acquired and is ready for rendering
  std::vector<VkSemaphore> image_available_semaphores_;
  // rendering has finished and presentation can be happened
  std::vector<VkSemaphore> render_finished_semaphores_;
  // to use the right pair of semaphores every time
  size_t current_frame_ = 0;
  // for CPU-GPU synchronization
  std::vector<VkFence> in_flight_fences_;
  // wait on before a new frame can use that image
  std::vector<VkFence> images_in_flight_;
};

} // namespace hnll::graphics