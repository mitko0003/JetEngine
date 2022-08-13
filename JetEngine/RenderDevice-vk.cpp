#include "Precompiled.h"

#include "FileSystem.h"
#include "WindowContext-nt.h"
#include "RenderDevice.h"
#include "RenderDevice-vk.h"

static constexpr uint32 WindowWidth = 1000u;
static constexpr uint32 WindowHeight = 800u;

// Resources:
// https://software.intel.com/en-us/articles/api-without-secrets-introduction-to-vulkan-preface

enum QueueType
{
	Graphics,
	Present
};

void TVulkanAPI::InitLib()
{
    Library = LoadLibrary("vulkan-1.dll");
    ASSERT(Library != nullptr);

    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(Library, "vkGetInstanceProcAddr"));

#define VK_GLOBAL_LEVEL_FUNCTION(name) PFN_##name name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(nullptr, #name)); ASSERT(name != nullptr)
#	include "VulkanFunctions.h"
#undef VK_GLOBAL_LEVEL_FUNCTION
}

void *VulkanAllocate(void *pUserData, size_t size, size_t aligment, VkSystemAllocationScope allocationScope)
{
    // DebugPrint("%s: alloc(%d, %d)", static_cast<const char*>(pUserData), size, aligment);
    return malloc(size);
}

void *VulkanReallocate(void *pUserData, void *pOriginal, size_t size, size_t aligment, VkSystemAllocationScope allocationScope)
{
    // DebugPrint("%s: realloc(%d, %d)", static_cast<const char*>(pUserData), size, aligment);
    return realloc(pOriginal, size);
}

void VulkanFree(void *pUserData, void *pMemory)
{
    // DebugPrint("%s: free()", static_cast<const char*>(pUserData));
    free(pMemory);
}

void VulkanInternalAllocate(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
{
}

void VulkanInternalFree(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
{
}

VkAllocationCallbacks GetVulkanAllocator()
{
	VkAllocationCallbacks allocationCallbacks;
	allocationCallbacks.pfnAllocation = VulkanAllocate;
	allocationCallbacks.pfnReallocation = VulkanReallocate;
	allocationCallbacks.pfnFree = VulkanFree;
	allocationCallbacks.pfnInternalAllocation = VulkanInternalAllocate;
	allocationCallbacks.pfnInternalFree = VulkanInternalFree;
	return allocationCallbacks;
}

void TVulkanAPI::InitInstance()
{
	uint32 uPropertyCount;
	VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &uPropertyCount, nullptr);
	ASSERT(result == VK_SUCCESS);
	VkExtensionProperties* extensions = ALLOCA(VkExtensionProperties, uPropertyCount);
	result = vkEnumerateInstanceExtensionProperties(nullptr, &uPropertyCount, extensions);
	ASSERT(result == VK_SUCCESS);

	DebugPrint("****** EXTENSION PROPERTIES ******\n");
	for (uint32 i = 0; i < uPropertyCount; ++i)
		DebugPrint("VkExtensionProperty: %s(%d)\n", extensions[i].extensionName, extensions[i].specVersion);

	uPropertyCount = 0;
	result = vkEnumerateInstanceLayerProperties(&uPropertyCount, nullptr);
	ASSERT(result == VK_SUCCESS);
	VkLayerProperties* layers = ALLOCA(VkLayerProperties, uPropertyCount);
	result = vkEnumerateInstanceLayerProperties(&uPropertyCount, layers);
	ASSERT(result == VK_SUCCESS);

	DebugPrint("****** LAYER PROPERTIES ******\n");
	for (uint32 i = 0; i < uPropertyCount; ++i)
		DebugPrint("vkEnumerateInstanceLayerProperties: %s(%d)\n", layers[i].layerName, layers[i].specVersion);

	const char* needed[] = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME
	};

	// TODO: check availability
	VkApplicationInfo applicationInfo;
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pNext = nullptr;
	applicationInfo.pApplicationName = "Tech Demo";
	applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.pEngineName = "JetEngine";
	applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

	const char* layerNames[] = {
		"VK_LAYER_KHRONOS_validation",
	};

	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = nullptr;
	instanceCreateInfo.flags = 0;
	instanceCreateInfo.pApplicationInfo = &applicationInfo;
	instanceCreateInfo.enabledLayerCount = ArrayLength(layerNames);
	instanceCreateInfo.ppEnabledLayerNames = layerNames;
	instanceCreateInfo.enabledExtensionCount = ArrayLength(needed);
	instanceCreateInfo.ppEnabledExtensionNames = needed;

	result = vkCreateInstance(&instanceCreateInfo, Allocator, &Instance);
	ASSERT(result == VK_SUCCESS);

#define VK_INSTANCE_LEVEL_FUNCTION(name) PFN_##name name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(Instance, #name)); ASSERT(name != nullptr)
#	include "VulkanFunctions.h"
#undef VK_INSTANCE_LEVEL_FUNCTION
}

