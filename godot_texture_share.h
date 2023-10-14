#pragma once

#include <core/io/image.h>
#include <scene/resources/texture.h>

#define VK_NO_PROTOTYPES
#include "rendering_backend.h"

class GodotTextureShare : public Texture2D {
	GDCLASS(GodotTextureShare, Texture2D);

public:
	GodotTextureShare();
	~GodotTextureShare() override;

	static texture_format_t convert_godot_to_rendering_device_format(Image::Format format);
	static Image::Format convert_rendering_device_to_godot_format(texture_format_t format);

	void _init();
	static void _register_methods();

	int32_t get_width() const override { return this->_width; }

	int32_t get_height() const override { return this->_height; }

	bool has_alpha() const override { return true; }

	// virtual RID _get_rid() override { return this->_texture; }

	virtual void set_flags(const int64_t flags) { _flags = flags; }

	virtual int64_t get_flags() const { return _flags; }

	void draw(RID p_canvas_item, const Point2 &p_pos, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	void draw_rect(RID p_canvas_item, const Rect2 &p_rect, bool p_tile = false, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	void draw_rect_region(RID p_canvas_item, const Rect2 &p_rect, const Rect2 &p_src_rect, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false, bool p_clip_uv = true) const override;

	String get_shared_name() const;
	void set_shared_name(String shared_name);

protected:
	static void _bind_methods();

	bool _create_receiver(const std::string &name);
	void _receive_texture();
	void _update_texture(const uint64_t width, const uint64_t height, const Image::Format format);

private:
	// Texture
	RID _texture = RID();
	texture_id_t _texture_id = 0;
	int64_t _width = 0, _height = 0;
	uint32_t _flags;

	// TextureShareReceiver
	std::unique_ptr<texture_share_client_t> _receiver = nullptr;
	std::string _channel_name;

	void _create_initial_texture(const uint64_t width, const uint64_t height, const Image::Format format);
};
