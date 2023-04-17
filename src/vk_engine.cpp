
#include "vk_engine.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vk_initializers.h>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include <vk_textures.h>

#include <iostream>
#include <fstream>

//bootstrap library
#include <VkBootstrap.h>
#include <glm/gtx/transform.hpp>

#define VK_CHECK(x) 																\
	do 																				\
	{ 																				\
		VkResult err = x; 															\
		if (err) { 																	\
			std::cerr << "Detected error in vulkan. Error: " << err << std::endl;	\
			printf("error at line number %d in file %s\n", __LINE__, __FILE__);		\
			abort(); 																\
		} 																			\
	} while (false) 																\

VkPipeline PipelineBuilder::buildPipeline(VkDevice device_, VkRenderPass pass_)
{
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;

	viewportState.viewportCount = 1;
	viewportState.pViewports = &_viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &_scissor;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.pNext = nullptr;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &_colorBlendAttachment;

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;

	pipelineInfo.stageCount = _shaderStages.size();
	pipelineInfo.pStages = _shaderStages.data();

	pipelineInfo.pVertexInputState = &_vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &_inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &_rasterizer;
	pipelineInfo.pMultisampleState = &_multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = _pipelineLayout;
	pipelineInfo.renderPass = pass_;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.pDepthStencilState = &_depthStencil;	

	VkPipeline newPipeline;
	if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
		std::cerr << "error creating the pipeline" << std::endl;
		return VK_NULL_HANDLE;
	} else {
		return newPipeline;		
	}
}

void VulkanEngine::init()
{
	// We initialize SDL and create a window with it. 
	SDL_Init(SDL_INIT_VIDEO);

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);
	
	_window = SDL_CreateWindow(
		"Vulkan Engine",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		_windowExtent.width,
		_windowExtent.height,
		window_flags
	);
	
	initVulkan();
	initSwapchain();
	initCommands();
	initDefaultRenderpass();
	initFramebuffers();
	initSyncStructures();
	initDescriptors();
	initPipeline();
	std::cout << "loading" << std::endl;
	loadImages();
	std::cout << "loading" << std::endl;
	loadMeshes();
	initScene();
	//everything went fine
	_isInitialized = true;
}

