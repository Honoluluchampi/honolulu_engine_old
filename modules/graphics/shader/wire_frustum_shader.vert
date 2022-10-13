#version 450

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec3 frag_pos_world;
layout(location = 2) out vec3 frag_normal_world;

// ubo
struct PointLight {
    vec4 position;
    vec4 color;
};

layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projection;
  mat4 view;
  vec4 ambient_light_colo;
  PointLight point_lights[20];
  int num_lights;
} ubo;

layout(push_constant) uniform Push {
  vec3 top_n;
  vec3 bottom_n;
  vec3 left_n;
  vec3 right_n;
  vec3 near_n;
  vec3 origin_p;
  vec3 near_p;
  vec3 far_p;
  vec3 color;
} push;



vec3 positions[8] = {
  vec3(push.)
};


void main() {
  vec3 far_n  = -push.near_n;
  
}