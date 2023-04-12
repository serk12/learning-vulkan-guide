// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>
#include <vector>
#include <deque>
#include <functional>
#include <glm/glm.hpp>
#include <unordered_map>
#include <string>

#include "vk_mesh.h"

struct MeshPushConstants {
	glm::vec4 data;
	glm::mat4 renderMatrix; 
};

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
	VkPipelineDepthStencilStateCreateInfo _depthStencil;

	VkPipeline buildPipeline(VkDevice device_, VkRenderPass pass_);
};

struct Material {
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};

struct RenderObject {
	Mesh* mesh;
	Material* material;
	glm::mat4 transformMatrix;
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
	VkPipelineLayout _meshPipelineLayout;
	VkPipeline _trianglePipeline;
	VkPipeline _redTrianglePipeline;
	VkPipeline _meshPipeline;
	//deletor cleanup
	DeletionQueue _mainDeletionQueue;
	//GPU memory alocator
	VmaAllocator _allocator;
	//mesh
	Mesh _triangleMesh;
	Mesh _monkeyMesh;
	//depth
	VkImageView _depthImageView;
	AllocatedImage _depthImage;
	VkFormat _depthFormat;

	std::vector<RenderObject> _renderables;
	std::unordered_map<std::string,Material> _materials;
	std::unordered_map<std::string,Mesh> _meshes;


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

	Material* createMaterial(VkPipeline pipeline, VkPipelineLayout layout,const std::string& name);
	//returns nullptr if it can't be found
	Material* getMaterial(const std::string& name);
	//returns nullptr if it can't be found
	Mesh* getMesh(const std::string& name);
	//our draw function
	void drawObjects(VkCommandBuffer cmd,RenderObject* first, int count);

private:
	void initVulkan();
	void initSwapchain();
	void initCommands();
	void initDefaultRenderpass();
	void initFramebuffers();
	void initSyncStructures();
	void initPipeline();

	bool loadShaderModule(const char* filePath_, VkShaderModule* outShaderModule_); 
	void loadMeshes();
	void initScene();

	void uploadMesh(Mesh &mesh_);
};
