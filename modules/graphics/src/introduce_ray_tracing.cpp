// hnll
#include <graphics/device.hpp>
#include <graphics/buffer.hpp>
#include <graphics/descriptor_set_layout.hpp>
#include <graphics/pipeline.hpp>

// sub
#include <extensions_vk.hpp>

// std
#include <iostream>

// lib
#include <eigen3/Eigen/Dense>

namespace hnll {

using vec3 = Eigen::Vector3f;
template<typename T> using u_ptr = std::unique_ptr<T>;
template<typename T> using s_ptr = std::shared_ptr<T>;

enum class shader_stages {
  RAY_GENERATION,
  MISS,
  CLOSEST_HIT,
  ANY_HIT,
  INTERSECTION,
  MAX_STAGE,
};

struct acceleration_structure
{
  VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
  VkDeviceMemory             memory = VK_NULL_HANDLE;
  VkBuffer                   buffer = VK_NULL_HANDLE;
  VkDeviceAddress            device_address = 0;
};

struct ray_tracing_scratch_buffer
{
  VkBuffer        handle = VK_NULL_HANDLE;
  VkDeviceMemory  memory = VK_NULL_HANDLE;
  VkDeviceAddress device_address = 0;
};

class image_resource {
  public:
    // getter
    [[nodiscard]] VkImage        get_image()        const { return image_; }
    [[nodiscard]] VkImageView    get_image_view()   const { return view_; }
    [[nodiscard]] VkDeviceMemory get_memory()       const { return memory_; }
    [[nodiscard]] VkImageLayout  get_image_layout() const { return layout_; }

    const VkDescriptorImageInfo *get_descriptor(VkSampler sampler = VK_NULL_HANDLE)
    {
      descriptor_.imageView = view_;
      descriptor_.imageLayout = layout_;
      descriptor_.sampler = sampler;
      return &descriptor_;
    }

    // setter
    void set_image(VkImage image) { image_ = image; }
    void set_image_view(VkImageView view) { view_ = view; }
    void set_device_memory(VkDeviceMemory memory) { memory_ = memory; }

    void set_image_layout_barrier_state(VkCommandBuffer command, VkImageLayout new_layout)
    {
      VkImageMemoryBarrier barrier {};
      barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barrier.oldLayout = layout_;
      barrier.newLayout = new_layout;
      barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.subresourceRange = sub_resource_range_;
      barrier.image = image_;

      VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
      VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

      switch (layout_) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
          barrier.srcAccessMask = 0;
          break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
          barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
          src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
          break;
        default:
          break;
      }

      switch (new_layout) {
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
          barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
          dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
          break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
          barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
          dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
          break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
          barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
          dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
          dst_stage = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
          break;
        default:
          break;
      }

      vkCmdPipelineBarrier(
        command,
        src_stage,
        dst_stage,
        0, 0, nullptr, 0, nullptr, 1, &barrier
      );

      layout_ = new_layout;
    }

  private:
    VkImage image_ = VK_NULL_HANDLE;
    VkImageView view_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
    VkImageLayout layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageSubresourceRange sub_resource_range_ = {
      VK_IMAGE_ASPECT_COLOR_BIT,
      0, // base mip level
      1, // level count
      0, // base array layer
      1, // layer count
    };
    VkDescriptorImageInfo descriptor_ = {};
};

VkDeviceAddress get_device_address(VkDevice device, VkBuffer buffer)
{
  VkBufferDeviceAddressInfo buffer_device_info {
    VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
    nullptr
  };
  buffer_device_info.buffer = buffer;
  return vkGetBufferDeviceAddress(device, &buffer_device_info);
}

template<class T> T align(T size, uint32_t align)
{ return (size + align - 1) & ~static_cast<T>(align - 1); }

class hello_triangle {
  public:
    hello_triangle()
    {
      window_ = std::make_unique<graphics::window>(1920, 1080, "hello ray tracing triangle");
      device_ = std::make_unique<graphics::device>(*window_, graphics::rendering_type::RAY_TRACING);

      // load all available extensions (of course including ray tracing extensions)
      load_VK_EXTENSIONS(device_->get_instance(), vkGetInstanceProcAddr, device_->get_device(), vkGetDeviceProcAddr);

      create_triangle_as();
    }

