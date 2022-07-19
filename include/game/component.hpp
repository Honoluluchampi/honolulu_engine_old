#pragma once

// std
#include <memory>

template <class T> using s_ptr = std::shared_ptr<T>;
template <class T> using u_ptr = std::unique_ptr<T>;

namespace hnll {
namespace game {

using component_id = unsigned int;

class component
{
public:
  component();
  virtual ~component(){}

  inline void update(float dt) { update_component(dt); }

#ifndef IMGUI_DISABLED
  virtual void update_gui(){}
#endif

  // getter
  component_id get_id() const { return id_; }
private:
  virtual void update_component(float dt) {}
  component_id id_;
};

} // namespace game
} // namespace hnll