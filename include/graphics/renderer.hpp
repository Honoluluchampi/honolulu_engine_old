// resposible for swap chain, command buffers, drawing frame

#pragma once

// hnll
#include <graphics/window.hpp>
#include <graphics/device.hpp>
#include <graphics/swap_chain.hpp>

// std
#include <memory>
#include <vector>
#include <cassert>

namespace hnll::graphics {
// define ~_RENDER_PASS_ID in renderer class
// to specify indices of multiple render pass and frame buffer
#define HVE_RENDER_PASS_ID 0

template <class S> using s_ptr = std::shared_ptr<S>;
template <class U> using u_ptr = std::unique_ptr<U>;

class renderer
{
  public:

    renderer(window& window, device& device, bool recreate_from_scratch = true);
    virtual ~renderer();

    renderer(const renderer &) = delete;
    renderer &operator= (const renderer &) = delete;

    // getter
#ifdef IMGUI_DISABLED
    inline VkRenderPass get_swap_chain_render_pass() const { return swap_chain_->get_render_pass(); }
#else
    inline VkRenderPass get_swap_chain_render_pass(int render_pass_id) const 
    { return swap_chain_->get_render_pass(render_pass_id); }
#endif 

    inline float get_aspect_ratio()         const { return swap_chain_->extent_aspect_ratio(); }
    inline bool is_frame_in_progress()      const { return is_frame_started_; }
    inline swap_chain& get_swap_chain()     const { return *swap_chain_; }
    inline VkCommandPool get_command_pool() const { return device_.get_command_pool(); }
    inline VkImage       get_image(int index) const { return swap_chain_->get_image(index); }
    inline VkImageView   get_view(int index)  const { return swap_chain_->get_image_view(index); }
    VkCommandBuffer get_current_command_buffer() const
    {
      // assert(is_frame_started_ && "Cannot get command buffer when frame not in progress");
      return command_buffers_[current_frame_index_];
    }
    int get_frame_index() const 
    {
      // assert(is_frame_started_ && "Cannot get frame when frame not in progress");
      return current_frame_index_;
    }

    VkCommandBuffer begin_frame();
    void end_frame();
    void begin_swap_chain_render_pass(VkCommandBuffer command_buffer, int render_pass_id);
    void end_swap_chain_render_pass(VkCommandBuffer command_buffer);

    virtual void recreate_swap_chain();

    inline void set_next_renderer(renderer* renderer)
    { next_renderer_ = renderer; }

    const bool is_last_renderer() const 
    { return !next_renderer_; }

    static bool swap_chain_recreated_;
    static void cleanup_swap_chain();

  private:

#ifndef IMGUI_DISABLED
    static void submit_command_buffers();
    static void reset_frame();
#endif

    void create_command_buffers();
    void free_command_buffers();

  protected:    
    window& window_;
    device& device_;
    std::vector<VkCommandBuffer> command_buffers_;

    static uint32_t current_image_index_;
    static int current_frame_index_; // [0, max_frames_in_flight]
    bool is_frame_started_ = false;

#ifndef IMGUI_DISABLED
    // store multiple renderers' command buffers
    static std::vector<VkCommandBuffer> submitting_command_buffers_;
#endif

    // TODO : use smart pointer
    renderer* next_renderer_ = nullptr;

    static u_ptr<swap_chain> swap_chain_;
};

} // namespace hnll::graphics