void VulkanEngine::initDescriptors()
{
	std::vector<VkDescriptorPoolSize> sizes = { 
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
		//add combined-image-sampler descriptor types to the pool
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 }
	};
	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = 0;
	poolInfo.maxSets = 10;
	poolInfo.poolSizeCount = (uint32_t)sizes.size();
	poolInfo.pPoolSizes = sizes.data();
	vkCreateDescriptorPool(_device, &poolInfo, nullptr, &_descriptorPool);

	//binding for object
	VkDescriptorSetLayoutBinding objectBind = vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
	VkDescriptorSetLayoutCreateInfo set2info = {};
	set2info.bindingCount = 1;
	set2info.flags = 0;
	set2info.pNext = nullptr;
	set2info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set2info.pBindings = &objectBind;
	vkCreateDescriptorSetLayout(_device, &set2info, nullptr, &_objectSetLayout);
 

	//binding for camera data at 0
	VkDescriptorSetLayoutBinding camBufferBinding = vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,VK_SHADER_STAGE_VERTEX_BIT,0);
	//binding for scene data at 1
	VkDescriptorSetLayoutBinding sceneBind = vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);
	VkDescriptorSetLayoutCreateInfo setinfo = {};
	setinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setinfo.pNext = nullptr;

	setinfo.bindingCount = 2;
	setinfo.flags = 0;
	VkDescriptorSetLayoutBinding bindings[] = { camBufferBinding,sceneBind };
	setinfo.pBindings = &bindings[0];
	vkCreateDescriptorSetLayout(_device, &setinfo, nullptr, &_globalSetLayout);

	VkDescriptorSetLayoutBinding textureBind = vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);

	VkDescriptorSetLayoutCreateInfo set3info = {};
	set3info.bindingCount = 1;
	set3info.flags = 0;
	set3info.pNext = nullptr;
	set3info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set3info.pBindings = &textureBind;

	vkCreateDescriptorSetLayout(_device, &set3info, nullptr, &_singleTextureSetLayout);


	const size_t sceneParamBufferSize = FRAME_OVERLAP * padUniformBufferSize(sizeof(GPUSceneData));
	_sceneParameterBuffer = createBuffer(sceneParamBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	for (int i = 0; i < FRAME_OVERLAP; ++i) {
		_frames[i]._objectBuffer = createBuffer(sizeof(GPUObjectData) * MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		_frames[i]._cameraBuffer = createBuffer(sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.pNext = nullptr;
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = _descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &_globalSetLayout;
		vkAllocateDescriptorSets(_device, &allocInfo, &_frames[i]._globalDescriptor);


		VkDescriptorSetAllocateInfo objectSetAlloc = {};
		objectSetAlloc.pNext = nullptr;
		objectSetAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		objectSetAlloc.descriptorPool = _descriptorPool;
		objectSetAlloc.descriptorSetCount = 1;
		objectSetAlloc.pSetLayouts = &_objectSetLayout;

		vkAllocateDescriptorSets(_device, &objectSetAlloc, &_frames[i]._objectDescriptor);

		VkDescriptorBufferInfo cameraInfo;
		cameraInfo.buffer = _frames[i]._cameraBuffer._buffer;
		cameraInfo.offset = 0;
		cameraInfo.range = sizeof(GPUCameraData);
		VkWriteDescriptorSet cameraWrite = vkinit::write_descriptor_buffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, _frames[i]._globalDescriptor,&cameraInfo,0);

		VkDescriptorBufferInfo sceneInfo;	
		sceneInfo.buffer = _sceneParameterBuffer._buffer;
		sceneInfo.offset = 0;
		sceneInfo.range = sizeof(GPUSceneData);
		VkWriteDescriptorSet sceneWrite = vkinit::write_descriptor_buffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, _frames[i]._globalDescriptor, &sceneInfo, 1);


		VkDescriptorBufferInfo objectBufferInfo;
		objectBufferInfo.buffer = _frames[i]._objectBuffer._buffer;
		objectBufferInfo.offset = 0;
		objectBufferInfo.range = sizeof(GPUObjectData) * MAX_OBJECTS;
		VkWriteDescriptorSet objectWrite = vkinit::write_descriptor_buffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, _frames[i]._objectDescriptor, &objectBufferInfo, 0);


		VkWriteDescriptorSet setWrites[] = { cameraWrite, sceneWrite, objectWrite };
		vkUpdateDescriptorSets(_device, 3, &setWrites[0], 0, nullptr);

		_mainDeletionQueue.pushFunction([&]() {
			vmaDestroyBuffer(_allocator, _frames[i]._objectBuffer._buffer, _frames[i]._objectBuffer._allocation);
			vmaDestroyBuffer(_allocator, _frames[i]._cameraBuffer._buffer, _frames[i]._cameraBuffer._allocation);
		});
	}

	_mainDeletionQueue.pushFunction([&]() {
		vmaDestroyBuffer(_allocator, _sceneParameterBuffer._buffer, _sceneParameterBuffer._allocation);
		vkDestroyDescriptorSetLayout(_device, _globalSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(_device, _objectSetLayout, nullptr);
		vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);
	});
}

void VulkanEngine::initVulkan()
{
	vkb::InstanceBuilder builder;
	auto instRet = builder.set_app_name("Learning Vulkan")
							.request_validation_layers(true)
							.require_api_version(1, 1, 0)
							.use_default_debug_messenger()
							.build();
	vkb::Instance vkbInst = instRet.value();
	_instance = vkbInst.instance;
	_debugMessenger = vkbInst.debug_messenger;
	SDL_Vulkan_CreateSurface(_window, _instance, &_surface);
	vkb::PhysicalDeviceSelector selector { vkbInst };
	vkb::PhysicalDevice physicalDevice = selector.set_minimum_version(1, 1)
													.set_surface(_surface)
													.select()
													.value();
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };
	VkPhysicalDeviceShaderDrawParametersFeatures shaderDrawParametersFeatures = {};
	shaderDrawParametersFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
	shaderDrawParametersFeatures.pNext = nullptr;
	shaderDrawParametersFeatures.shaderDrawParameters = VK_TRUE;
	vkb::Device vkbDevice = deviceBuilder.add_pNext(&shaderDrawParametersFeatures).build().value();
	_device = vkbDevice.device;
	_chosenGPU = physicalDevice.physical_device;

	// use vkbootstrap to get a Graphics queue
	_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = _chosenGPU;
	allocatorInfo.device = _device;
	allocatorInfo.instance = _instance;
	vmaCreateAllocator(&allocatorInfo, &_allocator);

	_gpuProperties = vkbDevice.physical_device.properties;
	std::cout << "the GPU has a minimum buffer alignment of " << _gpuProperties.limits.minUniformBufferOffsetAlignment << std::endl;
}

