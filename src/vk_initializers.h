// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>

namespace vkinit {
	VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex_, VkCommandPoolCreateFlags flags_ = 0);
	VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool_, uint32_t count_ = 1, VkCommandBufferLevel level_ = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

