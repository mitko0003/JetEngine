#include "Precompiled.h"

#include "RenderDevice.h"
#include "RenderDevice-vk.h"

// Resources:
// https://software.intel.com/en-us/articles/api-without-secrets-introduction-to-vulkan-preface

enum QueueType
{
	Graphics,
	Present
};

struct
{
	VkSwapchainKHR Handle = VK_NULL_HANDLE;
	VkImage Images[2] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
	VkImageView Views[2] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
} SwapChain;

static struct 
{
	LibraryType Library = VK_NULL_HANDLE;
	VkInstance Instance = VK_NULL_HANDLE;
	VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
	VkDevice Device = VK_NULL_HANDLE;
	VkAllocationCallbacks *Allocator = nullptr;
	VkSurfaceKHR BackBuffer = VK_NULL_HANDLE;

	VkQueue Queues[2] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
	VkCommandPool Pool = VK_NULL_HANDLE;
} Vulkan;

void InitVulkanLib()
{
    Vulkan.Library = LoadLibrary("vulkan-1.dll");
    ASSERT(Vulkan.Library != nullptr);

    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(Vulkan.Library, "vkGetInstanceProcAddr"));

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

void InitVulkanInstance()
{
	uint32 uPropertyCount;
	VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &uPropertyCount, nullptr);
	ASSERT(result == VK_SUCCESS);
	VkExtensionProperties* extensions = ALLOCA(VkExtensionProperties, uPropertyCount);
	result = vkEnumerateInstanceExtensionProperties(nullptr, &uPropertyCount, extensions);
	ASSERT(result == VK_SUCCESS);

	DebugPrint("****** EXTENSION PROPERTIES ******\n");
	for (uint32 i = 0; i < uPropertyCount; ++i)
	{
		DebugPrint("VkExtensionProperty(%d): %s(%d)\n", i, extensions[i].extensionName, extensions[i].specVersion);
	}

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

	VkInstanceCreateInfo instanceCreateInfo;
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = nullptr;
	instanceCreateInfo.flags = 0;
	instanceCreateInfo.pApplicationInfo = &applicationInfo;
	instanceCreateInfo.enabledLayerCount = 0;
	instanceCreateInfo.ppEnabledLayerNames = nullptr;
	instanceCreateInfo.enabledExtensionCount = ArrayLength(needed);
	instanceCreateInfo.ppEnabledExtensionNames = needed;

	result = vkCreateInstance(&instanceCreateInfo, Vulkan.Allocator, &Vulkan.Instance);
	ASSERT(result == VK_SUCCESS);

#define VK_INSTANCE_LEVEL_FUNCTION(name) PFN_##name name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(Vulkan.Instance, #name)); ASSERT(name != nullptr)
#	include "VulkanFunctions.h"
#undef VK_INSTANCE_LEVEL_FUNCTION
}

void InitVulkanBackBuffer(HINSTANCE instance, HWND hwnd)
{
	VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo;
	win32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	win32SurfaceCreateInfo.pNext = nullptr;
	win32SurfaceCreateInfo.flags = 0;
	win32SurfaceCreateInfo.hinstance = instance;
	win32SurfaceCreateInfo.hwnd = hwnd;

	VkResult result = vkCreateWin32SurfaceKHR(Vulkan.Instance, &win32SurfaceCreateInfo, Vulkan.Allocator, &Vulkan.BackBuffer);
	ASSERT(result == VK_SUCCESS);
}