void VulkanEngine::initSwapchain()
{
	vkb::SwapchainBuilder swapchainBuilder{_chosenGPU,_device,_surface};
	vkb::Swapchain vkbShawpchain = swapchainBuilder.use_default_format_selection()
													.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
													.set_desired_extent(_windowExtent.width, _windowExtent.height)
													.build()
													.value();
	_swapchain = vkbShawpchain.swapchain;
	_swapchainImages = vkbShawpchain.get_images().value();
	_swapchainImageViews = vkbShawpchain.get_image_views().value();

	_swapchainImageFormat = vkbShawpchain.image_format;

	_mainDeletionQueue.pushFunction([=]() {
		vkDestroySwapchainKHR(_device, _swapchain, nullptr);
	});

	VkExtent3D depthImageExtent = {
		_windowExtent.width,
		_windowExtent.height,
		1
	};

	_depthFormat = VK_FORMAT_D32_SFLOAT;
	VkImageCreateInfo dimgInfo = vkinit::image_create_info(_depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);
	VmaAllocationCreateInfo dimgAllocation = {};
	dimgAllocation.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	dimgAllocation.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vmaCreateImage(_allocator, &dimgInfo, &dimgAllocation, &_depthImage._image, &_depthImage._allocation, nullptr);
	VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(_depthFormat, _depthImage._image, VK_IMAGE_ASPECT_DEPTH_BIT);

	VK_CHECK(vkCreateImageView(_device, &dview_info, nullptr, &_depthImageView));

	//add to deletion queues
	_mainDeletionQueue.pushFunction([=]() {
		vkDestroyImageView(_device, _depthImageView, nullptr);
		vmaDestroyImage(_allocator, _depthImage._image, _depthImage._allocation);
	});
}

void VulkanEngine::initCommands()
{
	VkCommandPoolCreateInfo uploadCommandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily);
	VK_CHECK(vkCreateCommandPool(_device, &uploadCommandPoolInfo, nullptr, &_uploadContext._commandPool));
	_mainDeletionQueue.pushFunction([=]() {
		vkDestroyCommandPool(_device, _uploadContext._commandPool, nullptr);
	});
	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_uploadContext._commandPool, 1);
	VkCommandBuffer cmd;
	VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_uploadContext._commandBuffer));

	VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	
	for (int i = 0; i < FRAME_OVERLAP; ++i) {
		VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

		//allocate the defaults buffer for rendering
		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);
		VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));

		_mainDeletionQueue.pushFunction([=]() {
			vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);
		});
	}
}

void VulkanEngine::initDefaultRenderpass()
{
	/* define depth attachment */
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.flags = 0;
	depthAttachment.format = _depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	/* Define color attachment */
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = _swapchainImageFormat;
	// what we do with the textures?
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	// what we do with stencil?
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	//start the layout options as you wish
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	//but show me an image at the end
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/* create subpass */
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	/* dependency */
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkSubpassDependency depthDependency = {};
	depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	depthDependency.dstSubpass = 0;
	depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depthDependency.srcAccessMask = 0;
	depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkSubpassDependency dependencies[2] = { dependency, depthDependency };

	/* use a render pass info to connect the color attachment and the subpass */
	VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = &attachments[0];

	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies = &dependencies[0];

	VK_CHECK(vkCreateRenderPass(_device, &renderPassInfo, nullptr, &_renderPass));
	_mainDeletionQueue.pushFunction([=]() {
		vkDestroyRenderPass(_device,_renderPass, nullptr);
	});
}

void VulkanEngine::initFramebuffers()
{
	VkFramebufferCreateInfo fbInfo = {};
	fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbInfo.pNext = nullptr;

	fbInfo.renderPass = _renderPass;
	fbInfo.attachmentCount = 1;
	fbInfo.width = _windowExtent.width;
	fbInfo.height = _windowExtent.height;
	fbInfo.layers = 1;

	//how many frames we need (depends on the swapchain images)
	const uint32_t swapchainImagecount = _swapchainImages.size();
	_frameBuffers = std::vector<VkFramebuffer>(swapchainImagecount);

	//create a fb for each swapchain
	for (int i = 0; i < _swapchainImages.size(); ++i) {
		VkImageView attachments[2] = { _swapchainImageViews[i], _depthImageView};
		fbInfo.pAttachments = &attachments[0];
		fbInfo.attachmentCount = 2;

		VK_CHECK(vkCreateFramebuffer(_device, &fbInfo, nullptr, &_frameBuffers[i]));

		_mainDeletionQueue.pushFunction([=]() {
			vkDestroyFramebuffer(_device, _frameBuffers[i], nullptr);
			vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
		});
	}
}