void TVulkanAPI::InitBackBuffer(const IWindow *baseWindow)
{
	VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo;
	win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	win32SurfaceCreateInfo.pNext = nullptr;
	win32SurfaceCreateInfo.flags = 0;

	const auto *window = static_cast<const TWindowNT *>(baseWindow);
	win32SurfaceCreateInfo.hinstance = window->Instance;
	win32SurfaceCreateInfo.hwnd = window->Handle;

	VkResult result = vkCreateWin32SurfaceKHR(Instance, &win32SurfaceCreateInfo, Allocator, &BackBuffer);
	ASSERT(result == VK_SUCCESS);
}

void TVulkanAPI::InitDevice()
{
	uint32 deviceCount;
	VkResult result = vkEnumeratePhysicalDevices(Instance, &deviceCount, nullptr);
	ASSERT(result == VK_SUCCESS);

	VkPhysicalDevice *physicalDevices = ALLOCA(VkPhysicalDevice, deviceCount);
	result = vkEnumeratePhysicalDevices(Instance, &deviceCount, physicalDevices);
	ASSERT(result == VK_SUCCESS);

	int32 physicalDeviceIndex = -1;
	int32 graphicsQueueIndex = -1;
	int32 presentQueueIndex = -1;

	DebugPrint<logVerbose>("****** PHYSICAL DEVICES ******\n");
	for (uint32 deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex)
	{
		const auto &physicalDevice = physicalDevices[deviceIndex];

		uint32 extensionsCount = 0;
		result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionsCount, nullptr);
		ASSERT(result == VK_SUCCESS);

		VkExtensionProperties* extensionProperties = ALLOCA(VkExtensionProperties, extensionsCount);
		result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionsCount, extensionProperties);
		ASSERT(result == VK_SUCCESS);

		DebugPrint<logVerbose>("Device %d:\n", deviceIndex);

		DebugPrint<logVerbose>("VkExtensionProperties = {\n");
		for (uint32 extensionIndex = 0; extensionIndex < extensionsCount; ++extensionIndex)
			DebugPrint<logVerbose>("	%s: %d\n", extensionProperties[extensionIndex].extensionName, extensionProperties[extensionIndex].specVersion);
		DebugPrint<logVerbose>("}\n");

		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;

		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
		vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

		const char* physicalDeviceType[] = {
			"VK_PHYSICAL_DEVICE_TYPE_OTHER",
			"VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU",
			"VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU",
			"VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU",
			"VK_PHYSICAL_DEVICE_TYPE_CPU",
		};

		DebugPrint<logVerbose>("VkPhysicalDeviceProperties.apiVersion = VK_MAKE_VERSION(%d, %d, %d)\n", VK_VERSION_MAJOR(deviceProperties.apiVersion), VK_VERSION_MINOR(deviceProperties.apiVersion), VK_VERSION_PATCH(deviceProperties.apiVersion));
		DebugPrint<logVerbose>("VkPhysicalDeviceProperties.driverVersion = %d\n", deviceProperties.driverVersion);
		DebugPrint<logVerbose>("VkPhysicalDeviceProperties.vendorID = %d\n", deviceProperties.vendorID);
		DebugPrint<logVerbose>("VkPhysicalDeviceProperties.deviceID = %d\n", deviceProperties.deviceID);
		DebugPrint<logVerbose>("VkPhysicalDeviceProperties.deviceType = %s(%d)\n", physicalDeviceType[deviceProperties.deviceType], deviceProperties.deviceType);
		DebugPrint<logVerbose>("VkPhysicalDeviceProperties.deviceName = %s\n", deviceProperties.deviceName);

		uint32 queueCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);

		VkQueueFamilyProperties *queueFamilyProperties = ALLOCA(VkQueueFamilyProperties, queueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueFamilyProperties);

		for (uint32 familyIndex = 0; familyIndex < queueCount; ++familyIndex)
		{
			const auto &queueFamily = queueFamilyProperties[familyIndex];

			VkBool32 presentQueueSupport;
			result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, familyIndex, BackBuffer, &presentQueueSupport);
			ASSERT(result == VK_SUCCESS);

			DebugPrint<logVerbose>("Queue %d: \n", familyIndex);
			DebugPrint<logVerbose>("queueFlags = {\n");
			DebugPrint<logVerbose>("	VK_QUEUE_GRAPHICS_BIT       = %s", queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT       ? "true" : "false");
			DebugPrint<logVerbose>("	VK_QUEUE_COMPUTE_BIT        = %s", queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT        ? "true" : "false");
			DebugPrint<logVerbose>("	VK_QUEUE_TRANSFER_BIT       = %s", queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT       ? "true" : "false");
			DebugPrint<logVerbose>("	VK_QUEUE_SPARSE_BINDING_BIT = %s", queueFamily.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT ? "true" : "false");
			DebugPrint<logVerbose>("	VK_QUEUE_PROTECTED_BIT      = %s", queueFamily.queueFlags & VK_QUEUE_PROTECTED_BIT      ? "true" : "false");
			DebugPrint<logVerbose>("}\n");
			DebugPrint<logVerbose>("queueCount = %d\n", queueFamily.queueFlags);
			DebugPrint<logVerbose>("timestampValidBits = %d\n", queueFamily.timestampValidBits);
			DebugPrint<logVerbose>("minImageTransferGranularity = VkExtent3D { width = %d, height = %d, depth = %d }\n",
				queueFamily.minImageTransferGranularity.width,
				queueFamily.minImageTransferGranularity.height,
				queueFamily.minImageTransferGranularity.depth
			);
			
			if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentQueueSupport)
			{
				physicalDeviceIndex = deviceIndex;
				graphicsQueueIndex = familyIndex;
				presentQueueIndex = familyIndex;
			}
		}
	}
	
	ASSERT(physicalDeviceIndex != -1 && graphicsQueueIndex != -1 && presentQueueIndex != -1);

	float32 queuePriorities[] = { 1.0f };

	VkDeviceQueueCreateInfo deviceQueueCreateInfo = {};
	deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCreateInfo.pNext = nullptr;
	deviceQueueCreateInfo.flags = 0;
	deviceQueueCreateInfo.queueFamilyIndex = graphicsQueueIndex;
	deviceQueueCreateInfo.queueCount = ArrayLength(queuePriorities);
	deviceQueueCreateInfo.pQueuePriorities = queuePriorities;

	const char *extensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = nullptr;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = nullptr;
	deviceCreateInfo.enabledExtensionCount = ArrayLength(extensions);
	deviceCreateInfo.ppEnabledExtensionNames = extensions;
	deviceCreateInfo.pEnabledFeatures = nullptr;

	PhysicalDevice = physicalDevices[physicalDeviceIndex];

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, BackBuffer, &surfaceCapabilities);
	ASSERT(result == VK_SUCCESS);

	DebugPrint<logVerbose>("BackBufferSurfaceCapabilities.supportedUsageFlags = {\n");
	DebugPrint<logVerbose>("	VK_IMAGE_USAGE_TRANSFER_SRC_BIT             = %s\n", surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT ? "true" : "false");
	DebugPrint<logVerbose>("	VK_IMAGE_USAGE_TRANSFER_DST_BIT             = %s\n", surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT ? "true" : "false");
	DebugPrint<logVerbose>("	VK_IMAGE_USAGE_SAMPLED_BIT                  = %s\n", surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_SAMPLED_BIT ? "true" : "false");
	DebugPrint<logVerbose>("	VK_IMAGE_USAGE_STORAGE_BIT                  = %s\n", surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT ? "true" : "false");
	DebugPrint<logVerbose>("	VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT         = %s\n", surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ? "true" : "false");
	DebugPrint<logVerbose>("	VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT = %s\n", surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ? "true" : "false");
	DebugPrint<logVerbose>("	VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT     = %s\n", surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT ? "true" : "false");
	DebugPrint<logVerbose>("	VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT         = %s\n", surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT ? "true" : "false");
	DebugPrint<logVerbose>("}\n");

	result = vkCreateDevice(PhysicalDevice, &deviceCreateInfo, Allocator, &Device);
	ASSERT(result == VK_SUCCESS);