void InitVulkanDevice()
{
	uint32 deviceCount;
	VkResult result = vkEnumeratePhysicalDevices(Vulkan.Instance, &deviceCount, nullptr);
	ASSERT(result == VK_SUCCESS);

	VkPhysicalDevice *physicalDevices = ALLOCA(VkPhysicalDevice, deviceCount);
	result = vkEnumeratePhysicalDevices(Vulkan.Instance, &deviceCount, physicalDevices);
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
			result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, familyIndex, Vulkan.BackBuffer, &presentQueueSupport);
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

	VkDeviceQueueCreateInfo deviceQueueCreateInfo;
	deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCreateInfo.pNext = nullptr;
	deviceQueueCreateInfo.flags = 0;
	deviceQueueCreateInfo.queueFamilyIndex = graphicsQueueIndex;
	deviceQueueCreateInfo.queueCount = ArrayLength(queuePriorities);
	deviceQueueCreateInfo.pQueuePriorities = queuePriorities;

	const char *extensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

	VkDeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = nullptr;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = nullptr;
	deviceCreateInfo.enabledExtensionCount = ArrayLength(extensions);
	deviceCreateInfo.ppEnabledExtensionNames = extensions;
	deviceCreateInfo.pEnabledFeatures = nullptr;

	Vulkan.PhysicalDevice = physicalDevices[physicalDeviceIndex];

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Vulkan.PhysicalDevice, Vulkan.BackBuffer, &surfaceCapabilities);
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

	result = vkCreateDevice(Vulkan.PhysicalDevice, &deviceCreateInfo, Vulkan.Allocator, &Vulkan.Device);
	ASSERT(result == VK_SUCCESS);

#define VK_DEVICE_LEVEL_FUNCTION(name) PFN_##name name = reinterpret_cast<PFN_##name>(vkGetDeviceProcAddr(Vulkan.Device, #name)); ASSERT(name != nullptr)
#	include "VulkanFunctions.h"
#undef VK_DEVICE_LEVEL_FUNCTION
}

void InitVulkanSwapChain() 
{
	vkDeviceWaitIdle(Vulkan.Device);

	for (int32 i = 0; i < ArrayLength(SwapChain.Images); ++i) 
	{
		if (SwapChain.Images[i] != VK_NULL_HANDLE) 
		{
			vkDestroyImageView(Vulkan.Device, SwapChain.Views[i], Vulkan.Allocator);
			SwapChain.Images[i] = VK_NULL_HANDLE;
			SwapChain.Views[i] = VK_NULL_HANDLE;
		}
	}

	const auto &GetSwapChainFormat = []() -> VkSurfaceFormatKHR {
		uint32 surfaceFormatsCount;
		VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(Vulkan.PhysicalDevice, Vulkan.BackBuffer, &surfaceFormatsCount, nullptr);
		ASSERT(result == VK_SUCCESS && surfaceFormatsCount != 0);

		VkSurfaceFormatKHR *surfaceFormats = ALLOCA(VkSurfaceFormatKHR, surfaceFormatsCount);
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(Vulkan.PhysicalDevice, Vulkan.BackBuffer, &surfaceFormatsCount, surfaceFormats);
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

	const auto &GetSwapChainExtent = []() -> VkExtent2D {
		VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
		VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Vulkan.PhysicalDevice, Vulkan.BackBuffer, &surfaceCapabilities);
		ASSERT(result == VK_SUCCESS);

		if (surfaceCapabilities.currentExtent.width == -1) {
			VkExtent2D swapChainExtent = {
				Clamp(1920u, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
				Clamp(1080u, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height)
			};
		}

		return surfaceCapabilities.currentExtent;
	};

	const auto &GetSwapChainUsageFlags = []() -> VkImageUsageFlags {
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Vulkan.PhysicalDevice, Vulkan.BackBuffer, &surfaceCapabilities);
		ASSERT(result == VK_SUCCESS);

		if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		ASSERT(!"");
		return static_cast<VkImageUsageFlags>(-1);
	};

	const auto &GetSwapChainTransform = []() -> VkSurfaceTransformFlagBitsKHR {
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Vulkan.PhysicalDevice, Vulkan.BackBuffer, &surfaceCapabilities);
		ASSERT(result == VK_SUCCESS);
		return surfaceCapabilities.currentTransform;
	};

	const auto &GetSwapChainPresentMode = []() -> VkPresentModeKHR {
		uint32 presentModesCount;
		VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(Vulkan.PhysicalDevice, Vulkan.BackBuffer, &presentModesCount, nullptr);
		ASSERT(result == VK_SUCCESS && presentModesCount != 0);

		VkPresentModeKHR *presentModes = ALLOCA(VkPresentModeKHR, presentModesCount);
		result = vkGetPhysicalDeviceSurfacePresentModesKHR(Vulkan.PhysicalDevice, Vulkan.BackBuffer, &presentModesCount, presentModes);
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
	swapchainCreateInfo.surface = Vulkan.BackBuffer;
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

	VkResult result = vkCreateSwapchainKHR(Vulkan.Device, &swapchainCreateInfo, Vulkan.Allocator, &SwapChain.Handle);
	ASSERT(result == VK_SUCCESS);

	if (prevSwapchain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(Vulkan.Device, prevSwapchain, Vulkan.Allocator);

	uint32 imageCount = 0;
	result = vkGetSwapchainImagesKHR(Vulkan.Device, SwapChain.Handle, &imageCount, nullptr);
	ASSERT(result == VK_SUCCESS && imageCount == ArrayLength(SwapChain.Images));

	result = vkGetSwapchainImagesKHR(Vulkan.Device, SwapChain.Handle, &imageCount, SwapChain.Images);
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

		VkResult result = vkCreateImageView(Vulkan.Device, &imageViewCreateInfo, Vulkan.Allocator, &SwapChain.Views[i]); 
		ASSERT(result == VK_SUCCESS);
	}
}

