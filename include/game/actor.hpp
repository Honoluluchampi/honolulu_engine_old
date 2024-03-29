#pragma once

// hnll
#include <game/component.hpp>
#include <utils/utils.hpp>

// std
#include <vector>
#include <memory>
#include <unordered_map>
#include <iostream>

namespace hnll {

// forward declaration
namespace physics { class collision_info; }
namespace game{

template <class T> using s_ptr = std::shared_ptr<T>;
template <class T> using u_ptr = std::unique_ptr<T>;

using actor_id = unsigned int;
// forward declaration
class renderable_component;
class rigid_component;

class actor
{
  public:
    enum class state { ACTIVE, PAUSED, DEAD };

    static s_ptr<actor> create();
    actor();
    // uncopyable, movable
    actor(const actor &) = delete;
    actor& operator=(const actor &) = delete;
    actor(actor &&) = default;
    actor& operator=(actor &&) = default;
    // TODO : delete renderable component from graphics_engine::shading_systems::render_targets
    virtual ~actor() = default;

    void update(float dt);
    void update_components(float dt);
    // for specific update
    virtual void update_actor(float dt) {}
    // for collision detection
    virtual void re_update(const physics::collision_info& info);

#ifndef IMGUI_DISABLED
void update_gui();
      void update_component_imgui();
      virtual void update_actor_imgui(){}
#endif

    // getter
    inline actor_id    get_id()          const { return id_; }
    inline const state get_actor_state() const { return state_; }
    inline bool        is_renderable()   const { return renderable_component_ != nullptr; }
    inline renderable_component&  get_renderable_component_r() { return *renderable_component_; }
    s_ptr<hnll::utils::transform> get_transform_sp();
    // setter
    void add_component(u_ptr<component>&& comp) { unique_components_.emplace_back(std::move(comp)); }
    void add_component(s_ptr<component>&& comp) { shared_components_.emplace_back(std::move(comp)); }
    void add_component(const s_ptr<component>& comp) { shared_components_.emplace_back(comp); }
    // takes std::shared_ptr<RenderableComponent>
    void set_renderable_component(s_ptr<renderable_component>&& comp);
    void set_renderable_component(const s_ptr<renderable_component>& comp);
    void set_transform(const s_ptr<utils::transform>& t) { transform_sp_ = t; }
    void set_translation(const glm::vec3& translation)   { transform_sp_->translation = translation; }
    void set_rotation(const glm::vec3& rotation)         { transform_sp_->rotation = rotation; }
    void set_scale(const glm::vec3& scale)               { transform_sp_->scale = scale; }
    inline void set_actor_state(state st)                { state_ = st; }

  private:
    actor_id id_;
    state state_ = state::ACTIVE;

    std::vector<u_ptr<component>> unique_components_;
    std::vector<s_ptr<component>> shared_components_;
    // TODO : multiple renderableComponent for one actor
    s_ptr<renderable_component> renderable_component_ = nullptr;
    s_ptr<hnll::utils::transform> transform_sp_;
};

}
} // namespace hnll::game