void VulkanEngine::initSyncStructures()
{
	VkFenceCreateInfo uploadFenceCreateInfo = vkinit::fence_create_info();
	VK_CHECK(vkCreateFence(_device, &uploadFenceCreateInfo, nullptr, &_uploadContext._uploadFence));
	_mainDeletionQueue.pushFunction([=]() {
		vkDestroyFence(_device, _uploadContext._uploadFence, nullptr);
	});

	VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();
	for (int i = 0; i < FRAME_OVERLAP; ++i) {
		VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));
		_mainDeletionQueue.pushFunction([=]() {
			vkDestroyFence(_device, _frames[i]._renderFence, nullptr);
		});

		VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._presentSemaphore));
		VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));
		_mainDeletionQueue.pushFunction([=]() {
			vkDestroySemaphore(_device, _frames[i]._presentSemaphore, nullptr);
			vkDestroySemaphore(_device, _frames[i]._renderSemaphore, nullptr);
		});
	}
}

void VulkanEngine::initPipeline()
{
	// hardcoded pipeline
	VkShaderModule triangleFragShader;
	if (!loadShaderModule("../shaders/triangle.frag.spv", &triangleFragShader)) {
		std::cerr << "Error with triangle fragment shader" << std::endl;
	} else {
		std::cout << "triangle fragment shader successfully loaded" << std::endl;
	}

	VkShaderModule triangleVertShader;
	if (!loadShaderModule("../shaders/triangle.vert.spv", &triangleVertShader)) {
		std::cerr << "Error with triangle vertex shader" << std::endl;
	} else {
		std::cout << "triangle vertex shader successfully loaded" << std::endl;
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipeline_layout_create_info();
	VK_CHECK(vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_trianglePipelineLayout));

	PipelineBuilder pipelineBuilder;
	pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
	pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(
												VK_SHADER_STAGE_VERTEX_BIT, triangleVertShader));
	pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(
											VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));
	pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();
	pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder._viewport.x = pipelineBuilder._viewport.y = 0.0f;
	pipelineBuilder._viewport.width = (float)_windowExtent.width; 
	pipelineBuilder._viewport.height = (float)_windowExtent.height; 
	pipelineBuilder._viewport.maxDepth = 1.0f;
	pipelineBuilder._viewport.minDepth = 0.0f;

	pipelineBuilder._scissor.offset = {0, 0};
	pipelineBuilder._scissor.extent = _windowExtent;

	pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
	pipelineBuilder._multisampling = vkinit::multisampling_state_create_info();
	pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state();
	pipelineBuilder._pipelineLayout = _trianglePipelineLayout;
	_redTrianglePipeline = pipelineBuilder.buildPipeline(_device, _renderPass);

	// color pipeline
	VkShaderModule triangleColorFragShader;
	if (!loadShaderModule("../shaders/triangle_color.frag.spv", &triangleColorFragShader)) {
		std::cerr << "Error with triangle Color fragment shader" << std::endl;
	} else {
		std::cout << "triangle Color fragment shader successfully loaded" << std::endl;
	}

	VkShaderModule triangleColorVertShader;
	if (!loadShaderModule("../shaders/triangle_color.vert.spv", &triangleColorVertShader)) {
		std::cerr << "Error with triangle Color vertex shader" << std::endl;
	} else {
		std::cout << "triangle Color vertex shader successfully loaded" << std::endl;
	}
	pipelineBuilder._shaderStages.clear();
	pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(
												VK_SHADER_STAGE_VERTEX_BIT, triangleColorVertShader));
	pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(
											VK_SHADER_STAGE_FRAGMENT_BIT, triangleColorFragShader));
	_trianglePipeline = pipelineBuilder.buildPipeline(_device, _renderPass);

	//mesh pipeline
	VkPipelineLayoutCreateInfo meshPipelineLayoutInfo = vkinit::pipeline_layout_create_info();
	VkPushConstantRange pushConstant;
	pushConstant.offset = 0;
	pushConstant.size = sizeof(MeshPushConstants);
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	meshPipelineLayoutInfo.pPushConstantRanges = &pushConstant;
	meshPipelineLayoutInfo.pushConstantRangeCount = 1;

	meshPipelineLayoutInfo.setLayoutCount = 2;
	VkDescriptorSetLayout setLayouts[] = { _globalSetLayout, _objectSetLayout };
	meshPipelineLayoutInfo.pSetLayouts = setLayouts;

	VK_CHECK(vkCreatePipelineLayout(_device, &meshPipelineLayoutInfo, nullptr, &_meshPipelineLayout));

    VertexInputDescription vertexDescriptor = Vertex::getVertexDescription();
    pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = vertexDescriptor.attributes.size();
    pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = vertexDescriptor.attributes.data();

    pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = vertexDescriptor.bindings.size();
    pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = vertexDescriptor.bindings.data();

	VkShaderModule meshVertexShader;
	if (!loadShaderModule("../shaders/mesh.vert.spv", &meshVertexShader)) {
		std::cerr << "Error with mesh vertex shader" << std::endl;
	} else {
		std::cout << "mesh vertex shader successfully loaded" << std::endl;
	}
	VkShaderModule meshFragShader;
	if (!loadShaderModule("../shaders/mesh.frag.spv", &meshFragShader)) {
		std::cerr << "Error with mesh fragment shader" << std::endl;
	} else {
		std::cout << "mesh fragment shader successfully loaded" << std::endl;
	}
	/*
    pipelineBuilder._shaderStages.clear();
	pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(
												VK_SHADER_STAGE_VERTEX_BIT, meshVertexShader));
	pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(
											VK_SHADER_STAGE_FRAGMENT_BIT, meshFragShader));

	pipelineBuilder._pipelineLayout = _meshPipelineLayout;
	_meshPipeline = pipelineBuilder.buildPipeline(_device, _renderPass);

	createMaterial(_meshPipeline, _meshPipelineLayout, "defaultmesh");
	*/
	//connect the new pipeline layout to the pipeline builder

	VkPipelineLayoutCreateInfo textured_pipeline_layout_info = meshPipelineLayoutInfo;

	VkDescriptorSetLayout texturedSetLayouts[] = { _globalSetLayout, _objectSetLayout,_singleTextureSetLayout };

	textured_pipeline_layout_info.setLayoutCount = 3;
	textured_pipeline_layout_info.pSetLayouts = texturedSetLayouts;

	VkPipelineLayout _texturedPipeLayout;
	VK_CHECK(vkCreatePipelineLayout(_device, &textured_pipeline_layout_info, nullptr, &_texturedPipeLayout));

	pipelineBuilder._shaderStages.clear();
	pipelineBuilder._shaderStages.push_back(
		vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, meshVertexShader));

	pipelineBuilder._shaderStages.push_back(
		vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, meshFragShader));

	pipelineBuilder._pipelineLayout = _texturedPipeLayout;
	VkPipeline texPipeline = pipelineBuilder.buildPipeline(_device, _renderPass);
	createMaterial(texPipeline, _texturedPipeLayout, "texturedmesh");


	vkDestroyShaderModule(_device, triangleFragShader, nullptr);
	vkDestroyShaderModule(_device, triangleVertShader, nullptr);
	vkDestroyShaderModule(_device, triangleColorFragShader, nullptr);
	vkDestroyShaderModule(_device, triangleColorVertShader, nullptr);
	vkDestroyShaderModule(_device, meshVertexShader, nullptr);
	vkDestroyShaderModule(_device, meshFragShader, nullptr);
 	_mainDeletionQueue.pushFunction([=]() {
		//destroy the 2 pipelines we have created
		vkDestroyPipeline(_device, _redTrianglePipeline, nullptr);
        vkDestroyPipeline(_device, _trianglePipeline, nullptr);
        vkDestroyPipeline(_device, _meshPipeline, nullptr);
		//destroy the pipeline layout that they use
		vkDestroyPipelineLayout(_device, _trianglePipelineLayout, nullptr);
		vkDestroyPipelineLayout(_device, _meshPipelineLayout, nullptr);
    });

}

