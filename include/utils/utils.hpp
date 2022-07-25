#pragma once

// TODO : put this file on a appropriate position
// TODO : create createOneShotCommandPool();

// std
#include <memory>
#include <iostream>

// lib
#include <glm/gtc/matrix_transform.hpp>

template<class U> using u_ptr = std::unique_ptr<U>;
template<class S> using s_ptr = std::shared_ptr<S>;

namespace hnll::utils {

// 3d transformation
struct transform
{
  glm::vec3 translation{}; // position offset
  glm::vec3 scale{1.f, 1.f, 1.f};
  // y-z-x tait-brian rotation
  glm::vec3 rotation{};

  // Matrix corrsponds to Translate * Ry * Rz * Rx * Scale
  // Rotations correspond to Tait-bryan angles of Y(1), Z(2), X(3)
  // https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
  glm::mat4 mat4(); 
  glm::mat4 rotate_mat4();
  glm::mat3 rotate_mat3();
  // normal = R * S(-1)
  glm::mat3 normal_matrix();
};

static inline glm::vec3 sclXvec(const float scalar, const glm::vec3& vec)
{ return {vec.x * scalar, vec.y * scalar, vec.z * scalar}; }

template <class V>
concept glm_vec = requires(V& vec){ glm::normalize(vec); };
// glm rotation direction (true : regular direction, false : irregular direction)
inline bool is_same_handed_system(const glm_vec auto& a1, const glm_vec auto& a2, const glm_vec auto& b1, const glm_vec auto& b2)
{
  if (glm::cross(a1, a2).z * glm::cross(b1, b2).z < 0) return false;
  return true;
}

struct viewer_info
{
  glm::mat4 projection{1.f};
  glm::mat4 view{1.f};
};
} // namespace hnll::utils
