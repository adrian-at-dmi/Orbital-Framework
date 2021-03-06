#include "Device.h"
#include "CommandList.h"

void Device_AddFence(Device* device, VkFence fence)
{
	device->activeFences[device->activeFenceCount] = fence;
	++device->activeFenceCount;
}

ORBITAL_EXPORT Device* Orbital_Video_Vulkan_Device_Create(Instance* instance, DeviceType type)
{
	Device* handle = (Device*)calloc(1, sizeof(Device));
	handle->instance = instance;
	handle->type = type;
	return handle;
}

ORBITAL_EXPORT int Orbital_Video_Vulkan_Device_Init(Device* handle, int adapterIndex)
{
	// -1 adapter defaults to 0
	if (adapterIndex == -1) adapterIndex = 0;

	// get physical device group if vulkan API version is newer otherwise get physical device only
	if (handle->instance->nativeMaxFeatureLevel >= VK_API_VERSION_1_1)
	{
		uint32_t deviceGroupCount;
		if (vkEnumeratePhysicalDeviceGroups(handle->instance->instance, &deviceGroupCount, NULL) != VK_SUCCESS) return 0;

		VkPhysicalDeviceGroupProperties* physicalDeviceGroups = alloca(sizeof(VkPhysicalDeviceGroupProperties) * deviceGroupCount);
		if (vkEnumeratePhysicalDeviceGroups(handle->instance->instance, &deviceGroupCount, physicalDeviceGroups) != VK_SUCCESS) return 0;
		handle->physicalDeviceGroup = physicalDeviceGroups[adapterIndex];
		if (handle->physicalDeviceGroup.physicalDeviceCount <= 0) return 0;
		handle->physicalDevice = handle->physicalDeviceGroup.physicalDevices[0];
	}
	else
	{
		uint32_t deviceCount;
		if (vkEnumeratePhysicalDevices(handle->instance->instance, &deviceCount, NULL) != VK_SUCCESS) return 0;
		if (adapterIndex >= deviceCount) return 0;

		VkPhysicalDevice* physicalDevices = alloca(sizeof(VkPhysicalDevice) * deviceCount);
		if (vkEnumeratePhysicalDevices(handle->instance->instance, &deviceCount, physicalDevices) != VK_SUCCESS) return 0;
		handle->physicalDevice = physicalDevices[adapterIndex];
	}

	// get max feature level
	VkPhysicalDeviceProperties physicalDeviceProperties = {0};
	vkGetPhysicalDeviceProperties(handle->physicalDevice, &physicalDeviceProperties);
	handle->nativeFeatureLevel = physicalDeviceProperties.apiVersion;

	// validate max isn't less than min
	if (handle->nativeFeatureLevel < handle->instance->nativeMinFeatureLevel) return 0;
	for (int i = 0; i != handle->physicalDeviceGroup.physicalDeviceCount; ++i)
	{
		VkPhysicalDeviceProperties deviceProperties = {0};
		vkGetPhysicalDeviceProperties(handle->physicalDeviceGroup.physicalDevices[i], &deviceProperties);
		if (deviceProperties.apiVersion < handle->instance->nativeMinFeatureLevel) return 0;
		if (deviceProperties.apiVersion != handle->nativeFeatureLevel) return 0;// make sure all devices support the same api version
	}

	// get device features
    vkGetPhysicalDeviceFeatures(handle->physicalDevice, &handle->physicalDeviceFeatures);
	
	// get supported extensions for device
	uint32_t extensionPropertiesCount = 0;
	if (vkEnumerateDeviceExtensionProperties(handle->physicalDevice, NULL, &extensionPropertiesCount, NULL) != VK_SUCCESS) return 0;

	VkExtensionProperties* extensionProperties = alloca(sizeof(VkExtensionProperties) * extensionPropertiesCount);
	if (vkEnumerateDeviceExtensionProperties(handle->physicalDevice, NULL, &extensionPropertiesCount, extensionProperties) != VK_SUCCESS) return 0;

	// make sure device supports required extensions
	uint32_t initExtensionCount = 0;
	char* initExtensions[32];
	if (handle->type == DeviceType_Presentation)
	{
		#ifdef _WIN32
		uint32_t expectedExtensionCount = 1;
		#endif
		for (uint32_t i = 0; i != extensionPropertiesCount; ++i)
		{
			if
			(
				strcmp(extensionProperties[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) != 0
			)
			{
				continue;
			}

			initExtensions[initExtensionCount] = extensionProperties[i].extensionName;
			++initExtensionCount;
		}

		// if there are missing extensions exit
		if (expectedExtensionCount != initExtensionCount) return 0;
	}

	// make sure device supports all command queue types
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(handle->physicalDevice, &queueFamilyCount, NULL);
    if (queueFamilyCount == 0) return 0;

	VkQueueFamilyProperties* queueFamilyProperties = alloca(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(handle->physicalDevice, &queueFamilyCount, queueFamilyProperties);
	int foundQueueFamilyIndex = -1;
    for (uint32_t i = 0; i != queueFamilyCount; ++i)
	{
		VkQueueFlags flags = queueFamilyProperties->queueFlags;
		if ((flags & VK_QUEUE_GRAPHICS_BIT) != 0 && (flags & VK_QUEUE_COMPUTE_BIT) != 0 && (flags & VK_QUEUE_TRANSFER_BIT) != 0)
		{
			foundQueueFamilyIndex = i;
			break;
		}
	}
	if (foundQueueFamilyIndex == -1) return 0;
	handle->queueFamilyIndex = foundQueueFamilyIndex;

	// create device
    float queuePriorities = 0;
    VkDeviceQueueCreateInfo queueCreateInfo[1] = {0};
    queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo[0].queueFamilyIndex = foundQueueFamilyIndex;
    queueCreateInfo[0].queueCount = 1;
    queueCreateInfo[0].pQueuePriorities = &queuePriorities;
    queueCreateInfo[0].flags = 0;

    VkDeviceCreateInfo deviceInfo = {0};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = queueCreateInfo;
    deviceInfo.enabledLayerCount = 0;
    deviceInfo.ppEnabledLayerNames = NULL;
    deviceInfo.enabledExtensionCount = initExtensionCount;
    deviceInfo.ppEnabledExtensionNames = initExtensions;
    deviceInfo.pEnabledFeatures = NULL;

	if (vkCreateDevice(handle->physicalDevice, &deviceInfo, NULL, &handle->device) != VK_SUCCESS) return 0;
	vkGetDeviceQueue(handle->device, foundQueueFamilyIndex, 0, &handle->queue);
	
	// create command pool
	VkCommandPoolCreateInfo poolCreateInfo = {0};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCreateInfo.queueFamilyIndex = foundQueueFamilyIndex;
    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	if (vkCreateCommandPool(handle->device, &poolCreateInfo, NULL, &handle->commandPool) != VK_SUCCESS) return 0;

	return 1;
}

ORBITAL_EXPORT void Orbital_Video_Vulkan_Device_Dispose(Device* handle)
{
	if (handle->commandPool != NULL)
	{
		vkDestroyCommandPool(handle->device, handle->commandPool, NULL);
		handle->commandPool = NULL;
	}

	if (handle->device != NULL)
	{
		vkDestroyDevice(handle->device, NULL);
		handle->device = NULL;
	}
	
	free(handle);
}

ORBITAL_EXPORT void Orbital_Video_Vulkan_Device_BeginFrame(Device* handle)
{
	if (handle->activeFenceCount != 0)
	{
		vkResetFences(handle->device, handle->activeFenceCount, &handle->activeFences);
		handle->activeFenceCount = 0;
	}
}

ORBITAL_EXPORT void Orbital_Video_Vulkan_Device_EndFrame(Device* handle)
{
	if (handle->activeFenceCount != 0) vkWaitForFences(handle->device, handle->activeFenceCount, &handle->activeFences, VK_TRUE, UINT64_MAX);
	vkQueueWaitIdle(handle->queue);
	vkDeviceWaitIdle(handle->device);
}