void VulkanEngine::loadMeshes() 
{
	/*
	_triangleMesh._vertices.resize(3);
	_triangleMesh._vertices[0].position = { 1.f, 1.f, 0.0f };
	_triangleMesh._vertices[1].position = {-1.f, 1.f, 0.0f };
	_triangleMesh._vertices[2].position = { 0.f,-1.f, 0.0f };

	_triangleMesh._vertices[0].color = { 0.f, 1.f, 0.f };
	_triangleMesh._vertices[1].color = { 0.f, 1.f, 0.f };
	_triangleMesh._vertices[2].color = { 0.f, 1.f, 0.f };

	uploadMesh(_triangleMesh);

	_monkeyMesh.loadFromObj("../assets/monkey_smooth.obj");
	uploadMesh(_monkeyMesh);

	_meshes["monkey"] = _monkeyMesh;
	_meshes["triangle"] = _triangleMesh;
	*/
	Mesh lostEmpire{};
	lostEmpire.loadFromObj("../assets/lost_empire.obj");
	uploadMesh(lostEmpire);
	_meshes["empire"] = lostEmpire;

}

void VulkanEngine::initScene()
{
	/*
	RenderObject monkey;
	monkey.mesh = getMesh("monkey");
	monkey.material = getMaterial("defaultmesh");
	monkey.transformMatrix = glm::mat4{ 1.0f };

	_renderables.push_back(monkey);

	for (int x = -20; x <= 20; x++) {
		for (int y = -20; y <= 20; y++) {

			RenderObject tri;
			tri.mesh = getMesh("triangle");
			tri.material = getMaterial("defaultmesh");
			glm::mat4 translation = glm::translate(glm::mat4{ 1.0 }, glm::vec3(x, 0, y));
			glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.2, 0.2, 0.2));
			tri.transformMatrix = translation * scale;

			_renderables.push_back(tri);
		}
	}
	*/

	RenderObject map;
	map.mesh = getMesh("empire");
	map.material = getMaterial("texturedmesh");
	map.transformMatrix = glm::translate(glm::vec3{ 5,-10,0 });

	_renderables.push_back(map);

	//create a sampler for the texture
	VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST);

	VkSampler blockySampler;
	vkCreateSampler(_device, &samplerInfo, nullptr, &blockySampler);



	Material* texturedMat =	getMaterial("texturedmesh");

	//allocate the descriptor set for single-texture to use on the material
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.pNext = nullptr;
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = _descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &_singleTextureSetLayout;

	vkAllocateDescriptorSets(_device, &allocInfo, &texturedMat->textureSet);

	//write to the descriptor set so that it points to our empire_diffuse texture
	VkDescriptorImageInfo imageBufferInfo;
	imageBufferInfo.sampler = blockySampler;
	imageBufferInfo.imageView = _loadedTextures["empire_diffuse"].imageView;
	imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet texture1 = vkinit::write_descriptor_image(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texturedMat->textureSet, &imageBufferInfo, 0);

	vkUpdateDescriptorSets(_device, 1, &texture1, 0, nullptr);

}