#define VK_DEVICE_LEVEL_FUNCTION(name) PFN_##name name = reinterpret_cast<PFN_##name>(vkGetDeviceProcAddr(Device, #name)); ASSERT(name != nullptr)
#	include "VulkanFunctions.h"
#undef VK_DEVICE_LEVEL_FUNCTION
}

void TVulkanAPI::InitSwapChain()
{
	vkDeviceWaitIdle(Device);

	for (int32 i = 0; i < ArrayLength(SwapChain.Images); ++i) 
	{
		if (SwapChain.Images[i] != VK_NULL_HANDLE) 
		{
			vkDestroyImageView(Device, SwapChain.Views[i], Allocator);
			SwapChain.Images[i] = VK_NULL_HANDLE;
			SwapChain.Views[i] = VK_NULL_HANDLE;
		}
	}

	const auto &GetSwapChainFormat = [&]() -> VkSurfaceFormatKHR {
		uint32 surfaceFormatsCount;
		VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, BackBuffer, &surfaceFormatsCount, nullptr);
		ASSERT(result == VK_SUCCESS && surfaceFormatsCount != 0);

		VkSurfaceFormatKHR *surfaceFormats = ALLOCA(VkSurfaceFormatKHR, surfaceFormatsCount);
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, BackBuffer, &surfaceFormatsCount, surfaceFormats);
		ASSERT(result == VK_SUCCESS);

		if (surfaceFormatsCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
			return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };

		for (uint32 i = 0; i < surfaceFormatsCount; ++i) {
			if (surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_UNORM) {
				ASSERT(surfaceFormats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR);
				return surfaceFormats[i];
			}
		}

		ASSERT(!"Unexpected surface format!");
		return surfaceFormats[0];
	};

	const auto &GetSwapChainExtent = [&]() -> VkExtent2D {
		VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
		VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, BackBuffer, &surfaceCapabilities);
		ASSERT(result == VK_SUCCESS);

		if (surfaceCapabilities.currentExtent.width == -1) {
			return VkExtent2D {
				Clamp(WindowWidth, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
				Clamp(WindowHeight, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height)
			};
		}

		return surfaceCapabilities.currentExtent;
	};

	const auto &GetSwapChainUsageFlags = [&]() -> VkImageUsageFlags {
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, BackBuffer, &surfaceCapabilities);
		ASSERT(result == VK_SUCCESS);

		if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		ASSERT(!"");
		return static_cast<VkImageUsageFlags>(-1);
	};

	const auto &GetSwapChainTransform = [&]() -> VkSurfaceTransformFlagBitsKHR {
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, BackBuffer, &surfaceCapabilities);
		ASSERT(result == VK_SUCCESS);
		return surfaceCapabilities.currentTransform;
	};

	const auto &GetSwapChainPresentMode = [&]() -> VkPresentModeKHR {
		uint32 presentModesCount;
		VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, BackBuffer, &presentModesCount, nullptr);
		ASSERT(result == VK_SUCCESS && presentModesCount != 0);

		VkPresentModeKHR *presentModes = ALLOCA(VkPresentModeKHR, presentModesCount);
		result = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, BackBuffer, &presentModesCount, presentModes);
		ASSERT(result == VK_SUCCESS);

		// MAILBOX is the lowest latency V-Sync enabled mode (something like triple-buffering) so use it if available
		for (uint32 i = 0; i < presentModesCount; ++i) {
			if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
				return presentModes[i];
			}
		}
		// IMMEDIATE mode allows us to display frames in a V-Sync independent manner so it can introduce screen tearing
		// But this mode is the best for benchmarking purposes if we want to check the real number of FPS
		for (uint32 i = 0; i < presentModesCount; ++i) {
			if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
				return presentModes[i];
			}
		}
		// FIFO present mode is always available
		for (uint32 i = 0; i < presentModesCount; ++i) {
			if (presentModes[i] == VK_PRESENT_MODE_FIFO_KHR) {
				return presentModes[i];
			}
		}
		ASSERT(!"Unsupported present mode!");
		return static_cast<VkPresentModeKHR>(-1);
	};

	auto surfaceFormat = GetSwapChainFormat();
	auto extent2D = GetSwapChainExtent();
	auto imageUsageFlags = GetSwapChainUsageFlags();
	auto surfaceTransformFlagBits = GetSwapChainTransform();
	auto presentMode = GetSwapChainPresentMode();
	auto prevSwapchain = SwapChain.Handle;

	ASSERT(static_cast<int32>(imageUsageFlags) != -1);
	ASSERT(static_cast<int32>(presentMode) != -1);
	ASSERT(extent2D.width != 0 || extent2D.height != 0);

	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.pNext = nullptr;
	swapchainCreateInfo.flags = 0;
	swapchainCreateInfo.surface = BackBuffer;
	swapchainCreateInfo.minImageCount = 2;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent = extent2D;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = imageUsageFlags;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.queueFamilyIndexCount = 0;
	swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	swapchainCreateInfo.preTransform = surfaceTransformFlagBits;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = prevSwapchain;

	VkResult result = vkCreateSwapchainKHR(Device, &swapchainCreateInfo, Allocator, &SwapChain.Handle);
	ASSERT(result == VK_SUCCESS);

	if (prevSwapchain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(Device, prevSwapchain, Allocator);

	uint32 imageCount = 0;
	result = vkGetSwapchainImagesKHR(Device, SwapChain.Handle, &imageCount, nullptr);
	ASSERT(result == VK_SUCCESS && imageCount == ArrayLength(SwapChain.Images));

	result = vkGetSwapchainImagesKHR(Device, SwapChain.Handle, &imageCount, SwapChain.Images);
	ASSERT(result == VK_SUCCESS);

	for (int32 i = 0; i < ArrayLength(SwapChain.Images); ++i) {
		VkImageViewCreateInfo imageViewCreateInfo;
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.pNext = nullptr;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.image = SwapChain.Images[i];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = surfaceFormat.format;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		VkResult result = vkCreateImageView(Device, &imageViewCreateInfo, Allocator, &SwapChain.Views[i]); 
		ASSERT(result == VK_SUCCESS);
	}
}

