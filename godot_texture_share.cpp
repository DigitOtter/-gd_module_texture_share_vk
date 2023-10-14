#include "godot_texture_share.h"

#include <core/variant/variant_parser.h>
#include <drivers/vulkan/rendering_device_vulkan.h>
#include <servers/rendering_server.h>
//#include <godot_cpp/core/class_db.hpp>
//#include <godot_cpp/core/error_macros.hpp>

GodotTextureShare::GodotTextureShare() {
	this->_receiver.reset(new texture_share_client_t());

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

	this->_receiver->InitializeVulkan(vk_inst, vk_dev, vk_ph_dev, vk_queue, vk_queue_index, true);

#endif

	this->_create_initial_texture(1, 1, Image::FORMAT_RGBA8);
}

GodotTextureShare::~GodotTextureShare() {
	RenderingServer *const prs = RenderingServer::get_singleton();
	if (this->_texture.is_valid()) {
		prs->free(this->_texture);
		this->_texture = RID();
	}
}

Image::Format GodotTextureShare::convert_rendering_device_to_godot_format(texture_format_t format) {
#ifdef USE_OPENGL
	switch (format) {
		case GL_BGRA:
		case GL_RGBA:
			return Image::Format::FORMAT_RGBA8;

		default:
			return Image::Format::FORMAT_MAX;
	}
#else
	switch (format) {
		case VkFormat::VK_FORMAT_R8G8B8A8_UNORM:
		case VkFormat::VK_FORMAT_B8G8R8A8_UNORM:
			return Image::Format::FORMAT_RGBA8;

		case VkFormat::VK_FORMAT_B8G8R8_UNORM:
		case VkFormat::VK_FORMAT_R8G8B8_UNORM:
			return Image::Format::FORMAT_RGB8;

		default:
			return Image::Format::FORMAT_MAX;
	}
#endif
}

texture_format_t GodotTextureShare::convert_godot_to_rendering_device_format(Image::Format format) {
#ifdef USE_OPENGL
	switch (format) {
		case Image::Format::FORMAT_RGBA8:
			return GL_RGBA;
		case Image::Format::FORMAT_RGB8:
			return GL_RGB;
		default:
			return GL_NONE;
	}
#else
	switch (format) {
		case Image::Format::FORMAT_RGBA8:
			return VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
		case Image::Format::FORMAT_RGB8:
			return VkFormat::VK_FORMAT_R8G8B8_UNORM;
		default:
			return VkFormat::VK_FORMAT_UNDEFINED;
	}
#endif
}

void GodotTextureShare::_init() {
	assert(this->_texture == RID());
	this->_width = 0;
	this->_height = 0;
	this->_flags = 0;

	this->_receiver.reset(nullptr);
	this->_channel_name = "";

	this->_create_initial_texture(1, 1, Image::FORMAT_RGB8);
}

void GodotTextureShare::draw(RID p_canvas_item, const Point2 &p_pos, const Color &p_modulate, bool p_transpose) const {
	// Code taken from godot source ImageTexture class
	if ((this->_width | this->_height) == 0)
		return;

	RenderingServer::get_singleton()->canvas_item_add_texture_rect(
			p_canvas_item, Rect2(p_pos, Size2(this->_width, this->_height)), this->_texture, false, p_modulate,
			p_transpose);
}

void GodotTextureShare::draw_rect(RID p_canvas_item, const Rect2 &p_rect, bool p_tile, const Color &p_modulate, bool p_transpose) const {
	// Code taken from godot source ImageTexture class
	if ((this->_width | this->_height) == 0)
		return;

	RenderingServer::get_singleton()->canvas_item_add_texture_rect(p_canvas_item, p_rect, this->_texture, p_tile,
			p_modulate, p_transpose);
}