void VulkanEngine::uploadMesh(Mesh& mesh_) 
{
	const size_t bufferSize = mesh_._vertices.size() * sizeof(Vertex);
	VkBufferCreateInfo stagingBufferInfo = {};
	stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferInfo.pNext = nullptr;

	stagingBufferInfo.size = bufferSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	AllocatedBuffer stagingBuffer;
	VK_CHECK(vmaCreateBuffer(_allocator, &stagingBufferInfo, &vmaallocInfo, 
			&stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));

	void* data;
	vmaMapMemory(_allocator, stagingBuffer._allocation, &data);
	memcpy(data, mesh_._vertices.data(), mesh_._vertices.size() * sizeof(Vertex));
	vmaUnmapMemory(_allocator, stagingBuffer._allocation);

	VkBufferCreateInfo vertexBufferInfo = {};
	vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferInfo.pNext = nullptr;
	//this is the total size, in bytes, of the buffer we are allocating
	vertexBufferInfo.size = bufferSize;
	//this buffer is going to be used as a Vertex Buffer
	vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	//let the VMA library know that this data should be GPU native
	vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	//allocate the buffer
	VK_CHECK(vmaCreateBuffer(_allocator, &vertexBufferInfo, &vmaallocInfo,
		&mesh_._vertexBuffer._buffer,
		&mesh_._vertexBuffer._allocation,
		nullptr));
	immediateSubmit([=](VkCommandBuffer cmd) {
		VkBufferCopy copy;
		copy.dstOffset = 0;
		copy.srcOffset = 0;
		copy.size = bufferSize;
		vkCmdCopyBuffer(cmd, stagingBuffer._buffer, mesh_._vertexBuffer._buffer, 1, &copy);
	});

	_mainDeletionQueue.pushFunction([=]() {
		vmaDestroyBuffer(_allocator, mesh_._vertexBuffer._buffer, mesh_._vertexBuffer._allocation);
	});
	vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);
}

bool VulkanEngine::loadShaderModule(const char* filePath_, VkShaderModule* outShaderModule_)
{
	std::ifstream file(filePath_, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "error opening file" << std::endl;
		return false;
	}
	size_t fileSize = (size_t)file.tellg();

	std::vector<uint32_t> buffer(fileSize/sizeof(uint32_t));
	file.seekg(0);
	file.read((char*) buffer.data(), fileSize);
	file.close();

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;

	createInfo.codeSize = buffer.size() * sizeof(uint32_t);
	createInfo.pCode = buffer.data();

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		std::cerr << "error creating shader module" << std::endl;
		return false;
	} 
	*outShaderModule_ = shaderModule;
	return true;
}


