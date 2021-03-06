#pragma once

#if !defined(VK_GLOBAL_LEVEL_FUNCTION)
#	define VK_GLOBAL_LEVEL_FUNCTION(name)
#endif

VK_GLOBAL_LEVEL_FUNCTION(vkEnumerateInstanceExtensionProperties);
VK_GLOBAL_LEVEL_FUNCTION(vkEnumerateInstanceLayerProperties);
VK_GLOBAL_LEVEL_FUNCTION(vkCreateInstance);

#undef VK_GLOBAL_LEVEL_FUNCTION

#if !defined(VK_INSTANCE_LEVEL_FUNCTION)
#	define VK_INSTANCE_LEVEL_FUNCTION(name)
#endif

VK_INSTANCE_LEVEL_FUNCTION(vkDestroyInstance);
VK_INSTANCE_LEVEL_FUNCTION(vkCreateWin32SurfaceKHR);
VK_INSTANCE_LEVEL_FUNCTION(vkDestroySurfaceKHR);
VK_INSTANCE_LEVEL_FUNCTION(vkCreateDevice);
VK_INSTANCE_LEVEL_FUNCTION(vkGetDeviceProcAddr);
VK_INSTANCE_LEVEL_FUNCTION(vkEnumeratePhysicalDevices);
VK_INSTANCE_LEVEL_FUNCTION(vkEnumerateDeviceExtensionProperties);
VK_INSTANCE_LEVEL_FUNCTION(vkGetPhysicalDeviceProperties);
VK_INSTANCE_LEVEL_FUNCTION(vkGetPhysicalDeviceFeatures);
VK_INSTANCE_LEVEL_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties);
VK_INSTANCE_LEVEL_FUNCTION(vkGetPhysicalDeviceSurfaceSupportKHR);

#undef VK_INSTANCE_LEVEL_FUNCTION

#if !defined(VK_DEVICE_LEVEL_FUNCTION)
#	define VK_DEVICE_LEVEL_FUNCTION(name)
#endif

VK_DEVICE_LEVEL_FUNCTION(vkGetDeviceQueue);
VK_DEVICE_LEVEL_FUNCTION(vkCreateImageView);
VK_DEVICE_LEVEL_FUNCTION(vkCreateRenderPass);
VK_DEVICE_LEVEL_FUNCTION(vkCreateFramebuffer);
VK_DEVICE_LEVEL_FUNCTION(vkCreateShaderModule);
VK_DEVICE_LEVEL_FUNCTION(vkCreatePipelineLayout);
VK_DEVICE_LEVEL_FUNCTION(vkCreateGraphicsPipelines);
VK_DEVICE_LEVEL_FUNCTION(vkDestroyDevice);
VK_DEVICE_LEVEL_FUNCTION(vkCreateSemaphore);
VK_DEVICE_LEVEL_FUNCTION(vkCreateCommandPool);
VK_DEVICE_LEVEL_FUNCTION(vkAllocateCommandBuffers);
VK_DEVICE_LEVEL_FUNCTION(vkBeginCommandBuffer);
VK_DEVICE_LEVEL_FUNCTION(vkCmdPipelineBarrier);
VK_DEVICE_LEVEL_FUNCTION(vkCmdClearColorImage);
VK_DEVICE_LEVEL_FUNCTION(vkCmdBeginRenderPass);
VK_DEVICE_LEVEL_FUNCTION(vkCmdBindPipeline);
VK_DEVICE_LEVEL_FUNCTION(vkCmdDraw);
VK_DEVICE_LEVEL_FUNCTION(vkCmdEndRenderPass);
VK_DEVICE_LEVEL_FUNCTION(vkEndCommandBuffer);
VK_DEVICE_LEVEL_FUNCTION(vkQueueSubmit);
VK_DEVICE_LEVEL_FUNCTION(vkFreeCommandBuffers);
VK_DEVICE_LEVEL_FUNCTION(vkDestroyCommandPool);
VK_DEVICE_LEVEL_FUNCTION(vkDestroySemaphore);
VK_DEVICE_LEVEL_FUNCTION(vkDestroyShaderModule);
VK_DEVICE_LEVEL_FUNCTION(vkDestroyPipelineLayout);
VK_DEVICE_LEVEL_FUNCTION(vkDestroyPipeline);
VK_DEVICE_LEVEL_FUNCTION(vkDestroyRenderPass);
VK_DEVICE_LEVEL_FUNCTION(vkDestroyFramebuffer);
VK_DEVICE_LEVEL_FUNCTION(vkDestroyImageView);

#undef VK_DEVICE_LEVEL_FUNCTION