void InitVulkanCommandPool()
{
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.pNext = nullptr;
	commandPoolCreateInfo.flags = 0;
	commandPoolCreateInfo.queueFamilyIndex = 0;

	VkResult result = vkCreateCommandPool(Vulkan.Device, &commandPoolCreateInfo, nullptr, &Vulkan.Pool);
	ASSERT(result == VK_SUCCESS);
}

VkCommandBuffer CreateCommandBuffer()
{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo;
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = nullptr;
	commandBufferAllocateInfo.commandPool = Vulkan.Pool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	VkResult result = vkAllocateCommandBuffers(Vulkan.Device, &commandBufferAllocateInfo, &commandBuffer);
	ASSERT(result == VK_SUCCESS);

	return commandBuffer;
}

void DestroyCommandBuffer(VkCommandBuffer commandBuffer)
{
	vkFreeCommandBuffers(Vulkan.Device, Vulkan.Pool, 1, &commandBuffer);
}

VkRenderPass CreateRenderPass()
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

	VkRenderPass renderPass;
	VkResult result = vkCreateRenderPass(Vulkan.Device, &renderPassCreateInfo, Vulkan.Allocator, &renderPass);
	ASSERT(result == VK_SUCCESS);
	return renderPass;
}

void DestroyRenderPass(VkRenderPass renderPass)
{
	vkDestroyRenderPass(Vulkan.Device, renderPass, Vulkan.Allocator);
}

VkFramebuffer CreateFramebuffer(VkImageView imageView, VkRenderPass renderPass)
{
	VkFramebufferCreateInfo framebufferCreateInfo = {};
	framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCreateInfo.pNext = nullptr;
	framebufferCreateInfo.flags = 0;
	framebufferCreateInfo.renderPass = renderPass;
	framebufferCreateInfo.attachmentCount = 1;
	framebufferCreateInfo.pAttachments = &imageView;
	framebufferCreateInfo.width = 300;
	framebufferCreateInfo.height = 300;
	framebufferCreateInfo.layers = 1;

	VkFramebuffer framebuffer;
	VkResult result = vkCreateFramebuffer(Vulkan.Device, &framebufferCreateInfo, Vulkan.Allocator, &framebuffer);
	ASSERT(result == VK_SUCCESS);
	return framebuffer;
}

void DestroyFramebuffer(VkFramebuffer framebuffer)
{
	vkDestroyFramebuffer(Vulkan.Device, framebuffer, Vulkan.Allocator);
}

VkPipelineLayout CreatePipelineLayout()
{
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = nullptr;
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.setLayoutCount = 0;
	pipelineLayoutCreateInfo.pSetLayouts = nullptr;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	VkPipelineLayout pipelineLayout;
	VkResult result = vkCreatePipelineLayout(Vulkan.Device, &pipelineLayoutCreateInfo, Vulkan.Allocator, &pipelineLayout);
	ASSERT(result == VK_SUCCESS);
	return pipelineLayout;
}

void DestroyPipelineLayout(VkPipelineLayout pipelineLayout)
{
	vkDestroyPipelineLayout(Vulkan.Device, pipelineLayout, Vulkan.Allocator);
}

