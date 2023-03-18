#include <vk_initializers.h>

VkCommandPoolCreateInfo vkinit::command_pool_create_info(uint32_t queueFamilyIndex_, VkCommandPoolCreateFlags flags_)
{
	VkCommandPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.pNext = nullptr;

	info.queueFamilyIndex = queueFamilyIndex_;
	info.flags = flags_;

	return info;
}

VkCommandBufferAllocateInfo vkinit::command_buffer_allocate_info(VkCommandPool pool_, uint32_t count_, VkCommandBufferLevel level_)
{
	VkCommandBufferAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.pNext = nullptr;

	info.commandPool = pool_;
	info.commandBufferCount = count_;
	info.level = level_;
	return info;
}
