#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

//#define VK_DEFINE_FUNCTION(name) extern PFN_##name name
//#define VK_GLOBAL_LEVEL_FUNCTION(name) VK_DEFINE_FUNCTION(name)
//#define VK_INSTANCE_LEVEL_FUNCTION(name) VK_DEFINE_FUNCTION(name)
//#define VK_DEVICE_LEVEL_FUNCTION(name) VK_DEFINE_FUNCTION(name)
//
//#include "VulkanFunctions.h"

#undef VK_DEFINE_FUNCTION

#include "RenderDevice.h"

class TVulkanSwapChain final : public ISwapChain
{
private:
	friend class TVulkanAPI;

	VkSwapchainKHR Handle = VK_NULL_HANDLE;
	VkImage Images[2] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
	VkImageView Views[2] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
};

class TVulkanTexture final : public IGraphicsTexture
{
public:

};

class TVulkanBuffer final : public IGraphicsBuffer
{
public:
	using IGraphicsBuffer::IGraphicsBuffer;
	~TVulkanBuffer();

	VkBuffer ResourceHandle;
	VkDeviceMemory MemoryHandle;
};

class TVulkanAPI final : public IGraphicsAPI
{
public:
	void Init(const IWindow*) override;
	void Done() override;

	IGraphicsBuffer *CreateBuffer(int32 size) override;

	void HelloWorld();

private:
	friend TVulkanBuffer;

	void InitLib();
	void InitInstance();
	void InitDevice();
	void InitBackBuffer(const IWindow*);
	void InitSwapChain();
	void InitCommandPool();

	void DoneLib();
	void DoneBackBuffer();
	void DoneInstance();
	void DoneDevice();
	void DoneSwapChain();
	void DoneCommandPool();

	void ClearColor();
	VkSemaphore CreateSemaphore();
	void DestroyPipeline(VkPipeline pipeline);
	VkPipeline CreatePipeline(VkRenderPass renderPass);
	VkCommandBuffer CreateCommandBuffer();
	void DestroyCommandBuffer(VkCommandBuffer commandBuffer);
	VkRenderPass CreateRenderPass();
	void DestroyRenderPass(VkRenderPass renderPass);
	VkFramebuffer CreateFramebuffer(VkImageView imageView, VkRenderPass renderPass);
	void DestroyFramebuffer(VkFramebuffer framebuffer);
	void UploadVertexData(VkDeviceMemory deviceMemory);
	VkPipelineLayout CreatePipelineLayout();
	void DestroyPipelineLayout(VkPipelineLayout pipelineLayout);
	VkShaderModule CreateShaderModule(const uint32 *shader, uint32 size);
	void DestroyShaderModule(VkShaderModule shaderModule);

	LibraryType Library = VK_NULL_HANDLE;
	VkInstance Instance = VK_NULL_HANDLE;
	VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
	VkDevice Device = VK_NULL_HANDLE;
	VkAllocationCallbacks *Allocator = nullptr;
	VkSurfaceKHR BackBuffer = VK_NULL_HANDLE;
	TVulkanSwapChain SwapChain;

	VkQueue Queues[2] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
	VkCommandPool Pool = VK_NULL_HANDLE;
};