void VulkanEngine::cleanup()
{	
	if (_isInitialized) {
		vkDeviceWaitIdle(_device);
		_mainDeletionQueue.flush();
		vmaDestroyAllocator(_allocator);
		vkDestroyDevice(_device, nullptr);
		vkDestroySurfaceKHR(_instance, _surface, nullptr);
		vkb::destroy_debug_utils_messenger(_instance, _debugMessenger);
		vkDestroyInstance(_instance, nullptr);
		SDL_DestroyWindow(_window);
	}
}

void VulkanEngine::draw()
{
	VK_CHECK(vkWaitForFences(_device, 1, &getCurrentFrame()._renderFence, true, 1000000000));
	VK_CHECK(vkResetFences(_device, 1, &getCurrentFrame()._renderFence));
	uint32_t swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, getCurrentFrame()._presentSemaphore, nullptr, &swapchainImageIndex));
	VK_CHECK(vkResetCommandBuffer(getCurrentFrame()._mainCommandBuffer, 0));
	VkCommandBuffer cmd = getCurrentFrame()._mainCommandBuffer;
	VkCommandBufferBeginInfo cmdBeginInfo = {};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBeginInfo.pNext = nullptr;

	cmdBeginInfo.pInheritanceInfo = nullptr;
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
	VkClearValue clearValue;
	float flash = abs(sin(_frameNumber / 120.0f));
	clearValue.color = { { 0.0f, 0.0f, flash, 1.0f} };

	VkClearValue depthClear;
	depthClear.depthStencil.depth = 1.0f;

	VkRenderPassBeginInfo rpInfo = vkinit::renderpass_begin_info(_renderPass, _windowExtent, _frameBuffers[swapchainImageIndex]);
	rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpInfo.pNext = nullptr;

	rpInfo.renderPass = _renderPass;
	rpInfo.renderArea.offset.x = 0;
	rpInfo.renderArea.offset.y = 0;
	rpInfo.renderArea.extent = _windowExtent;
	rpInfo.framebuffer = _frameBuffers[swapchainImageIndex];

	rpInfo.clearValueCount = 2;
	VkClearValue clearValues[] = { clearValue, depthClear };
	rpInfo.pClearValues = &clearValues[0];

	vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
	drawObjects(cmd, _renderables.data(), _renderables.size());
	vkCmdEndRenderPass(cmd);
	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit = {};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.pNext = nullptr;
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submit.pWaitDstStageMask = &waitStage;

	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &getCurrentFrame()._presentSemaphore;

	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &getCurrentFrame()._renderSemaphore;
	
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &cmd;

	VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, getCurrentFrame()._renderFence));

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;

	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &_swapchain;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &getCurrentFrame()._renderSemaphore;

	presentInfo.pImageIndices = &swapchainImageIndex;

	VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));
	++_frameNumber;
}

void VulkanEngine::run()
{
	SDL_Event e;
	bool bQuit = false;

	//main loop
	while (!bQuit)
	{
		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			//close the window when user alt-f4s or clicks the X button			
			if (e.type == SDL_QUIT) {
				bQuit = true;
			}
			else if (e.type == SDL_KEYDOWN) {
				if (e.key.keysym.sym == SDLK_SPACE) {
					_selectedShader = (_selectedShader + 1) % 2;
				} else if (e.key.keysym.sym == SDLK_ESCAPE) {
					bQuit = true;
				}
			}
		}

		draw();
	}
}


Material* VulkanEngine::createMaterial(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name)
{
	Material mat;
	mat.pipeline = pipeline;
	mat.pipelineLayout = layout;
	_materials[name] = mat;
	return &_materials[name];
}

Material* VulkanEngine::getMaterial(const std::string& name)
{
	//search for the object, and return nullptr if not found
	auto it = _materials.find(name);
	if (it == _materials.end()) {
		return nullptr;
	}
	else {
		return &(*it).second;
	}
}


Mesh* VulkanEngine::getMesh(const std::string& name)
{
	auto it = _meshes.find(name);
	if (it == _meshes.end()) {
		return nullptr;
	}
	else {
		return &(*it).second;
	}
}