void TVulkanAPI::InitCommandPool()
{
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.pNext = nullptr;
	commandPoolCreateInfo.flags = 0;
	commandPoolCreateInfo.queueFamilyIndex = 0;

	VkResult result = vkCreateCommandPool(Device, &commandPoolCreateInfo, nullptr, &Pool);
	ASSERT(result == VK_SUCCESS);
}

VkCommandBuffer TVulkanAPI::CreateCommandBuffer()
{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo;
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = nullptr;
	commandBufferAllocateInfo.commandPool = Pool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	VkResult result = vkAllocateCommandBuffers(Device, &commandBufferAllocateInfo, &commandBuffer);
	ASSERT(result == VK_SUCCESS);

	return commandBuffer;
}

void TVulkanAPI::DestroyCommandBuffer(VkCommandBuffer commandBuffer)
{
	vkFreeCommandBuffers(Device, Pool, 1, &commandBuffer);
}

VkRenderPass TVulkanAPI::CreateRenderPass()
{
	VkAttachmentDescription attachmentDescription = {};
	attachmentDescription.flags = 0;
	attachmentDescription.format = VK_FORMAT_B8G8R8A8_UNORM;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference attachmentReference = {};
	attachmentReference.attachment = 0;
	attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription = {};
	subpassDescription.flags = 0;
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = nullptr;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &attachmentReference;
	subpassDescription.pResolveAttachments = nullptr;
	subpassDescription.pDepthStencilAttachment = nullptr;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = nullptr;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = nullptr;
	renderPassCreateInfo.flags = 0;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &attachmentDescription;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.dependencyCount = 0;
	renderPassCreateInfo.pDependencies = nullptr;

	VkRenderPass renderPass = VK_NULL_HANDLE;
	VkResult result = vkCreateRenderPass(Device, &renderPassCreateInfo, Allocator, &renderPass);
	ASSERT(result == VK_SUCCESS);
	return renderPass;
}

