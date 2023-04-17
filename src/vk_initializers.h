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

	VkImageCreateInfo image_create_info(VkFormat format_, VkImageUsageFlags usageFlags_, VkExtent3D extent_);
	VkImageViewCreateInfo imageview_create_info(VkFormat format_, VkImage image_, VkImageAspectFlags aspectFlags_);
	VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp);
	VkRenderPassBeginInfo renderpass_begin_info(VkRenderPass renderPass, VkExtent2D windowExtent, VkFramebuffer framebuffer);

	VkDescriptorSetLayoutBinding descriptorset_layout_binding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);
	VkWriteDescriptorSet write_descriptor_buffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, uint32_t binding);

}