    ~hello_triangle()
    {
      destroy_acceleration_structure(*blas_);
      destroy_acceleration_structure(*tlas_);
      destroy_image_resource(*ray_traced_image_);
      vkDestroyPipelineLayout(device_->get_device(), pipeline_layout_, nullptr);
      vkDestroyPipeline(device_->get_device(), pipeline_, nullptr);
    }

    void run()
    {
      while (glfwWindowShouldClose(window_->get_glfw_window()) == GLFW_FALSE) {
        glfwPollEvents();
        update();
        render();
      }
    }

  private:

    void update()
    {

    }

    void render()
    {

    }

    void create_triangle_as()
    {
      create_vertex_buffer();
      create_triangle_blas();
      create_scene_tlas();
      create_ray_traced_image();
      create_layout();
      create_pipeline();
      create_shader_binding_table();
      create_descriptor_set();
    }

    void create_vertex_buffer()
    {
      uint32_t vertex_count = triangle_vertices_.size();
      uint32_t vertex_size = sizeof(triangle_vertices_[0]);
      VkDeviceSize buffer_size = vertex_size * vertex_count;

      // create staging buffer
      graphics::buffer staging_buffer{
          *device_,
          vertex_size,
          vertex_count,
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
      };
      staging_buffer.map();
      staging_buffer.write_to_buffer((void *) triangle_vertices_.data());

      // setup vertex buffer create info
      VkBufferUsageFlags usage =
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

      // create vertex buffer
      vertex_buffer_ = std::make_unique<graphics::buffer>(
          *device_,
          vertex_size,
          vertex_count,
          usage,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
      );
      // write data to the buffer
      device_->copy_buffer(staging_buffer.get_buffer(), vertex_buffer_->get_buffer(), buffer_size);
    }

