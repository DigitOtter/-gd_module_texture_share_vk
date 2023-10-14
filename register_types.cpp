#include "register_types.h"

#include <drivers/vulkan/vulkan_context.h>

// #include "godot_texture_share.h"
// #include "texture_sender.h"

void initialize_gd_module_texture_share_vk_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_CORE) {
		WARN_PRINT("Loading additional Vulkan extensions");
		VulkanContext::register_requested_instance_extension(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME, true);
		VulkanContext::register_requested_instance_extension(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, true);

		VulkanContext::register_requested_device_extension(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME, true);
		VulkanContext::register_requested_device_extension(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME, true);
		VulkanContext::register_requested_device_extension(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME, true);
		VulkanContext::register_requested_device_extension(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME, true);
		VulkanContext::register_requested_device_extension(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME, true);
	} else if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		// ClassDB::register_class<GodotTextureShare>();
		// ClassDB::register_class<TextureSender>();
	}
}

void uninitialize_gd_module_texture_share_vk_module(ModuleInitializationLevel p_level) {
}
