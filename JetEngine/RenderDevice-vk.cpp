#include "RenderDevice.h"
#include "RenderDevice-vk.h"

#include "Utils.h"

// Resources:
// https://software.intel.com/en-us/articles/api-without-secrets-introduction-to-vulkan-preface

enum
{
	Graphics,
	Present
};

static struct 
{
	LibraryType Library = VK_NULL_HANDLE;
	VkInstance Instance = VK_NULL_HANDLE;
	VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
	VkDevice Device = VK_NULL_HANDLE;
	VkSurfaceKHR BackBuffer = VK_NULL_HANDLE;
	VkAllocationCallbacks *Allocator = nullptr;

	VkQueue Queues[2] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
	struct
	{
		VkSwapchainKHR Handle = VK_NULL_HANDLE;
		VkImage Images[2] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
		VkImageView Views[2] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
	} SwapChain;
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
	for (int32 deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex)
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

	for (int32 i = 0; i < ArrayLength(Vulkan.SwapChain.Images); ++i) 
	{
		if (Vulkan.SwapChain.Images[i] != VK_NULL_HANDLE) 
		{
			vkDestroyImageView(Vulkan.Device, Vulkan.SwapChain.Views[i], Vulkan.Allocator);
			Vulkan.SwapChain.Images[i] = VK_NULL_HANDLE;
			Vulkan.SwapChain.Views[i] = VK_NULL_HANDLE;
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

		for (int32 i = 0; i < surfaceFormatsCount; ++i) {
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
	};

	const auto &GetSwapChainPresentMode = []() -> VkPresentModeKHR {
		uint32 presentModesCount;
		VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(Vulkan.PhysicalDevice, Vulkan.BackBuffer, &presentModesCount, nullptr);
		ASSERT(result == VK_SUCCESS && presentModesCount != 0);

		VkPresentModeKHR *presentModes = ALLOCA(VkPresentModeKHR, presentModesCount);
		result = vkGetPhysicalDeviceSurfacePresentModesKHR(Vulkan.PhysicalDevice, Vulkan.BackBuffer, &presentModesCount, presentModes);
		ASSERT(result == VK_SUCCESS);

		// MAILBOX is the lowest latency V-Sync enabled mode (something like triple-buffering) so use it if available
		for (int32 i = 0; i < presentModesCount; ++i) {
			if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
				return presentModes[i];
			}
		}
		// IMMEDIATE mode allows us to display frames in a V-Sync independent manner so it can introduce screen tearing
		// But this mode is the best for benchmarking purposes if we want to check the real number of FPS
		for (int32 i = 0; i < presentModesCount; ++i) {
			if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
				return presentModes[i];
			}
		}
		// FIFO present mode is always available
		for (int32 i = 0; i < presentModesCount; ++i) {
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
	auto prevSwapchain = Vulkan.SwapChain.Handle;

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

	VkResult result = vkCreateSwapchainKHR(Vulkan.Device, &swapchainCreateInfo, Vulkan.Allocator, &Vulkan.SwapChain.Handle);
	ASSERT(result == VK_SUCCESS);

	if (prevSwapchain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(Vulkan.Device, prevSwapchain, Vulkan.Allocator);

	uint32 imageCount = 0;
	result = vkGetSwapchainImagesKHR(Vulkan.Device, Vulkan.SwapChain.Handle, &imageCount, nullptr);
	ASSERT(result == VK_SUCCESS && imageCount == ArrayLength(Vulkan.SwapChain.Images));

	result = vkGetSwapchainImagesKHR(Vulkan.Device, Vulkan.SwapChain.Handle, &imageCount, Vulkan.SwapChain.Images);
	ASSERT(result == VK_SUCCESS);

	for (int32 i = 0; i < ArrayLength(Vulkan.SwapChain.Images); ++i) {
		VkImageViewCreateInfo imageViewCreateInfo;
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.pNext = nullptr;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.image = Vulkan.SwapChain.Images[i];
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

		VkResult result = vkCreateImageView(Vulkan.Device, &imageViewCreateInfo, Vulkan.Allocator, &Vulkan.SwapChain.Views[i]); 
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

void Done()
{
	DoneVulkanSwapChain();
	DoneVulkanDevice();
	DoneVukanBackBuffer();
	DoneVulkanInstance();
    DoneVulkanLib();
}