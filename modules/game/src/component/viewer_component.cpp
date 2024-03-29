// hnll
#include <game/components/viewer_component.hpp>
#include <geometry/perspective_frustum.hpp>

// std
#include <cassert>
#include <limits>

// lib
#include <glm/glm.hpp>

namespace hnll::game {

float viewer_component::near_distance_ = 0.1f;
float viewer_component::far_distance_  = 50.f;
float viewer_component::fov_y_ = glm::radians(50.f);

s_ptr<viewer_component> viewer_component::create(utils::transform& transform, graphics::renderer &renderer,double fov_x, double fov_y, double near_z, double far_z)
{
  auto viewer_comp = std::make_shared<viewer_component>(transform, renderer);
  auto frustum = geometry::perspective_frustum::create(fov_x, fov_y, near_z, far_z);
  viewer_comp->set_perspective_frustum(std::move(frustum));
  return viewer_comp;
}

viewer_component::viewer_component(hnll::utils::transform& transform, hnll::graphics::renderer& renderer)
  : component(), transform_(transform), renderer_(renderer)
{
}

viewer_component::~viewer_component() = default;

void viewer_component::set_orthogonal_projection(float left, float right, float top, float bottom, float near, float far)
{
  projection_matrix_ <<
    2.f / (right - left), 0.f,                  0.f,                -(right + left) / (right - left),
    0.f,                  2.f / (bottom - top), 0.f,                -(bottom + top) / (bottom - top),
    0.f,                  0.f,                  1.f / (far - near), -near / (far - near),
    0.f,                  0.f,                  0.f,                1.f;
}

void viewer_component::update_frustum() { perspective_frustum_->update_planes(transform_); }
 
void viewer_component::set_perspective_projection(float fov_y, float aspect, float near, float far)
{
  assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
  const float tan_half_fov_y = tan(fov_y / 2.f);
  projection_matrix_ <<
    1.f / (aspect * tan_half_fov_y), 0.f,                  0.f,                0.f,
    0.f,                             1.f / tan_half_fov_y, 0.f,                0.f,
    0.f,                             0.f,                  far / (far - near), -(far * near) / (far - near),
    0.f,                             0.f,                  1.f,                0.f;
}

void viewer_component::set_view_direction(const glm::vec3& position, const glm::vec3& direction, const glm::vec3& up)
{
  const glm::vec3 w{glm::normalize(direction)};
  const glm::vec3 u{glm::normalize(glm::cross(w, up))};
  const glm::vec3 v{glm::cross(w, u)};

  view_matrix_ <<
    u.x, u.y, u.z, -glm::dot(u, position),
    v.x, v.y, v.z, -glm::dot(u, position),
    w.x, w.y, w.z, -glm::dot(w, position),
    0.f, 0.f, 0.f, 1.f;

  // also update inverse view matrix
  inverse_view_matrix_ <<
    u.x, v.x, w.x, position.x,
    u.y, v.y, w.y, position.y,
    u.z, v.z, w.z, position.z,
    0.f, 0.f, 0.f, 1.f;
}

void viewer_component::set_view_target(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up) 
{ set_view_direction(position, target - position, up); }

void viewer_component::set_view_yxz()
{
  // TODO : eigenize and delete
  auto& position_glm = transform_.translation;
  vec3 position = { position_glm.x, position_glm.y, position_glm.z };
  auto& rotation = transform_.rotation;
  const float c3 = std::cos(rotation.z);
  const float s3 = std::sin(rotation.z);
  const float c2 = std::cos(rotation.x);
  const float s2 = std::sin(rotation.x);
  const float c1 = std::cos(rotation.y);
  const float s1 = std::sin(rotation.y);
  const vec3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
  const vec3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
  const vec3 w{(c2 * s1), (-s2), (c1 * c2)};
  view_matrix_ <<
    u.x(), u.y(), u.z(), -u.dot(position),
    v.x(), v.y(), v.z(), -v.dot(position),
    w.x(), w.y(), w.z(), -w.dot(position),
    0.f,   0.f,   0.f,   1.f;

  // also update inverse view matrix
  inverse_view_matrix_ <<
    u.x(), v.x(), w.x(), position.x(),
    u.y(), v.y(), w.y(), position.y(),
    u.z(), v.z(), w.z(), position.z(),
    0.f, 0.f, 0.f, 1.f;
}

Eigen::Matrix4f viewer_component::get_inverse_perspective_projection() const
{
  // TODO : calc this in a safe manner
  return projection_matrix_.inverse();
}

Eigen::Matrix4f viewer_component::get_inverse_view_yxz() const
{
  // TODO : eigenize and delete
  auto& position_glm = transform_.translation;
  vec3 position = { -position_glm.x, -position_glm.y, -position_glm.z };
  auto rotation = -transform_.rotation;
  const float c3 = std::cos(rotation.z);
  const float s3 = std::sin(rotation.z);
  const float c2 = std::cos(rotation.x);
  const float s2 = std::sin(rotation.x);
  const float c1 = std::cos(rotation.y);
  const float s1 = std::sin(rotation.y);
  const vec3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
  const vec3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
  const vec3 w{(c2 * s1), (-s2), (c1 * c2)};

  Eigen::Matrix4f inv_view;
  inv_view <<
    u.x(), u.y(), u.z(), -u.dot(position),
    v.x(), v.y(), v.z(), -v.dot(position),
    w.x(), w.y(), w.z(), -w.dot(position),
    0.f,   0.f,   0.f,   1.f;
  return inv_view;
}

// owner's transform should be updated by keyMoveComp before this function
void viewer_component::update_component(float dt)
{
  set_view_yxz();
  auto aspect = renderer_.get_aspect_ratio();
  set_perspective_projection(fov_y_, aspect, near_distance_, far_distance_);

  if (update_view_frustum_ == update_view_frustum::ON)
    perspective_frustum_->update_planes(transform_);
}

// getter setter
const geometry::perspective_frustum& viewer_component::get_perspective_frustum_ref() const
{ return *perspective_frustum_; }

void viewer_component::set_perspective_frustum(s_ptr<geometry::perspective_frustum>&& frustum)
{ perspective_frustum_ = std::move(frustum); }



} // namespace hnll::game