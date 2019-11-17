#pragma once
#include "Instance.h"

typedef enum DeviceType
{
	DeviceType_Presentation,
	DeviceType_Background
} DeviceType;

typedef struct Device
{
	DeviceType type;
	uint32_t nativeFeatureLevel;
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceGroupProperties physicalDeviceGroup;
	VkPhysicalDeviceFeatures physicalDeviceFeatures;
	uint32_t queueFamilyIndex;

	Instance* instance;
	VkDevice device;
	VkQueue queue;
	VkCommandPool commandPool;
	VkFence fence;
	VkSemaphore semaphore;
} Device;