    void create_triangle_blas()
    {
      // blas build setup

      // get vertex buffer device address
      VkDeviceOrHostAddressConstKHR vertex_buffer_device_address {};
      vertex_buffer_device_address.deviceAddress =
          get_device_address(device_->get_device(), vertex_buffer_->get_buffer());

      // geometry
      VkAccelerationStructureGeometryKHR as_geometry {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR
      };
      as_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
      as_geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
      as_geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
      as_geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
      as_geometry.geometry.triangles.vertexData = vertex_buffer_device_address;
      as_geometry.geometry.triangles.maxVertex = triangle_vertices_.size();
      as_geometry.geometry.triangles.vertexStride = sizeof(vec3);
      as_geometry.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_KHR;

      // build geometry info
      VkAccelerationStructureBuildGeometryInfoKHR as_build_geometry_info {};
      as_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
      as_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
      // prefer performance rather than as build
      as_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
      as_build_geometry_info.geometryCount = 1; // only one triangle
      as_build_geometry_info.pGeometries = &as_geometry;

      // get as size
      uint32_t num_triangles = 1;
      VkAccelerationStructureBuildSizesInfoKHR as_build_sizes_info {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR
      };
      vkGetAccelerationStructureBuildSizesKHR(
        device_->get_device(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &as_build_geometry_info,
        &num_triangles,
        &as_build_sizes_info
      );

      // build blas (get handle of VkAccelerationStructureKHR)
      blas_ = create_acceleration_structure_buffer(as_build_sizes_info);

      // create blas
      VkAccelerationStructureCreateInfoKHR as_create_info {};
      as_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
      as_create_info.buffer = blas_->buffer;
      as_create_info.size = as_build_sizes_info.accelerationStructureSize;
      as_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
      vkCreateAccelerationStructureKHR(device_->get_device(), &as_create_info, nullptr, &blas_->handle);

      // get the device address of blas
      VkAccelerationStructureDeviceAddressInfoKHR as_device_address_info {};
      as_device_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
      as_device_address_info.accelerationStructure = blas_->handle;
      blas_->device_address = vkGetAccelerationStructureDeviceAddressKHR(device_->get_device(), &as_device_address_info);

      // create scratch buffer
      auto scratch_buffer = create_scratch_buffer(as_build_sizes_info.buildScratchSize);

      // build blas
      as_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
      as_build_geometry_info.dstAccelerationStructure = blas_->handle;
      as_build_geometry_info.scratchData.deviceAddress = scratch_buffer->device_address;

      // execute blas build command (in GPU)
      VkAccelerationStructureBuildRangeInfoKHR as_build_range_info {};
      as_build_range_info.primitiveCount = num_triangles;
      as_build_range_info.primitiveOffset = 0;
      as_build_range_info.firstVertex = 0;
      as_build_range_info.transformOffset = 0;

      std::vector<VkAccelerationStructureBuildRangeInfoKHR*> as_build_range_infos = { &as_build_range_info };

      auto command = device_->begin_one_shot_commands();
      vkCmdBuildAccelerationStructuresKHR(
        command, 1, &as_build_geometry_info, as_build_range_infos.data()
      );

      VkBufferMemoryBarrier barrier { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
      barrier.buffer = blas_->buffer;
      barrier.size = VK_WHOLE_SIZE;
      barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.srcAccessMask =
        VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR |
        VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
      barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

      vkCmdPipelineBarrier(
        command,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        0, 0, nullptr, 1, &barrier, 0, nullptr
      );

      device_->end_one_shot_commands(command);

      // destroy scratch buffer
      vkDestroyBuffer(device_->get_device(), scratch_buffer->handle, nullptr);
      vkFreeMemory(device_->get_device(), scratch_buffer->memory, nullptr);
    }

    u_ptr<acceleration_structure> create_acceleration_structure_buffer
        (VkAccelerationStructureBuildSizesInfoKHR build_size_info)
    {
      auto as = std::make_unique<acceleration_structure>();
      auto usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

      device_->create_buffer(
        build_size_info.accelerationStructureSize,
        usage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        as->buffer,
        as->memory
      );

      return as;
    }

    u_ptr<ray_tracing_scratch_buffer> create_scratch_buffer(VkDeviceSize size)
    {
      auto scratch_buffer = std::make_unique<ray_tracing_scratch_buffer>();
      auto usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

      device_->create_buffer(
        size,
        usage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        scratch_buffer->handle,
        scratch_buffer->memory
      );

      scratch_buffer->device_address = get_device_address(device_->get_device(), scratch_buffer->handle);
      return scratch_buffer;
    }

    void create_scene_tlas()
    {
      VkTransformMatrixKHR transform_matrix = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f
      };

      VkAccelerationStructureInstanceKHR as_instance {};
      as_instance.transform = transform_matrix;
      as_instance.instanceCustomIndex = 0;
      as_instance.mask = 0xFF;
      as_instance.instanceShaderBindingTableRecordOffset = 0;
      as_instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
      as_instance.accelerationStructureReference = blas_->device_address;

      auto device = device_->get_device();
      VkBufferUsageFlags usage =
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
      VkMemoryPropertyFlags host_memory_props =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
      VkDeviceSize buffer_size = sizeof(VkAccelerationStructureInstanceKHR);

      // create instances buffer
      instances_buffer_ = std::make_unique<graphics::buffer>(
        *device_,
        buffer_size,
        1,
        usage,
        host_memory_props
      );

      // write the data to the buffer
      instances_buffer_->map();
      instances_buffer_->write_to_buffer((void *) &as_instance);

      // compute required memory size
      VkDeviceOrHostAddressConstKHR instance_data_device_address {};
      instance_data_device_address.deviceAddress = get_device_address(device_->get_device(), instances_buffer_->get_buffer());

      VkAccelerationStructureGeometryKHR as_geometry {};
      as_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
      as_geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
      as_geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
      as_geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
      as_geometry.geometry.instances.arrayOfPointers = VK_FALSE;
      as_geometry.geometry.instances.data = instance_data_device_address;

      // get size
      VkAccelerationStructureBuildGeometryInfoKHR as_build_geometry_info {};
      as_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
      as_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
      as_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
      as_build_geometry_info.geometryCount = 1; // only one triangle
      as_build_geometry_info.pGeometries = &as_geometry;

      uint32_t primitive_count = 1;
      VkAccelerationStructureBuildSizesInfoKHR as_build_sizes_info {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR
      };
      vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                              &as_build_geometry_info, &primitive_count, &as_build_sizes_info);

      // create tlas
      tlas_ = create_acceleration_structure_buffer(as_build_sizes_info);

      // create tlas buffer
      VkAccelerationStructureCreateInfoKHR as_create_info {};
      as_create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
      as_create_info.buffer = tlas_->buffer;
      as_create_info.size = as_build_sizes_info.accelerationStructureSize;
      as_create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
      vkCreateAccelerationStructureKHR(device, &as_create_info, nullptr, &tlas_->handle);

      // create scratch buffer
      auto scratch_buffer = create_scratch_buffer(as_build_sizes_info.buildScratchSize);

      // set up for building tlas
      as_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
      as_build_geometry_info.dstAccelerationStructure = tlas_->handle;
      as_build_geometry_info.scratchData.deviceAddress = scratch_buffer->device_address;

      // execute build command
      VkAccelerationStructureBuildRangeInfoKHR as_build_range_info {};
      as_build_range_info.primitiveCount = primitive_count;
      as_build_range_info.primitiveOffset = 0;
      as_build_range_info.firstVertex = 0;
      as_build_range_info.transformOffset = 0;
      std::vector<VkAccelerationStructureBuildRangeInfoKHR*> as_build_range_infos = { &as_build_range_info };

      auto command = device_->begin_one_shot_commands();
      vkCmdBuildAccelerationStructuresKHR(command, 1, &as_build_geometry_info, as_build_range_infos.data());

      VkBufferMemoryBarrier barrier { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
      barrier.buffer = tlas_->buffer;
      barrier.size = VK_WHOLE_SIZE;
      barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.srcAccessMask =
        VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR |
        VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
      barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

      vkCmdPipelineBarrier(
        command,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        0, 0, nullptr, 1, &barrier, 0, nullptr
      );

      device_->end_one_shot_commands(command);

      // destroy scratch buffer
      vkDestroyBuffer(device_->get_device(), scratch_buffer->handle, nullptr);
      vkFreeMemory(device_->get_device(), scratch_buffer->memory, nullptr);
    }

    void create_ray_traced_image()
    {
      // temporary : for format info
      auto extent = window_->get_extent();
      VkSurfaceFormatKHR back_buffer_format = {
        VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
      };
      auto format = back_buffer_format.format;
      auto device = device_->get_device();
      VkImageUsageFlags usage =
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT;
      VkMemoryPropertyFlags device_memory_props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

      // create image
      ray_traced_image_ = create_texture_2d(extent, format, usage, device_memory_props);

      auto command = device_->begin_one_shot_commands();
      ray_traced_image_->set_image_layout_barrier_state(command, VK_IMAGE_LAYOUT_GENERAL);
      device_->end_one_shot_commands(command);
    }

    u_ptr<image_resource> create_texture_2d(VkExtent2D extent, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_props)
    {
      auto res = std::make_unique<image_resource>();

      // create image
      VkImageCreateInfo create_info {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr
      };
      create_info.imageType = VK_IMAGE_TYPE_2D;
      create_info.format = format;
      create_info.extent = {extent.width, extent.height, 1};
      create_info.mipLevels = 1;
      create_info.arrayLayers = 1;
      create_info.samples = VK_SAMPLE_COUNT_1_BIT;
      create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
      create_info.usage = usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
      create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

      VkImage image;
      VkDeviceMemory image_memory;

      device_->create_image_with_info(create_info, memory_props, image, image_memory);

      // create image view
      VkImageViewCreateInfo view_create_info {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr
      };
      view_create_info.image = image;
      view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      view_create_info.format = format;
      view_create_info.components = VkComponentMapping{
        VK_COMPONENT_SWIZZLE_R,
        VK_COMPONENT_SWIZZLE_G,
        VK_COMPONENT_SWIZZLE_B,
        VK_COMPONENT_SWIZZLE_A,
      };
      view_create_info.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

      VkImageView view;
      vkCreateImageView(device_->get_device(), &view_create_info, nullptr, &view);

      res->set_image(image);
      res->set_image_view(view);
      res->set_device_memory(image_memory);

      return res;
    }

    void create_layout()
    {
      descriptor_set_layout_ = graphics::descriptor_set_layout::builder(*device_)
        .add_binding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR)
        .add_binding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR)
        .build();