void VulkanEngine::drawObjects(VkCommandBuffer cmd, RenderObject* first, int count)
{
	// scene data
	float framed = _frameNumber / 120.0f;
	_sceneParameters.ambientColor = { sin(framed), 0, cos(framed), 1 };
	char* sceneData;
	vmaMapMemory(_allocator, _sceneParameterBuffer._allocation, (void**)&sceneData);
	int frameIndex = _frameNumber % FRAME_OVERLAP;
	sceneData += padUniformBufferSize(sizeof(GPUSceneData)) * frameIndex;
	memcpy(sceneData, &_sceneParameters, sizeof(GPUSceneData));
	vmaUnmapMemory(_allocator, _sceneParameterBuffer._allocation);

	//make a model view matrix for rendering the object
	//camera view
	glm::vec3 camPos = { 0.f,-6.f,-10.f };

	glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
	//camera projection
	glm::mat4 projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);
	projection[1][1] *= -1;


	GPUCameraData camData {projection, view, projection * view};
	void* data;
	vmaMapMemory(_allocator, getCurrentFrame()._cameraBuffer._allocation, &data);
	memcpy(data, &camData, sizeof(GPUCameraData));
	vmaUnmapMemory(_allocator, getCurrentFrame()._cameraBuffer._allocation);

	//object data
	GPUObjectData* objectData;
	vmaMapMemory(_allocator, getCurrentFrame()._objectBuffer._allocation, (void**)&objectData);
	for (int i = 0; i < count; ++i) {
		objectData[i].modelMatrix = first[i].transformMatrix;
	}
	vmaUnmapMemory(_allocator, getCurrentFrame()._objectBuffer._allocation);

	Mesh* lastMesh = nullptr;
	Material* lastMaterial = nullptr;

	for (int i = 0; i < count; ++i)
	{
		RenderObject& object = first[i];

		//only bind the pipeline if it doesn't match with the already bound one
		if (object.material != lastMaterial) {
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
			lastMaterial = object.material;

			uint32_t uniformOffset = padUniformBufferSize(sizeof(GPUSceneData)) * frameIndex;
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0, 1, &getCurrentFrame()._globalDescriptor, 1, &uniformOffset);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 1, 1, &getCurrentFrame()._objectDescriptor, 0, nullptr);
			if (object.material->textureSet != VK_NULL_HANDLE) {
				//texture descriptor
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 2, 1, &object.material->textureSet, 0, nullptr);
			}
		}


		MeshPushConstants constants;
		constants.renderMatrix = object.transformMatrix;

		//upload the mesh to the GPU via push constants
		vkCmdPushConstants(cmd, object.material->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

		//only bind the mesh if it's a different one from last bind
		if (object.mesh != lastMesh) {
			//bind the mesh vertex buffer with offset 0
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(cmd, 0, 1, &object.mesh->_vertexBuffer._buffer, &offset);
			lastMesh = object.mesh;
		}
		//we can now draw
		vkCmdDraw(cmd, object.mesh->_vertices.size(), 1, 0, i);
	}
}

FrameData& VulkanEngine::getCurrentFrame() { return _frames[_frameNumber % FRAME_OVERLAP]; }

AllocatedBuffer VulkanEngine::createBuffer(size_t allocSize_, VkBufferUsageFlags usage_, VmaMemoryUsage memoryUsage_)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;

	bufferInfo.size = allocSize_;
	bufferInfo.usage = usage_;

	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = memoryUsage_;

	AllocatedBuffer newBuffer;
	VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo, &newBuffer._buffer, &newBuffer._allocation, nullptr));
	return newBuffer;
}

size_t VulkanEngine::padUniformBufferSize(size_t originalSize)
{
	// Calculate required alignment based on minimum device offset alignment
	size_t minUboAlignment = _gpuProperties.limits.minUniformBufferOffsetAlignment;
	size_t alignedSize = originalSize;
	if (minUboAlignment > 0) {
		alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}
	return alignedSize;
}


void VulkanEngine::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	VkCommandBuffer cmd = _uploadContext._commandBuffer;
	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
	function(cmd);
	VK_CHECK(vkEndCommandBuffer(cmd));
	VkSubmitInfo submit = vkinit::submit_info(&cmd);
	VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, _uploadContext._uploadFence));
	vkWaitForFences(_device, 1, &_uploadContext._uploadFence, true, 9999999999);
	vkResetFences(_device, 1, &_uploadContext._uploadFence);
	vkResetCommandPool(_device, _uploadContext._commandPool, 0);
}

void VulkanEngine::loadImages()
{
	Texture lostEmpire;

	vkutil::load_image_from_file(*this, "../assets/lost_empire-RGBA.png", lostEmpire.image);

	VkImageViewCreateInfo imageinfo = vkinit::imageview_create_info(VK_FORMAT_R8G8B8A8_SRGB, lostEmpire.image._image, VK_IMAGE_ASPECT_COLOR_BIT);
	vkCreateImageView(_device, &imageinfo, nullptr, &lostEmpire.imageView);

	_loadedTextures["empire_diffuse"] = lostEmpire;
}