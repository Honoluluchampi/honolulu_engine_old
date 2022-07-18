#pragma once

// hnll
#include <game/component.hpp>
#include <utils/utils.hpp>

namespace hnll {
namespace game {

enum class render_type 
{
  SIMPLE,
  POINT_LIGHT,
  LINE
};

class renderable_component : public hnll::game::component
{
  public:
    renderable_component(render_type type) : component(), render_type_(type)
    { transform_sp_ = std::make_shared<hnll::utils::transform>(); }
    ~renderable_component() {}

    renderable_component(const renderable_component &) = delete;
    renderable_component& operator=(const renderable_component &) = delete;
    renderable_component(renderable_component &&) = default;
    renderable_component& operator=(renderable_component &&) = default;

    // getter
    inline hnll::utils::transform get_transform() { return *transform_sp_; }
    inline s_ptr<hnll::utils::transform> get_transform_sp() { return transform_sp_; }
    const render_type get_render_type() const { return render_type_; }

    // setter
    // basically called by game::actor
    void set_transform_sp(const s_ptr<hnll::utils::transform>& ptr) { transform_sp_ = ptr; }
    template<class V> void set_transform(V&& vec) { transform_sp_ = std::make_unique<hnll::utils::transform>(vec); }
    template<class V> void set_translation(V&& vec) { transform_sp_->translation = std::forward<V>(vec); }
    template<class V> void set_scale(V&& vec) { transform_sp_->scale = std::forward<V>(vec); }
    template<class V> void set_rotation(V&& vec) { transform_sp_->rotation = std::forward<V>(vec); }

    virtual void update_component(float dt) override {}

  protected:
    // update this member
    s_ptr<hnll::utils::transform> transform_sp_ = nullptr;
    render_type render_type_;
};

} // namespace graphics
} // namespace hnll