      VkPipelineLayoutCreateInfo pl_create_info {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
      };
      pl_create_info.setLayoutCount = 1;
      pl_create_info.pSetLayouts = descriptor_set_layout_->get_p_descriptor_set_layout();
      vkCreatePipelineLayout(device_->get_device(), &pl_create_info, nullptr, &pipeline_layout_);
    }

    void create_pipeline()
    {
      auto ray_generation_stage = load_shader("ray_generation.rgen.spv", VK_SHADER_STAGE_RAYGEN_BIT_KHR);
      auto miss_stage = load_shader("miss.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR);
      auto closest_hit_stage = load_shader("closest_hit.rchit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);

      std::vector<VkPipelineShaderStageCreateInfo> stages = {
        ray_generation_stage,
        miss_stage,
        closest_hit_stage,
      };

      const int index_ray_generation = 0;
      const int index_miss = 1;
      const int index_closest_hit = 2;

      // create shader group
      auto ray_generation_group = VkRayTracingShaderGroupCreateInfoKHR {
        VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR
      };
      ray_generation_group.generalShader = index_ray_generation;
      ray_generation_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
      ray_generation_group.closestHitShader = VK_SHADER_UNUSED_KHR;
      ray_generation_group.anyHitShader = VK_SHADER_UNUSED_KHR;
      ray_generation_group.intersectionShader = VK_SHADER_UNUSED_KHR;

      auto miss_group = VkRayTracingShaderGroupCreateInfoKHR {
        VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR
      };
      miss_group.generalShader = index_miss;
      miss_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
      miss_group.closestHitShader = VK_SHADER_UNUSED_KHR;
      miss_group.anyHitShader = VK_SHADER_UNUSED_KHR;
      miss_group.intersectionShader = VK_SHADER_UNUSED_KHR;

      auto closest_hit_group = VkRayTracingShaderGroupCreateInfoKHR {
        VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR
      };
      closest_hit_group.generalShader = VK_SHADER_UNUSED_KHR;
      closest_hit_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
      closest_hit_group.closestHitShader = index_closest_hit;
      closest_hit_group.anyHitShader = VK_SHADER_UNUSED_KHR;
      closest_hit_group.intersectionShader = VK_SHADER_UNUSED_KHR;

      // shader_group_create_infos_.resize(static_cast<size_t>(shader_stages::MAX_STAGE));
      shader_group_create_infos_.resize(3);
      shader_group_create_infos_[static_cast<int>(shader_stages::RAY_GENERATION)] = ray_generation_group;
      shader_group_create_infos_[static_cast<int>(shader_stages::MISS)] = miss_group;
      shader_group_create_infos_[static_cast<int>(shader_stages::CLOSEST_HIT)] = closest_hit_group;

      // create pipeline
      VkRayTracingPipelineCreateInfoKHR pipeline_create_info {
        VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR
      };
      pipeline_create_info.stageCount = uint32_t(stages.size());
      pipeline_create_info.pStages = stages.data();
      pipeline_create_info.groupCount = uint32_t(shader_group_create_infos_.size());
      pipeline_create_info.pGroups = shader_group_create_infos_.data();
      pipeline_create_info.maxPipelineRayRecursionDepth = 1;
      pipeline_create_info.layout = pipeline_layout_;
      vkCreateRayTracingPipelinesKHR(
        device_->get_device(),
        VK_NULL_HANDLE,
        VK_NULL_HANDLE,
        1,
        &pipeline_create_info,
        nullptr,
        &pipeline_
        );

      // delete shader modules
      for (auto& stage : stages) {
        vkDestroyShaderModule(device_->get_device(), stage.module, nullptr);
      }
    }

    VkPipelineShaderStageCreateInfo load_shader(const char* shader_name, VkShaderStageFlagBits stage)
    {
      std::string shaders_directory = std::string(std::getenv("HNLL_ENGN"))
        + std::string("/modules/graphics/ray_tracing_shader/spv/");

      VkPipelineShaderStageCreateInfo shader_create_info {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr
      };

      auto shader_spv = graphics::pipeline::read_file(shaders_directory + shader_name);
      VkShaderModuleCreateInfo module_create_info {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr
      };
      module_create_info.codeSize = uint32_t(shader_spv.size());
      module_create_info.pCode = reinterpret_cast<const uint32_t*>(shader_spv.data());

      VkShaderModule shader_module;
      vkCreateShaderModule(device_->get_device(), &module_create_info, nullptr, &shader_module);

      shader_create_info.stage = stage;
      shader_create_info.pName = "main";
      shader_create_info.module = shader_module;

      return shader_create_info;
    }

    void create_shader_binding_table()
    {
      auto memory_props =
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
      auto usage =
        VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT; // additional
      const auto pipeline_props = get_ray_tracing_pipeline_properties();

      // compute size of each entry
      const auto handle_size = pipeline_props.shaderGroupHandleSize;
      const auto handle_alignment = pipeline_props.shaderGroupHandleAlignment;
      auto shader_entry_size = align(handle_size, handle_alignment);

      const auto raygen_shader_count = 1;
      const auto miss_shader_count = 1;
      const auto hit_shader_count = 1;

      const auto base_align = pipeline_props.shaderGroupBaseAlignment;
      auto region_raygen = align(shader_entry_size * raygen_shader_count, base_align);
      auto region_miss   = align(shader_entry_size * miss_shader_count, base_align);
      auto region_hit    = align(shader_entry_size * hit_shader_count, base_align);

      shader_binding_table_ = std::make_unique<graphics::buffer>(
        *device_,
        region_raygen + region_miss + region_hit,
        1,
        usage,
        memory_props,
        0
      );

      // get shader group handle
      auto handle_size_aligned = align(handle_size, handle_alignment);
      auto handle_storage_size = shader_group_create_infos_.size() * handle_size_aligned;
      std::vector<uint8_t> shader_handle_storage(handle_storage_size);
      vkGetRayTracingShaderGroupHandlesKHR(
        device_->get_device(),
        pipeline_,
        0,
        uint32_t(shader_group_create_infos_.size()),
        shader_handle_storage.size(),
        shader_handle_storage.data()
      );

      // write raygen shader entry
      auto device_address = get_device_address(device_->get_device(), shader_binding_table_->get_buffer());
      shader_binding_table_->map(VK_WHOLE_SIZE, 0);
      auto dst = static_cast<uint8_t*>(shader_binding_table_->get_mapped_memory());

      auto raygen = shader_handle_storage.data() + handle_size_aligned * static_cast<int>(shader_stages::RAY_GENERATION);
      memcpy(dst, raygen, handle_size);
      dst += region_raygen;
      region_raygen_.deviceAddress = device_address;
      region_raygen_.stride = shader_entry_size;
      region_raygen_.size = region_raygen_.stride;

      // write miss shader entry
      auto miss = shader_handle_storage.data() + handle_size_aligned * static_cast<int>(shader_stages::MISS);
      memcpy(dst, miss, handle_size);
      dst += region_miss;
      region_miss_.deviceAddress = device_address + region_raygen;
      region_miss_.size = region_miss;
      region_miss_.stride = shader_entry_size;

      // write hit shader entry
      auto hit = shader_handle_storage.data() + handle_size_aligned * static_cast<int>(shader_stages::CLOSEST_HIT);
      memcpy(dst, hit, handle_size);
      dst += region_hit;
      region_hit_.deviceAddress = device_address + region_raygen + region_miss;
      region_hit_.size = region_hit;
      region_hit_.stride = shader_entry_size;
    }

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR get_ray_tracing_pipeline_properties()
    {
      // get data for size and order of shader entries
      VkPhysicalDeviceRayTracingPipelinePropertiesKHR pipeline_properties {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR
      };
      VkPhysicalDeviceProperties2 device_properties2 {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
      };
      device_properties2.pNext = &pipeline_properties;
      vkGetPhysicalDeviceProperties2(device_->get_physical_device(), &device_properties2);
      return pipeline_properties;
    }

    void create_descriptor_set()
    {
      // create descriptor pool
      descriptor_pool_ = graphics::descriptor_pool::builder(*device_)
        .set_max_sets(100)
        .add_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000)
        .add_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000)
        .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000)
        .add_pool_size(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 100)
        .add_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000)
        .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000)
        .set_pool_flags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
        .build();

      // write
      VkWriteDescriptorSetAccelerationStructureKHR as_info {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR
      };
      as_info.accelerationStructureCount = 1;
      as_info.pAccelerationStructures = &tlas_->handle;

      VkDescriptorImageInfo image_info {};
      image_info.imageView = ray_traced_image_->get_image_view();
      image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

      graphics::descriptor_writer(*descriptor_set_layout_, *descriptor_pool_)
        .write_acceleration_structure(0, &as_info)
        .write_image(1, &image_info)
        .build(descriptor_set_);
    }

    VkDescriptorSet allocate_descriptor_set(VkDescriptorSetLayout layout, const void* pNext = nullptr)
    {
      VkDescriptorSetAllocateInfo allocate_info {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr
      };
//      allocate_info.descriptorPool =
      allocate_info.pSetLayouts = &layout;
      allocate_info.descriptorSetCount = 1;
      allocate_info.pNext = pNext;
      VkDescriptorSet descriptor_set;
      if (vkAllocateDescriptorSets(device_->get_device(), &allocate_info, &descriptor_set) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor set");
      }
      return descriptor_set;
    }

    void destroy_acceleration_structure(acceleration_structure& as)
    {
      auto device = device_->get_device();
      vkDestroyAccelerationStructureKHR(device, as.handle, nullptr);
      vkFreeMemory(device, as.memory, nullptr);
      vkDestroyBuffer(device, as.buffer, nullptr);
    }

    void destroy_image_resource(image_resource& ir)
    {
      auto device = device_->get_device();
      vkDestroyImage(device, ir.get_image(), nullptr);
      vkDestroyImageView(device, ir.get_image_view(), nullptr);
      vkFreeMemory(device, ir.get_memory(), nullptr);
    }

    // ----------------------------------------------------------------------------------------------
    // variables
    u_ptr<graphics::window> window_;
    u_ptr<graphics::device> device_;
    u_ptr<graphics::buffer> vertex_buffer_;
    u_ptr<graphics::buffer> instances_buffer_;
    u_ptr<image_resource>   ray_traced_image_;

    std::vector<vec3> triangle_vertices_ = {
        {-0.5f, -0.5f, 0.0f},
        {+0.5f, -0.5f, 0.0f},
        {0.0f,  0.75f, 0.0f}
    };

    // acceleration structure
    u_ptr<acceleration_structure> blas_;
    u_ptr<acceleration_structure> tlas_;

    VkPipelineLayout      pipeline_layout_;
    VkPipeline            pipeline_;

    u_ptr<graphics::descriptor_set_layout> descriptor_set_layout_;
    u_ptr<graphics::descriptor_pool>       descriptor_pool_;
    VkDescriptorSet       descriptor_set_;

    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_group_create_infos_;
    u_ptr<graphics::buffer> shader_binding_table_;

    VkStridedDeviceAddressRegionKHR region_raygen_;
    VkStridedDeviceAddressRegionKHR region_miss_;
    VkStridedDeviceAddressRegionKHR region_hit_;
};
}

int main() {
  hnll::hello_triangle app {};

  try { app.run(); }
  catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

// empty
#include <geometry/mesh_model.hpp>
void hnll::geometry::mesh_model::align_vertex_id() {}