VkShaderModule CreateShaderModule(const uint32 *shader, uint32 size)
{
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.pNext = nullptr;
	shaderModuleCreateInfo.flags = 0;
	shaderModuleCreateInfo.codeSize = size;
	shaderModuleCreateInfo.pCode = shader;

	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(Vulkan.Device, &shaderModuleCreateInfo, Vulkan.Allocator, &shaderModule);
	ASSERT(result == VK_SUCCESS);
	return shaderModule;
}

void DestroyShaderModule(VkShaderModule shaderModule)
{
	vkDestroyShaderModule(Vulkan.Device, shaderModule, Vulkan.Allocator);
}

static const char *vertexShader =
"// Copyright 2016 Intel Corporation All Rights Reserved"
"// "
"// Intel makes no representations about the suitability of this software for any purpose."
"// THIS SOFTWARE IS PROVIDED ""AS IS."" INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES,"
"// EXPRESS OR IMPLIED, AND ALL LIABILITY, INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES,"
"// FOR THE USE OF THIS SOFTWARE, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY"
"// RIGHTS, AND INCLUDING THE WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE."
"// Intel does not assume any responsibility for any errors which may appear in this software"
"// nor any responsibility to update it."
""
"#version 450"
""
"out gl_PerVertex"
"{"
"  vec4 gl_Position;"
"};"
""
"void main() {"
"    vec2 pos[3] = vec2[3]( vec2(-0.7, 0.7), vec2(0.7, 0.7), vec2(0.0, -0.7) );"
"    gl_Position = vec4( pos[gl_VertexIndex], 0.0, 1.0 );"
"}";

static const char *fragmentShader =
"// Copyright 2016 Intel Corporation All Rights Reserved"
"// "
"// Intel makes no representations about the suitability of this software for any purpose."
"// THIS SOFTWARE IS PROVIDED ""AS IS."" INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES,"
"// EXPRESS OR IMPLIED, AND ALL LIABILITY, INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES,"
"// FOR THE USE OF THIS SOFTWARE, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY"
"// RIGHTS, AND INCLUDING THE WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE."
"// Intel does not assume any responsibility for any errors which may appear in this software"
"// nor any responsibility to update it."
""
"#version 450"
""
"layout(location = 0) out vec4 out_Color;"
""
"void main() {"
"  out_Color = vec4( 0.0, 0.4, 1.0, 1.0 );"
"}";

VkPipeline CreatePipeline(VkRenderPass renderPass)
{
	VkShaderModule vertexShaderModule = CreateShaderModule(reinterpret_cast<const uint32*>(vertexShader), sizeof(vertexShader));
	VkShaderModule fragmentShaderModule = CreateShaderModule(reinterpret_cast<const uint32*>(fragmentShader), sizeof(fragmentShader));

	VkPipelineShaderStageCreateInfo shaderStageCreateInfo[2];
	shaderStageCreateInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo[0].pNext = nullptr;
	shaderStageCreateInfo[0].flags = 0;
	shaderStageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStageCreateInfo[0].module = vertexShaderModule;
	shaderStageCreateInfo[0].pName = "main";
	shaderStageCreateInfo[0].pSpecializationInfo = nullptr;

	shaderStageCreateInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo[1].pNext = nullptr;
	shaderStageCreateInfo[1].flags = 0;
	shaderStageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStageCreateInfo[1].module = fragmentShaderModule;
	shaderStageCreateInfo[1].pName = "main";
	shaderStageCreateInfo[1].pSpecializationInfo = nullptr;

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.pNext = nullptr;
	vertexInputStateCreateInfo.flags = 0;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo.pNext = nullptr;
	inputAssemblyStateCreateInfo.flags = 0;
	inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = 1920u;
	viewport.height = 1080u;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 0.0f;

	VkRect2D scissor = {};
	scissor.offset.x = 0u;
	scissor.offset.x = 0u;
	scissor.extent.width = 1920u;
	scissor.extent.height = 1080u;

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

	VkPipelineLayout pipelineLayout = CreatePipelineLayout();

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.pNext = nullptr;
	graphicsPipelineCreateInfo.flags = 0;
	graphicsPipelineCreateInfo.stageCount = 2;
	graphicsPipelineCreateInfo.pStages = shaderStageCreateInfo;
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	graphicsPipelineCreateInfo.pTessellationState = nullptr;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = nullptr;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = nullptr;
	graphicsPipelineCreateInfo.layout = pipelineLayout;
	graphicsPipelineCreateInfo.renderPass = renderPass;
	graphicsPipelineCreateInfo.subpass = 0;
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;

	VkPipeline graphicsPipeline;
	VkResult result = vkCreateGraphicsPipelines(Vulkan.Device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, Vulkan.Allocator, &graphicsPipeline);
	ASSERT(result == VK_SUCCESS);

	DestroyShaderModule(vertexShaderModule);
	DestroyShaderModule(fragmentShaderModule);
	DestroyPipelineLayout(pipelineLayout);
	return graphicsPipeline;
}

