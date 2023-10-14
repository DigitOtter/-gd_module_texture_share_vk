#include "texture_sender.h"

#include "godot_texture_share.h"

#include <servers/rendering_server.h>

TextureSender::TextureSender() {
	this->_client.reset(new texture_share_client_t());

	// Load Extensions
#ifdef USE_OPENGL
	if (!ExternalHandleGl::LoadGlEXT())
		ERR_PRINT("Failed to load OpenGL Extensions");

#else

	RenderingDevice *const prd = RenderingServer::get_singleton()->get_rendering_device();
	assert(prd);
	VkInstance vk_inst =
			(VkInstance)prd->get_driver_resource(RenderingDevice::DRIVER_RESOURCE_VULKAN_INSTANCE, RID(), 0);
	VkPhysicalDevice vk_ph_dev =
			(VkPhysicalDevice)prd->get_driver_resource(RenderingDevice::DRIVER_RESOURCE_VULKAN_PHYSICAL_DEVICE, RID(), 0);
	VkDevice vk_dev = (VkDevice)prd->get_driver_resource(RenderingDevice::DRIVER_RESOURCE_VULKAN_DEVICE, RID(), 0);
	VkQueue vk_queue = (VkQueue)prd->get_driver_resource(RenderingDevice::DRIVER_RESOURCE_VULKAN_QUEUE, RID(), 0);
	uint32_t vk_queue_index =
			(uint32_t)prd->get_driver_resource(RenderingDevice::DRIVER_RESOURCE_VULKAN_QUEUE_FAMILY_INDEX, RID(), 0);

	this->_client->InitializeVulkan(vk_inst, vk_dev, vk_ph_dev, vk_queue, vk_queue_index, true);

//	this->_client->InitializeVulkan();

//	if(!ExternalHandleVk::LoadVulkanHandleExtensions(vk_inst))
//		ERR_PRINT("Failed to load Vulkan Extensions");
//	if(!ExternalHandleVk::LoadCompatibleSemaphorePropsInfo(vk_ph_dev))
//		ERR_PRINT("Failed to load Vulkan semaphore info");
#endif
}

void TextureSender::set_texture(const Ref<Texture2D> &texture, Image::Format texture_format) {
	this->_texture = texture;
	if (!this->_shared_texture_name.empty()) {
		this->check_and_update_shared_texture(texture_format);
	}
}

Ref<Texture2D> TextureSender::get_texture() {
	return this->_texture;
}

void TextureSender::set_shared_texture_name(const String &shared_texture_name) {
	const std::string new_name = shared_texture_name.ascii().ptr();
	if (new_name != this->_shared_texture_name) {
		this->_shared_texture_name = new_name;

		if (this->_texture.is_valid()) {
			this->update_shared_texture(this->_width, this->_height, this->_format);
		}
	}
}

String TextureSender::get_shared_texture_name() {
	return String(this->_shared_texture_name.c_str());
}

bool TextureSender::send_texture() {
	if (this->_shared_texture_name.empty()) {
		return false;
	}

	if (!this->check_and_update_shared_texture(this->_format)) {
		return false;
	}

	assert(this->_client);

	// Send texture
	const texture_id_t texture_id =
			(texture_id_t)RenderingServer::get_singleton()->texture_get_native_handle(this->_texture->get_rid());

#ifdef USE_OPENGL
	const texture_share_client_t::ImageExtent dim{
		{ 0, 0 },
		{ (GLsizei)this->_width, (GLsizei)this->_height },
	};

	GLint drawFboId = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);
	this->_client->SendImageBlit(this->_shared_texture_name, texture_id, GL_TEXTURE_2D, dim, false, drawFboId);
#else
	this->_client->SendImageBlit(this->_shared_texture_name, texture_id, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
#endif

	return true;
}

void TextureSender::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_texture"), &TextureSender::get_texture);
	ClassDB::bind_method(D_METHOD("set_texture"), &TextureSender::set_texture);
	//	ClassDB::add_property("TextureSender", PropertyInfo(Variant::STRING, "texture"), "set_texture",
	//	                      "get_texture");

	ClassDB::bind_method(D_METHOD("get_shared_texture_name"), &TextureSender::get_shared_texture_name);
	ClassDB::bind_method(D_METHOD("set_shared_texture_name"), &TextureSender::set_shared_texture_name);
	ClassDB::add_property("TextureSender", PropertyInfo(Variant::STRING, "shared_texture_name"),
			"set_shared_texture_name", "get_shared_texture_name");

	ClassDB::bind_method(D_METHOD("send_texture"), &TextureSender::send_texture);
}

bool TextureSender::update_shared_texture(uint32_t width, uint32_t height, Image::Format format) {
	assert(!this->_shared_texture_name.empty());
	if (this->_width == width && this->_height == height && this->_format == format) {
		return true;
	}

	const texture_format_t gl_format = GodotTextureShare::convert_godot_to_rendering_device_format(format);
	if (gl_format == GL_NONE) {
		return false;
	}

	this->_width = width;
	this->_height = height;
	this->_format = format;

	assert(this->_client);
	this->_client->InitImage(this->_shared_texture_name, width, height, gl_format, true);

	return true;
}

bool TextureSender::check_and_update_shared_texture(Image::Format format) {
	if (this->_texture.is_valid()) {
		const uint32_t new_width = this->_texture->get_width();
		const uint32_t new_height = this->_texture->get_height();
		return this->update_shared_texture(new_width, new_height, format);
	}

	return false;
}
