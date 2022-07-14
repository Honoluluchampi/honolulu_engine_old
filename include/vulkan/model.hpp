#pragma once

#include <vulkan/device.hpp>
#include <vulkan/buffer.hpp>

// lib
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// std
#include <memory>
#include <string>

#pragma once

#include <functional>

namespace hnll {

// https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x/57595105#57595105
template <typename T, typename... Rest>
void hashCombine(std::size_t& seed, const T& v, const Rest&... rest)
{
  seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  (hashCombine(seed, rest), ...);
}

class HveModel
{
  public:
    using map = std::unordered_map<std::string, std::shared_ptr<HveModel>>;
    // compatible with wavefront obj. file
    struct Vertex
    {
      glm::vec3 position_m{};
      glm::vec3 color_m{};
      glm::vec3 normal_m{};
      // texture cordinates
      glm::vec2 uv_m{};
      // return a description compatible with the shader
      static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
      static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

      bool operator==(const Vertex& other) const
      { return position_m == other.position_m && color_m == other.color_m && normal_m == other.normal_m && uv_m == other.uv_m; }
    };

    struct Builder
    {
      // copied to the vertex buffer and index buffer
      std::vector<Vertex> vertices_m{};
      std::vector<uint32_t> indices_m{};

      void loadModel(const std::string& filename);
    };

    HveModel(HveDevice& device, const HveModel::Builder &builder);
    ~HveModel();

    HveModel(const HveModel &) = delete;
    HveModel& operator=(const HveModel &) = delete;

    static std::shared_ptr<HveModel> createModelFromFile(HveDevice &device, const std::string &filename);

    void bind(VkCommandBuffer commandBuffer);
    void draw(VkCommandBuffer commandBuffer);

  private:
    void createVertexBuffers(const std::vector<Vertex> &vertices);
    void createIndexBuffers(const std::vector<uint32_t> &indices);

    HveDevice& hveDevice_m;
    // contains buffer itself and buffer memory
    std::unique_ptr<HveBuffer> vertexBuffer_m;
    std::unique_ptr<HveBuffer> indexBuffer_m;
    uint32_t vertexCount_m;
    uint32_t indexCount_m;

    bool hasIndexBuffer_m = false;
};

} // namespace hve