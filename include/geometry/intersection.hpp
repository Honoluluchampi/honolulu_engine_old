#pragma once

// std
#include <vector>
#include <memory>

// lib
#include <eigen3/Eigen/Dense>

namespace hnll{

template <typename T> using u_ptr = std::unique_ptr<T>;
template <typename T> using s_ptr = std::shared_ptr<T>;
using vec3d = Eigen::Vector3d;

namespace geometry {

// forward declaration
class bounding_volume;
class perspective_frustum;
struct ray;
struct vertex;
struct plane;

namespace intersection {

// returns the length of intersection
double test_bounding_volumes(const bounding_volume& a, const bounding_volume& b);

// testing functions for culling algorithms
double test_sphere_frustum(const geometry::bounding_volume& sphere, const perspective_frustum& frustum);

double test_aabb_aabb     (const bounding_volume& aabb_a, const bounding_volume& aabb_b);
double test_aabb_sphere   (const bounding_volume& aabb, const bounding_volume& sphere);
double test_sphere_sphere (const bounding_volume& sphere_a, const bounding_volume& sphere_b);
double test_ray_triangle  (const ray& _ray, const std::vector<vec3d>& vertices);
}; // namespace intersection

// helper functions
double distance_point_to_plane(const vec3d& q, const plane& p);
double sq_dist_point_to_aabb(const vec3d& p, const bounding_volume& aabb);

}} // namespace hnll::geometry