#pragma once

// std
#include <memory>

// lib
#include <eigen3/Eigen/Dense>
#include <vulkan/vulkan.h>

// forward declaration
namespace tinygltf {
  class Node;
  class Model;
  struct Mesh;
}

namespace hnll {

using vec2  = Eigen::Vector2f;
using vec3  = Eigen::Vector3f;
using vec4  = Eigen::Vector4f;
using uvec4 = Eigen::Matrix<unsigned, 4, 1>;
using mat4  = Eigen::Matrix4f;
using quat  = Eigen::Quaternionf;
template<class T> using u_ptr = std::unique_ptr<T>;
template<class T> using s_ptr = std::shared_ptr<T>;

namespace graphics {

class device;
class buffer;
class descriptor_set_layout;
class descriptor_pool;

namespace skinning_utils
{

#define MAX_JOINTS_NUM 128u

struct node;

struct vertex
{
  alignas(16) vec3 position;
  alignas(16) vec3 normal;
  vec2  tex_coord;
  uvec4 joint_indices;
  vec4  joint_weights;
  static std::vector<VkVertexInputBindingDescription>   get_binding_descriptions();
  static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions();
};

struct mesh
{
  uint32_t index_start    = 0;
  uint32_t vertex_start   = 0;
  uint32_t index_count    = 0;
  uint32_t vertex_count   = 0;
  uint32_t material_index = 0;
};

struct mesh_group
{
  public:
    mesh_group(device& device);
    void build_desc();
    void update_desc_buffer();

    int node_index;
    std::vector<mesh> meshes;
    // TODO : add desc buffer
    struct uniform_block
    {
      mat4 matrix;
      mat4 joint_matrices[MAX_JOINTS_NUM]{};
      float joint_count = 0;
    } block;

  private:
    device&                      device_;
    u_ptr<buffer>                desc_buffer_;
    u_ptr<descriptor_set_layout> desc_layout_;
    VkDescriptorSet              desc_set_;
};

struct skin
{
  std::string name;
  s_ptr<node> root_node = nullptr;
  std::vector<s_ptr<node>>  joints;
  std::vector<mat4>         inv_bind_matrices;
  uint32_t skin_vertex_count;
};

struct node
{
  node()  = default;
  ~node() = default;

  std::string name = "";

  vec3 translation = { 0.f, 0.f, 0.f };
  quat rotation    = { 1.f, 0.f, 0.f, 0.f };
  vec3 scale       = { 1.f, 1.f, 1.f };
  mat4 local_mat   = Eigen::Matrix4f::Identity();
  mat4 world_mat   = Eigen::Matrix4f::Identity();

  std::vector<s_ptr<node>> children;
  s_ptr<node>      parent = nullptr;

  uint32_t index;

  int mesh_index = -1;

  mat4 matrix;
  s_ptr<mesh_group> mesh_group = nullptr;
  s_ptr<skin>       skin = nullptr;
  int32_t skin_index = -1;

  mat4 get_local_matrix();
  mat4 get_matrix();
  void update();
};

struct material
{
    std::string name = "";
    int texture_index = -1;
    vec3 diffuse_color = { 1.f, 1.f, 1.f };
};

struct image_info
{
  std::vector<uint8_t> image_buffer;
  std::string filepath;
};

struct texture_info { int image_index; };

struct skinning_model_builder
{
  std::vector<uint32_t> index_buffer;
  std::vector<vec3>     position_buffer;
  std::vector<vec3>     normal_buffer;
  std::vector<vec2>     tex_coord_buffer;

  std::vector<uvec4> joint_buffer;
  std::vector<vec4>  weight_buffer;
};

struct loader
{
  std::vector<uint32_t>               index_buffer;
  std::vector<skinning_utils::vertex> vertex_buffer;
  size_t index_pos  = 0;
  size_t vertex_pos = 0;
};

enum class interpolation_type
{
  LINEAR,
  STEP,
  CUBICSPLINE
};

struct animation_sampler
{
  interpolation_type interpolation;
  std::vector<float> inputs;
  std::vector<vec4>  outputs;
};

struct animation_channel
{
  enum class path_type
  {
    TRANSLATION,
    ROTATION,
    SCALE
  };

  path_type   path; // translation, rotation, scale, or weights
  s_ptr<node> node;
  uint32_t    sampler_index;
};

struct animation
{
  std::string name;
  std::vector<animation_sampler> samplers;
  std::vector<animation_channel> channels;
  float start = std::numeric_limits<float>::max();
  float end   = std::numeric_limits<float>::min();
  float current_time = 0.0f;
};

}  // namespace skinning_utils
}} // namespace hnll::graphics