void TVulkanAPI::DestroyRenderPass(VkRenderPass renderPass)
{
	vkDestroyRenderPass(Device, renderPass, Allocator);
}

VkFramebuffer TVulkanAPI::CreateFramebuffer(VkImageView imageView, VkRenderPass renderPass)
{
	VkFramebufferCreateInfo framebufferCreateInfo = {};
	framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCreateInfo.pNext = nullptr;
	framebufferCreateInfo.flags = 0;
	framebufferCreateInfo.renderPass = renderPass;
	framebufferCreateInfo.attachmentCount = 1;
	framebufferCreateInfo.pAttachments = &imageView;
	framebufferCreateInfo.width = WindowWidth;
	framebufferCreateInfo.height = WindowHeight;
	framebufferCreateInfo.layers = 1;

	VkFramebuffer framebuffer = VK_NULL_HANDLE;
	VkResult result = vkCreateFramebuffer(Device, &framebufferCreateInfo, Allocator, &framebuffer);
	ASSERT(result == VK_SUCCESS);
	return framebuffer;
}

void TVulkanAPI::DestroyFramebuffer(VkFramebuffer framebuffer)
{
	vkDestroyFramebuffer(Device, framebuffer, Allocator);
}

static const float32 VertexData[][2] = {
	{ -0.5, -0.5 },
	{ -0.5,  0.5 },
	{  0.5, -0.5 },
	{  0.5,  0.5 }
};

void TVulkanAPI::UploadVertexData(VkDeviceMemory deviceMemory)
{
	void *mappedMemory = nullptr;
	VkResult result = vkMapMemory(Device, deviceMemory, 0, sizeof(VertexData), 0, &mappedMemory);
	ASSERT(result == VK_SUCCESS);
	MemCopy(mappedMemory, &VertexData, sizeof(VertexData));

	VkMappedMemoryRange mappedMemoryRange = {};
	mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedMemoryRange.pNext = nullptr;
	mappedMemoryRange.memory = deviceMemory;
	mappedMemoryRange.offset = 0;
	mappedMemoryRange.size = VK_WHOLE_SIZE;

	result = vkFlushMappedMemoryRanges(Device, 1, &mappedMemoryRange);
	ASSERT(result == VK_SUCCESS);

	vkUnmapMemory(Device, deviceMemory);
}

IGraphicsBuffer* TVulkanAPI::CreateBuffer(int32 size)
{
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = nullptr;
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount = 0;
	bufferCreateInfo.pQueueFamilyIndices = nullptr;

	VkBuffer resourceHandle = VK_NULL_HANDLE;
	VkResult result = vkCreateBuffer(Device, &bufferCreateInfo, Allocator, &resourceHandle);
	ASSERT(result == VK_SUCCESS);

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(Device, resourceHandle, &memoryRequirements);

	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &physicalDeviceMemoryProperties);

	VkDeviceMemory memoryHandle = VK_NULL_HANDLE;
	for (uint32 i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i)
	{
		if (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
		{
			VkMemoryAllocateInfo memoryAllocateInfo = {};
			memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memoryAllocateInfo.pNext = nullptr;
			memoryAllocateInfo.allocationSize = memoryRequirements.size;
			memoryAllocateInfo.memoryTypeIndex = i;

			VkResult result = vkAllocateMemory(Device, &memoryAllocateInfo, Allocator, &memoryHandle);
			if (result == VK_SUCCESS)
				break;
		}
	}

	auto *buffer = new TVulkanBuffer(this);
	buffer->ResourceHandle = resourceHandle;
	buffer->MemoryHandle = memoryHandle;
	return buffer;
}

TVulkanBuffer::~TVulkanBuffer()
{
	auto *vulkanDevice = static_cast<TVulkanAPI *>(ParentDevice);
	vkFreeMemory(vulkanDevice->Device, MemoryHandle, vulkanDevice->Allocator);
	vkDestroyBuffer(vulkanDevice->Device, ResourceHandle, vulkanDevice->Allocator);
}

VkPipelineLayout TVulkanAPI::CreatePipelineLayout()
{
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = nullptr;
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.setLayoutCount = 0;
	pipelineLayoutCreateInfo.pSetLayouts = nullptr;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	VkResult result = vkCreatePipelineLayout(Device, &pipelineLayoutCreateInfo, Allocator, &pipelineLayout);
	ASSERT(result == VK_SUCCESS);
	return pipelineLayout;
}

void TVulkanAPI::DestroyPipelineLayout(VkPipelineLayout pipelineLayout)
{
	vkDestroyPipelineLayout(Device, pipelineLayout, Allocator);
}

//dxc.exe -spirv -fspv-target-env=vulkan1.1 -T vs_6_0 -E VS_main HelloTriangle.vs.hlsl -Fo HelloTriangle.vs.spirv
//dxc.exe -spirv -fspv-target-env=vulkan1.1 -T ps_6_0 -E PS_main HelloTriangle.ps.hlsl -Fo HelloTriangle.ps.spirv

