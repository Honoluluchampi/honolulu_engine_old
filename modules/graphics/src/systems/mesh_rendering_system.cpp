// hnll
#include <graphics/systems/mesh_rendering_system.hpp>
#include <game/components/mesh_component.hpp>

// lib
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

//std
#include <stdexcept>
#include <array>
#include <string>

namespace hnll {

// should be compatible with a shader
struct MeshPushConstant
{
  glm::mat4 modelMatrix_m{1.0f};
  // to align data offsets with shader
  glm::mat4 normalMatrix_m{1.0f};
};

MeshRenderingSystem::MeshRenderingSystem
  (device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
  : HveRenderingSystem(device, render_type::SIMPLE)
{ 
  createPipelineLayout(globalSetLayout);
  createPipeline(renderPass);
}

MeshRenderingSystem::~MeshRenderingSystem()
{}

void MeshRenderingSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
{
  // config push constant range
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  // mainly for if you are going to separate ranges for the vertex and fragment shaders
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(MeshPushConstant);

  std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
  pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
  if (vkCreatePipelineLayout(hveDevice_m.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout_m) != VK_SUCCESS)
      throw std::runtime_error("failed to create pipeline layout!");
}

void MeshRenderingSystem::createPipeline(VkRenderPass renderPass)
{
  assert(pipelineLayout_m != nullptr && "cannot create pipeline before pipeline layout");

  PipelineConfigInfo pipelineConfig{};
  HvePipeline::defaultPipelineConfigInfo(pipelineConfig);
  pipelineConfig.renderPass_m = renderPass;
  pipelineConfig.pipelineLayout_m = pipelineLayout_m;
  hvePipeline_m = std::make_unique<HvePipeline>(
      hveDevice_m,
      std::string(std::getenv("HVE_DIR")) + std::string("/shader/spv/simple_shader.vert.spv"), 
      std::string(std::getenv("HVE_DIR")) + std::string("/shader/spv/simple_shader.frag.spv"),
      pipelineConfig);
}


void MeshRenderingSystem::render(FrameInfo frameInfo)
{
  hvePipeline_m->bind(frameInfo.commandBuffer_m);

  vkCmdBindDescriptorSets(
    frameInfo.commandBuffer_m,
    VK_PIPELINE_BIND_POINT_GRAPHICS,
    pipelineLayout_m,
    0, 1,
    &frameInfo.globalDiscriptorSet_m,
    0, nullptr
  );

  for (auto& target : renderTargetMap_m) {
    
    auto obj = dynamic_cast<mesh_component*>(target.second.get());
    if (obj->get_model_sp() == nullptr) continue;
    MeshPushConstant push{};
    // camera projection
    push.modelMatrix_m = obj->get_transform().mat4();
    // automatically converse mat3(normalMatrix_m) to mat4 for shader data alignment
    push.normalMatrix_m = obj->get_transform().normal_matrix();

    vkCmdPushConstants(
        frameInfo.commandBuffer_m,
        pipelineLayout_m, 
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
        0, 
        sizeof(MeshPushConstant), 
        &push);
    obj->get_model_sp()->bind(frameInfo.commandBuffer_m);
    obj->get_model_sp()->draw(frameInfo.commandBuffer_m);
  }
}

} // namespace hve