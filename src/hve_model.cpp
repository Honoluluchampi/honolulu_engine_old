#include <hve_model.hpp>
#include <hve_utils.hpp>

// libs
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

// std
#include <cstring>
#include <iostream>
#include <unordered_map>

namespace std {

template <>
struct hash<hve::HveModel::Vertex>
{
  size_t operator() (hve::HveModel::Vertex const &vertex) const
  {
    // stores final hash value
    size_t seed = 0;
    hve::hashCombine(seed, vertex.position_m, vertex.color_m, vertex.normal_m, vertex.uv_m);
    return seed;
  }
};
} // namespace stD

namespace hve {

HveModel::HveModel(HveDevice& device, const HveModel::Builder &builder) : hveDevice_m{device}
{
  createVertexBuffers(builder.vertices_m);
  createIndexBuffers(builder.indices_m);
}

HveModel::~HveModel()
{
  vkDestroyBuffer(hveDevice_m.device(), vertexBuffer_m, nullptr);
  vkFreeMemory(hveDevice_m.device(), vertexBufferMemory_m, nullptr);

  if (hasIndexBuffer_m) {
    vkDestroyBuffer(hveDevice_m.device(), indexBuffer_m, nullptr);
    vkFreeMemory(hveDevice_m.device(), indexBufferMemory_m, nullptr);
  }
}

std::unique_ptr<HveModel> HveModel::createModelFromFile(HveDevice &device, const std::string &filename)
{
  Builder builder;
  builder.loadModel(filename);
  std::cout << "Vertex count: " << builder.vertices_m.size() << "\n";
  return std::make_unique<HveModel>(device, builder);
}

void HveModel::createVertexBuffers(const std::vector<Vertex> &vertices)
{
  vertexCount_m = static_cast<uint32_t>(vertices.size());
  // vertexCount must be larger than 3 (triangle) 
  // use a host visible buffer as temporary buffer, use a device local buffer as actual vertex buffer
  VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount_m;

  // copy the data to the staging buffer
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  hveDevice_m.createBuffer(
    bufferSize,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    stagingBuffer,
    stagingBufferMemory
  );

  // filling in the data to the staging buffer
  void *data;
  vkMapMemory(hveDevice_m.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
  std::memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
  vkUnmapMemory(hveDevice_m.device(), stagingBufferMemory);

  hveDevice_m.createBuffer(
    bufferSize,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // optimal type of the memory type
    vertexBuffer_m,
    vertexBufferMemory_m
  );

  // copy the data from staging buffer to the vertex buffer
  hveDevice_m.copyBuffer(stagingBuffer, vertexBuffer_m, bufferSize);
  // clean up the staging buffer no longer necessary
  vkDestroyBuffer(hveDevice_m.device(), stagingBuffer, nullptr);
  vkFreeMemory(hveDevice_m.device(), stagingBufferMemory, nullptr);
}

void HveModel::createIndexBuffers(const std::vector<uint32_t> &indices)
{
  indexCount_m = static_cast<uint32_t>(indices.size());
  // if there is no index, nothing to do
  hasIndexBuffer_m = indexCount_m > 0;
  if (!hasIndexBuffer_m) return;

  // vertexCount must be larger than 3 (triangle) 
  // use a host visible buffer as temporary buffer, use a device local buffer as actual vertex buffer
  VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount_m;

  // copy the data to the staging buffer
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  hveDevice_m.createBuffer(
    bufferSize,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    stagingBuffer,
    stagingBufferMemory
  );

  // filling in the data to the staging buffer
  void *data;
  vkMapMemory(hveDevice_m.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
  std::memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
  vkUnmapMemory(hveDevice_m.device(), stagingBufferMemory);

  hveDevice_m.createBuffer(
    bufferSize,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // optimal type of the memory type
    indexBuffer_m,
    indexBufferMemory_m
  );

  // copy the data from staging buffer to the vertex buffer
  hveDevice_m.copyBuffer(stagingBuffer, indexBuffer_m, bufferSize);

  // clean up the staging buffer no longer necessary
  vkDestroyBuffer(hveDevice_m.device(), stagingBuffer, nullptr);
  vkFreeMemory(hveDevice_m.device(), stagingBufferMemory, nullptr);
}

void HveModel::draw(VkCommandBuffer commandBuffer)
{
  if (hasIndexBuffer_m)
    vkCmdDrawIndexed(commandBuffer, indexCount_m, 1, 0, 0, 0);
  else 
    vkCmdDraw(commandBuffer, vertexCount_m, 1, 0, 0);
}

void HveModel::bind(VkCommandBuffer commandBuffer)
{
  VkBuffer buffers[] = {vertexBuffer_m};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

  // last parameter should be same as the type of the Build::indices
  if (hasIndexBuffer_m) 
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer_m, 0, VK_INDEX_TYPE_UINT32);
}

std::vector<VkVertexInputBindingDescription> HveModel::Vertex::getBindingDescriptions()
{
  std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
  // per-vertex data is packed together in one array, so the index of the 
  // binding in the array is always 0
  bindingDescriptions[0].binding   = 0;
  // number of bytes from one entry to the next
  bindingDescriptions[0].stride    = sizeof(Vertex);
  bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return bindingDescriptions;
}
std::vector<VkVertexInputAttributeDescription> HveModel::Vertex::getAttributeDescriptions()
{
  // how to extract a vertex attribute from a chunk of vertex data
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

  // location, binding, format, offset
  attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position_m)});
  attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color_m)});
  attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal_m)});
  attributeDescriptions.push_back({3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv_m)});

  return attributeDescriptions;
}

void HveModel::Builder::loadModel(const std::string& filename)
{
  // loader
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str()))
    throw std::runtime_error(warn + err);

  vertices_m.clear();
  indices_m.clear();

  std::unordered_map<Vertex, uint32_t> uniqueVertices{};


  for (const auto &shape : shapes) {
    for (const auto &index : shape.mesh.indices) {
      Vertex vertex{};
      // copy the vertex
      if (index.vertex_index >= 0) {
        vertex.position_m = {
          attrib.vertices[3 * index.vertex_index + 0],
          attrib.vertices[3 * index.vertex_index + 1],
          attrib.vertices[3 * index.vertex_index + 2]
        };
        // color support
        vertex.color_m = {
          attrib.colors[3 * index.vertex_index + 0],
          attrib.colors[3 * index.vertex_index + 1],
          attrib.colors[3 * index.vertex_index + 2]
        };
      }
      // copy the normal
      if (index.vertex_index >= 0) {
        vertex.normal_m = {
          attrib.normals[3 * index.normal_index + 0],
          attrib.normals[3 * index.normal_index + 1],
          attrib.normals[3 * index.normal_index + 2]
        };
      }
      // copy the texture coordinate
      if (index.vertex_index >= 0) {
        vertex.uv_m = {
          attrib.vertices[2 * index.texcoord_index + 0],
          attrib.vertices[2 * index.texcoord_index + 1]
        };
      }
      // if vertex is a new vertex
      if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] = static_cast<uint32_t>(vertices_m.size());
        vertices_m.push_back(std::move(vertex));
      }
      indices_m.push_back(uniqueVertices[vertex]);
    }
  }
}
} // namespace hveo