VkShaderModule TVulkanAPI::CreateShaderModule(const uint32 *shader, uint32 size)
{
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.pNext = nullptr;
	shaderModuleCreateInfo.flags = 0;
	shaderModuleCreateInfo.codeSize = size;
	shaderModuleCreateInfo.pCode = shader;

	VkShaderModule shaderModule = VK_NULL_HANDLE;
	VkResult result = vkCreateShaderModule(Device, &shaderModuleCreateInfo, Allocator, &shaderModule);
	ASSERT(result == VK_SUCCESS);
	return shaderModule;
}

void TVulkanAPI::DestroyShaderModule(VkShaderModule shaderModule)
{
	vkDestroyShaderModule(Device, shaderModule, Allocator);
}

VkPipeline TVulkanAPI::CreatePipeline(VkRenderPass renderPass)
{
	auto ps = FS::Open("Test/HelloTriangle.ps.spirv", FS::Read);
	auto vs = FS::Open("Test/HelloTriangle.vs.spirv", FS::Read);
	auto vertexShaderModule = CreateShaderModule(reinterpret_cast<const uint32*>(vs.Platform.Buffer), uint32(vs.Size));
	auto pixelShaderModule = CreateShaderModule(reinterpret_cast<const uint32*>(ps.Platform.Buffer), uint32(ps.Size));
	FS::Close(ps);
	FS::Close(vs);

	VkPipelineShaderStageCreateInfo shaderStageCreateInfo[2];
	shaderStageCreateInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo[0].pNext = nullptr;
	shaderStageCreateInfo[0].flags = 0;
	shaderStageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStageCreateInfo[0].module = vertexShaderModule;
	shaderStageCreateInfo[0].pName = "VS_main";
	shaderStageCreateInfo[0].pSpecializationInfo = nullptr;

	shaderStageCreateInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo[1].pNext = nullptr;
	shaderStageCreateInfo[1].flags = 0;
	shaderStageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStageCreateInfo[1].module = pixelShaderModule;
	shaderStageCreateInfo[1].pName = "PS_main";
	shaderStageCreateInfo[1].pSpecializationInfo = nullptr;

	VkVertexInputBindingDescription vertexInputBindingDescription = {};
	vertexInputBindingDescription.binding = 0;
	vertexInputBindingDescription.stride = sizeof(float32[2]);
	vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription vertexInputAttributeDescription = {};
	vertexInputAttributeDescription.location = 0;
	vertexInputAttributeDescription.binding = 0;
	vertexInputAttributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
	vertexInputAttributeDescription.offset = 0;
	
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.pNext = nullptr;
	vertexInputStateCreateInfo.flags = 0;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 1;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = &vertexInputAttributeDescription;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo.pNext = nullptr;
	inputAssemblyStateCreateInfo.flags = 0;
	inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = WindowWidth;
	viewport.height = WindowHeight;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 0.0f;

	VkRect2D scissor = {};
	scissor.offset.x = 0u;
	scissor.offset.x = 0u;
	scissor.extent.width = WindowWidth;
	scissor.extent.height = WindowHeight;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.pNext = nullptr;
	viewportStateCreateInfo.flags = 0;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.pNext = nullptr;
	rasterizationStateCreateInfo.flags = 0;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
	rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
	rasterizationStateCreateInfo.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.pNext = nullptr;
	multisampleStateCreateInfo.flags = 0;
	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleStateCreateInfo.minSampleShading = 1.0f;
	multisampleStateCreateInfo.pSampleMask = nullptr;
	multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.blendEnable = VK_FALSE;
	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.pNext = nullptr;
	colorBlendStateCreateInfo.flags = 0;
	colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendStateCreateInfo.attachmentCount = 1;
	colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
	colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[3] = 0.0f;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
	depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCreateInfo.pNext = nullptr;
	depthStencilStateCreateInfo.flags = 0;
	depthStencilStateCreateInfo.depthTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.depthWriteEnable = VK_FALSE;
	depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;

	depthStencilStateCreateInfo.front.failOp = VK_STENCIL_OP_KEEP;
	depthStencilStateCreateInfo.front.passOp = VK_STENCIL_OP_KEEP;
	depthStencilStateCreateInfo.front.depthFailOp = VK_STENCIL_OP_KEEP;
	depthStencilStateCreateInfo.front.compareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilStateCreateInfo.front.compareMask = 0u;
	depthStencilStateCreateInfo.front.writeMask = 0u;
	depthStencilStateCreateInfo.front.reference = 0u;

	depthStencilStateCreateInfo.back.failOp = VK_STENCIL_OP_KEEP;
	depthStencilStateCreateInfo.back.passOp = VK_STENCIL_OP_KEEP;
	depthStencilStateCreateInfo.back.depthFailOp = VK_STENCIL_OP_KEEP;
	depthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilStateCreateInfo.back.compareMask = 0u;
	depthStencilStateCreateInfo.back.writeMask = 0u;
	depthStencilStateCreateInfo.back.reference = 0u;

	depthStencilStateCreateInfo.minDepthBounds = 0.0f;
	depthStencilStateCreateInfo.maxDepthBounds = 1.0f;

	VkPipelineLayout pipelineLayout = CreatePipelineLayout();

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.pNext = nullptr;
	graphicsPipelineCreateInfo.flags = 0;
	graphicsPipelineCreateInfo.stageCount = ArrayLength(shaderStageCreateInfo);
	graphicsPipelineCreateInfo.pStages = shaderStageCreateInfo;
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	graphicsPipelineCreateInfo.pTessellationState = nullptr;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = nullptr;
	graphicsPipelineCreateInfo.layout = pipelineLayout;
	graphicsPipelineCreateInfo.renderPass = renderPass;
	graphicsPipelineCreateInfo.subpass = 0;
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;

	VkPipeline graphicsPipeline = VK_NULL_HANDLE;
	VkResult result = vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, Allocator, &graphicsPipeline);
	ASSERT(result == VK_SUCCESS);

	DestroyShaderModule(vertexShaderModule);
	DestroyShaderModule(pixelShaderModule);
	DestroyPipelineLayout(pipelineLayout);
	return graphicsPipeline;
}

