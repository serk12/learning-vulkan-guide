// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>

namespace vkinit {
	//Command
	VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex_, VkCommandPoolCreateFlags flags_ = 0);
	VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool_, uint32_t count_ = 1, VkCommandBufferLevel level_ = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	//pipeline
	VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage_, VkShaderModule shaderModule_);
	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info();
	VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info(VkPrimitiveTopology topology_);
	VkPipelineMultisampleStateCreateInfo multisampling_state_create_info();
	VkPipelineRasterizationStateCreateInfo rasterization_state_create_info(VkPolygonMode polygonMode_);
	VkPipelineColorBlendAttachmentState color_blend_attachment_state();
	VkPipelineLayoutCreateInfo pipeline_layout_create_info();

	VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0);
	VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0);
}