void GodotTextureShare::draw_rect_region(RID p_canvas_item, const Rect2 &p_rect, const Rect2 &p_src_rect, const Color &p_modulate, bool p_transpose, bool p_clip_uv) const {
	// Code taken from godot source ImageTexture class
	if ((this->_width | this->_height) == 0)
		return;

	RenderingServer::get_singleton()->canvas_item_add_texture_rect_region(
			p_canvas_item, p_rect, this->_texture, p_src_rect, p_modulate, p_transpose, p_clip_uv);
}

String GodotTextureShare::get_shared_name() const {
	return String(this->_channel_name.c_str());
}

void GodotTextureShare::set_shared_name(String shared_name) {
	this->_channel_name = shared_name.ascii().ptr();
	if (this->_create_receiver(this->_channel_name))
		this->_receive_texture();
}

void GodotTextureShare::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_shared_name"), &GodotTextureShare::get_shared_name);
	ClassDB::bind_method(D_METHOD("set_shared_name"), &GodotTextureShare::set_shared_name);
	ClassDB::add_property("GodotTextureShare", PropertyInfo(Variant::STRING, "shared_name"), "set_shared_name",
			"get_shared_name");

	ClassDB::bind_method(D_METHOD("_receive_texture"), &GodotTextureShare::_receive_texture);
}

bool GodotTextureShare::_create_receiver(const std::string &name) {
	if (!this->_receiver)
		this->_receiver.reset(new texture_share_client_t());

	const auto *shared_image_data = this->_receiver->SharedImageHandle(name);
	if (!shared_image_data)
		return false;

	const Image::Format format = convert_rendering_device_to_godot_format(shared_image_data->ImageFormat());
	this->_update_texture(shared_image_data->Width(), shared_image_data->Height(), format);

	return true;
}

void GodotTextureShare::_receive_texture() {
	if (!this->_receiver)
		return;

		// Receive texture
#ifdef USE_OPENGL
	const texture_share_client_t::ImageExtent dim{
		{ 0, 0 },
		{ (GLsizei)this->_width, (GLsizei)this->_height },
	};

	GLint drawFboId = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);
	this->_receiver->RecvImageBlit(this->_channel_name, this->_texture_id, GL_TEXTURE_2D, dim, false, drawFboId);
#else
	RenderingServer *const prs = RenderingServer::get_singleton();
	RenderingDevice *const prd = prs->get_rendering_device();
	// prd->texture_clear(prs->texture_get_rd_texture(this->_texture), Color(1.0, 1.0, 1.0), 0, 1, 0, 1);

	const auto *p_texture = dynamic_cast<RenderingDeviceVulkan *>(prd)->texture_owner.get_or_null(prs->texture_get_rd_texture(this->_texture));
	this->_receiver->RecvImageBlit(this->_channel_name, this->_texture_id, p_texture->layout);
#endif
}

void GodotTextureShare::_update_texture(const uint64_t width, const uint64_t height, const Image::Format format) {
	this->_width = width;
	this->_height = height;

	// Create new texture with correct width, height, and format
	Ref<Image> img = Image::create_empty(width, height, false, format);
	img->fill(Color(1.0f, 0.0f, 0.0f));

	RenderingServer *const prs = RenderingServer::get_singleton();
	assert(this->_texture.is_valid());

	// Replace texture (only way to change height and width)
	RID tmp_tex = prs->texture_2d_create(img);
	prs->texture_replace(this->_texture, tmp_tex);
	prs->free(tmp_tex);

	this->_texture_id = (texture_id_t)prs->texture_get_native_handle(this->_texture, true);
}

void GodotTextureShare::_create_initial_texture(const uint64_t width, const uint64_t height,
		const Image::Format format) {
	this->_width = width;
	this->_height = height;

	// Create simple texture
	Ref<Image> img = Image::create_empty(width, height, false, format);
	img->fill(Color(1.0f, 0.0f, 0.0f));

	RenderingServer *const prs = RenderingServer::get_singleton();
	this->_texture = prs->texture_2d_create(img);
	this->_texture_id = (texture_id_t)prs->texture_get_native_handle(this->_texture);

	// Force redraw
	prs->texture_set_force_redraw_if_visible(this->_texture, true);
}