void TVulkanAPI::DestroyPipeline(VkPipeline pipeline)
{
	vkDestroyPipeline(Device, pipeline, Allocator);
}

VkSemaphore TVulkanAPI::CreateSemaphore()
{
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;
	semaphoreCreateInfo.flags = 0;

	VkSemaphore semaphore = VK_NULL_HANDLE;
	VkResult result = vkCreateSemaphore(Device, &semaphoreCreateInfo, Allocator, &semaphore);
	ASSERT(result == VK_SUCCESS);
	return semaphore;
}

static VkSemaphore renderingFinishedSemaphore = VK_NULL_HANDLE;
static VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;

void TVulkanAPI::HelloWorld()
{
	uint32_t i;
	VkResult result = vkAcquireNextImageKHR(Device, SwapChain.Handle, TNumericLimits<uint64>::Max(), imageAvailableSemaphore, VK_NULL_HANDLE, &i);
	ASSERT(result == VK_SUCCESS);

	VkRenderPass renderPass = CreateRenderPass();
	VkFramebuffer framebuffer = CreateFramebuffer(SwapChain.Views[i], renderPass);
	auto *buffer = static_cast<TVulkanBuffer*>(CreateBuffer(sizeof(VertexData)));

	UploadVertexData(buffer->MemoryHandle);
	result = vkBindBufferMemory(Device, buffer->ResourceHandle, buffer->MemoryHandle, 0);
	ASSERT(result == VK_SUCCESS);

	VkPipeline pipeline = CreatePipeline(renderPass);
	VkCommandBuffer commandBuffer = CreateCommandBuffer();

	VkImageSubresourceRange imageSubresourceRange = {};
	imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageSubresourceRange.baseMipLevel = 0;
	imageSubresourceRange.levelCount = 1;
	imageSubresourceRange.baseArrayLayer = 0;
	imageSubresourceRange.layerCount = 1;

	{
		VkCommandBufferBeginInfo commandBufferBeginInfo = {};
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.pNext = nullptr;
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		commandBufferBeginInfo.pInheritanceInfo = nullptr;

		vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
	}

	{
		VkImageMemoryBarrier imageMemoryBarrier = {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.pNext = nullptr;
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imageMemoryBarrier.srcQueueFamilyIndex = 0;
		imageMemoryBarrier.dstQueueFamilyIndex = 0;
		imageMemoryBarrier.image = SwapChain.Images[i];
		imageMemoryBarrier.subresourceRange = imageSubresourceRange;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier
		);
	}

	{
		VkClearValue clearValue = {};
		clearValue.color.float32[0] = 0.0f;
		clearValue.color.float32[1] = 0.0f;
		clearValue.color.float32[2] = 1.0f;
		clearValue.color.float32[3] = 1.0f;
		clearValue.depthStencil.depth = 0.0f;
		clearValue.depthStencil.stencil = 0u;

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.framebuffer = framebuffer;
		renderPassBeginInfo.renderArea.offset.x = 0u;
		renderPassBeginInfo.renderArea.offset.y = 0u;
		renderPassBeginInfo.renderArea.extent.width = WindowWidth;
		renderPassBeginInfo.renderArea.extent.height = WindowHeight;
		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.pClearValues = &clearValue;

		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buffer->ResourceHandle, &offset);
	vkCmdDraw(commandBuffer, 4, 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);

	{
		VkImageMemoryBarrier imageMemoryBarrier = {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.pNext = nullptr;
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.image = SwapChain.Images[i];
		imageMemoryBarrier.subresourceRange = imageSubresourceRange;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier
		);
	}

	vkEndCommandBuffer(commandBuffer);

	{
		VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
		submitInfo.pWaitDstStageMask = &wait_dst_stage_mask;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderingFinishedSemaphore;

		result = vkQueueSubmit(Queues[Present], 1, &submitInfo, VK_NULL_HANDLE);
		ASSERT(result == VK_SUCCESS);
	}
	
	{
		VkPresentInfoKHR presentInfoKHR = {};
		presentInfoKHR.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfoKHR.pNext = nullptr;
		presentInfoKHR.waitSemaphoreCount = 1;
		presentInfoKHR.pWaitSemaphores = &renderingFinishedSemaphore;
		presentInfoKHR.swapchainCount = 1;
		presentInfoKHR.pSwapchains = &SwapChain.Handle;
		presentInfoKHR.pImageIndices = &i;
		presentInfoKHR.pResults = nullptr;

		result = vkQueuePresentKHR(Queues[Present], &presentInfoKHR);
		ASSERT(result == VK_SUCCESS);
	}

	DestroyCommandBuffer(commandBuffer);
	DestroyPipeline(pipeline);
	delete buffer;
	DestroyFramebuffer(framebuffer);
	DestroyRenderPass(renderPass);
}

