#pragma once

// hnll
#include <graphics/mesh_model.hpp>
#include <game/component.hpp>
#include <game/components/renderable_component.hpp>
#include <game/actor.hpp>

// std
#include <unordered_map>

namespace hnll {
namespace game {

template<class U> using u_ptr = std::unique_ptr<U>;
template<class S> using s_ptr = std::shared_ptr<S>;

class mesh_component : public renderable_component
{
  public:
    template <Actor A>
    static s_ptr<mesh_component> create(s_ptr<A>& owner_sp, s_ptr<hnll::graphics::mesh_model>&& model_sp)
    {
      auto mesh = std::make_shared<mesh_component>(owner_sp);
      mesh->set_mesh_model(std::move(model_sp));
      owner_sp->set_renderable_component(mesh);
      return mesh;
    }
    template <Actor A>
    mesh_component(s_ptr<A>& owner_sp) : renderable_component(owner_sp, render_type::MESH) {}
    ~mesh_component() override = default;

    s_ptr<hnll::graphics::mesh_model>& get_model_sp() { return model_sp_; }
    void set_mesh_model(s_ptr<hnll::graphics::mesh_model>&& model) { model_sp_ = std::move(model); }
        
  private:
    // hnll::graphics::mesh_model can be shared all over a game
    s_ptr<hnll::graphics::mesh_model> model_sp_ = nullptr;
};

using mesh_component_map = std::unordered_map<game::component_id, s_ptr<game::mesh_component>>;

} // namespace game
} // namespace hnll