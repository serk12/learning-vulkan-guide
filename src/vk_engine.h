﻿// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>
#include <vector>
#include <deque>
#include <functional>

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void pushFunction(std::function<void()> &&function_) {
		deletors.push_back(function_);
	}

	void flush() {
		for (auto it = deletors.rbegin(); it != deletors.rend(); ++it){
			(*it)();
		}
		deletors.clear();
	}
};

class PipelineBuilder {
public:
	std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
	VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
	VkViewport _viewport;
	VkRect2D _scissor;
	VkPipelineRasterizationStateCreateInfo _rasterizer;
	VkPipelineColorBlendAttachmentState _colorBlendAttachment;
	VkPipelineMultisampleStateCreateInfo _multisampling;
	VkPipelineLayout _pipelineLayout;

	VkPipeline buildPipeline(VkDevice device_, VkRenderPass pass_);
};

class VulkanEngine {
public:
	//Vulkan instance, debug and device
	VkInstance _instance;						// vulkan library handle
	VkDebugUtilsMessengerEXT _debugMessenger;	// vulkan debug output handle
	VkPhysicalDevice _chosenGPU;				// GPU chosen as the default device
	VkDevice _device;							// vulkan device for commands
	VkSurfaceKHR _surface; 						// vulkan windows surface
	//vulkan swapchain
	VkSwapchainKHR _swapchain;	
	VkFormat _swapchainImageFormat; 			// image format expected for the windowing system
	std::vector<VkImage> _swapchainImages;		// array of image from the swapchain
	std::vector<VkImageView> _swapchainImageViews;//array of image view from the swapchain
	//vulkan commands
	VkQueue _graphicsQueue;						// queue to submit requests
	uint32_t _graphicsQueueFamily;				// family of that queue
	VkCommandPool _commandPool;					// the command pool for our commands
	VkCommandBuffer _mainCommandBuffer;			// the buffer to record into
	//vlukna renderpass
	VkRenderPass _renderPass;
	std::vector<VkFramebuffer> _frameBuffers;
	//mainloop semaphore/fence
	VkSemaphore _presentSemaphore, _renderSemaphore;
	VkFence _renderFence;
	//pipeline
	VkPipelineLayout _trianglePipelineLayout;
	VkPipeline _trianglePipeline;
	VkPipeline _redTrianglePipeline;
	//deletor cleanup
	DeletionQueue _mainDeletionQueue;


	bool _isInitialized = false;
	int _frameNumber = 0;
	int _selectedShader = 0;

	VkExtent2D _windowExtent{ 1700 , 900 };

	struct SDL_Window* _window{ nullptr };

	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();
private:
	void initVulkan();
	void initSwapchain();
	void initCommands();
	void initDefaultRenderpass();
	void initFramebuffers();
	void initSyncStructures();
	void initPipeline();

	bool loadShaderModule(const char* filePath_, VkShaderModule* outShaderModule_); 
};