void TVulkanAPI::ClearColor()
{
	uint32_t i;
	VkResult result = vkAcquireNextImageKHR(Device, SwapChain.Handle, TNumericLimits<uint64>::Max(), imageAvailableSemaphore, VK_NULL_HANDLE, &i);

	static VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	if (commandBuffer != VK_NULL_HANDLE)
		DestroyCommandBuffer(commandBuffer);
	commandBuffer = CreateCommandBuffer();
		
	VkImageSubresourceRange imageSubresourceRange = {};
	imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageSubresourceRange.baseMipLevel = 0;
	imageSubresourceRange.levelCount = 1;
	imageSubresourceRange.baseArrayLayer = 0;
	imageSubresourceRange.layerCount = 1;
	
	{
		VkCommandBufferBeginInfo commandBufferBeginInfo = {};
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.pNext = nullptr;
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		commandBufferBeginInfo.pInheritanceInfo = nullptr;

		vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
	}

	{
		VkImageMemoryBarrier imageMemoryBarrier = {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.pNext = nullptr;
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imageMemoryBarrier.srcQueueFamilyIndex = 0;
		imageMemoryBarrier.dstQueueFamilyIndex = 0;
		imageMemoryBarrier.image = SwapChain.Images[i];
		imageMemoryBarrier.subresourceRange = imageSubresourceRange;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr, 
			1, &imageMemoryBarrier
		);
	}

	{
		VkClearColorValue clearColorValue = {};
		clearColorValue.float32[0] = 0.0f;
		clearColorValue.float32[1] = i ? 1.0f : 0.0f;
		clearColorValue.float32[2] = 1.0f;
		clearColorValue.float32[3] = 1.0f;

		vkCmdClearColorImage(
			commandBuffer, 
			SwapChain.Images[i], 
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
			&clearColorValue,
			1, &imageSubresourceRange
		);
	}

	{
		VkImageMemoryBarrier imageMemoryBarrier = {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.pNext = nullptr;
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.image = SwapChain.Images[i];
		imageMemoryBarrier.subresourceRange = imageSubresourceRange;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0, 
			0, nullptr, 
			0, nullptr,
			1, &imageMemoryBarrier
		);
	}

	result = vkEndCommandBuffer(commandBuffer);
	ASSERT(result == VK_SUCCESS);
	
	{
		VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
		submitInfo.pWaitDstStageMask = &wait_dst_stage_mask;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderingFinishedSemaphore;

		result = vkQueueSubmit(Queues[Present], 1, &submitInfo, VK_NULL_HANDLE);
		ASSERT(result == VK_SUCCESS);
	}

	{
		VkPresentInfoKHR presentInfoKHR = {};
		presentInfoKHR.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfoKHR.pNext = nullptr;
		presentInfoKHR.waitSemaphoreCount = 1;
		presentInfoKHR.pWaitSemaphores = &renderingFinishedSemaphore;
		presentInfoKHR.swapchainCount = 1;
		presentInfoKHR.pSwapchains = &SwapChain.Handle;
		presentInfoKHR.pImageIndices = &i;
		presentInfoKHR.pResults = nullptr;

		result = vkQueuePresentKHR(Queues[Present], &presentInfoKHR);
		ASSERT(result == VK_SUCCESS);
	}
}

void TVulkanAPI::Init(const IWindow *window)
{
	InitLib();
	InitInstance();
	InitBackBuffer(window);
	InitDevice();
	InitSwapChain();
	vkGetDeviceQueue(Device, 0, 0, &Queues[Graphics]);
	vkGetDeviceQueue(Device, 0, 0, &Queues[Present]);
	InitCommandPool();

	renderingFinishedSemaphore = CreateSemaphore();
	imageAvailableSemaphore = CreateSemaphore();
}	

void TVulkanAPI::DoneLib()
{
	FreeLibrary(Library);
	Library = nullptr;
}

void TVulkanAPI::DoneBackBuffer()
{
	vkDestroySurfaceKHR(Instance, BackBuffer, Allocator);
	BackBuffer = nullptr;
}

void TVulkanAPI::DoneInstance()
{
	vkDestroyInstance(Instance, Allocator);
	Instance = nullptr;
}

void TVulkanAPI::DoneDevice()
{
	vkDestroyDevice(Device, Allocator);
	Device = nullptr;
}

void TVulkanAPI::DoneSwapChain()
{
	// TODO: Move from DoneVulkanSwapChain
}

void TVulkanAPI::DoneCommandPool()
{
	vkDestroyCommandPool(Device, Pool, Allocator);
}

void TVulkanAPI::Done()
{
	DoneCommandPool();
	DoneSwapChain();
	DoneDevice();
	DoneBackBuffer();
	DoneInstance();
    DoneLib();
}