void DestroyPipeline(VkPipeline pipeline)
{
	vkDestroyPipeline(Vulkan.Device, pipeline, Vulkan.Allocator);
}

VkSemaphore CreateSemaphore()
{
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;
	semaphoreCreateInfo.flags = 0;

	VkSemaphore semaphore;
	VkResult result = vkCreateSemaphore(Vulkan.Device, &semaphoreCreateInfo, Vulkan.Allocator, &semaphore);
	ASSERT(result == VK_SUCCESS);
	return semaphore;
}

static VkSemaphore renderingFinishedSemaphore = VK_NULL_HANDLE;
static VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;

void HelloWorld()
{
	uint32_t i;
	VkResult result = vkAcquireNextImageKHR(Vulkan.Device, SwapChain.Handle, std::numeric_limits<uint64>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &i);

	VkRenderPass renderPass = CreateRenderPass();
	VkFramebuffer frambuffer = CreateFramebuffer(SwapChain.Views[i], renderPass);
	VkPipeline pipeline = CreatePipeline(renderPass);
	VkCommandBuffer commandBuffer = CreateCommandBuffer();

	DestroyCommandBuffer(commandBuffer);
	DestroyPipeline(pipeline);
	DestroyFramebuffer(frambuffer);
	DestroyRenderPass(renderPass);
}

void ClearColor()
{
	uint32_t i;
	VkResult result = vkAcquireNextImageKHR(Vulkan.Device, SwapChain.Handle, std::numeric_limits<uint64>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &i);

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

		result = vkQueueSubmit(Vulkan.Queues[Present], 1, &submitInfo, VK_NULL_HANDLE);
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

		result = vkQueuePresentKHR(Vulkan.Queues[Present], &presentInfoKHR);
		ASSERT(result == VK_SUCCESS);
	}
}

void Init(HINSTANCE instance, HWND hwnd)
{
    InitVulkanLib();
	InitVulkanInstance();
	InitVulkanBackBuffer(instance, hwnd);
	InitVulkanDevice();
	InitVulkanSwapChain();
	vkGetDeviceQueue(Vulkan.Device, 0, 0, &Vulkan.Queues[Graphics]);
	vkGetDeviceQueue(Vulkan.Device, 0, 0, &Vulkan.Queues[Present]);
	InitVulkanCommandPool();

	renderingFinishedSemaphore = CreateSemaphore();
	imageAvailableSemaphore = CreateSemaphore();
	while(true) HelloWorld();
}	

void DoneVulkanLib()
{
	FreeLibrary(Vulkan.Library);
	Vulkan.Library = nullptr;
}

void DoneVukanBackBuffer()
{
	vkDestroySurfaceKHR(Vulkan.Instance, Vulkan.BackBuffer, Vulkan.Allocator);
	Vulkan.BackBuffer = nullptr;
}

void DoneVulkanInstance()
{
	vkDestroyInstance(Vulkan.Instance, Vulkan.Allocator);
	Vulkan.Instance = nullptr;
}

void DoneVulkanDevice()
{
	vkDestroyDevice(Vulkan.Device, Vulkan.Allocator);
	Vulkan.Device = nullptr;
}

void DoneVulkanSwapChain()
{
	// TODO: Move from DoneVulkanSwapChain
}

void DoneVulkanCommandPool()
{
	vkDestroyCommandPool(Vulkan.Device, Vulkan.Pool, Vulkan.Allocator);
}

void Done()
{
	DoneVulkanCommandPool();
	DoneVulkanSwapChain();
	DoneVulkanDevice();
	DoneVukanBackBuffer();
	DoneVulkanInstance();
    DoneVulkanLib();
}