#pragma once

#include <texture_share_vk/opengl/texture_share_gl_client.h>

#include <scene/resources/texture.h>
#include <servers/rendering_server.h>

#include "rendering_backend.h"

class TextureSender : public Resource {
	GDCLASS(TextureSender, Resource);

public:
	TextureSender();
	~TextureSender() override = default;

	void set_texture(const Ref<Texture2D> &texture, Image::Format texture_format);
	Ref<Texture2D> get_texture();

	void set_shared_texture_name(const String &shared_texture_name);
	String get_shared_texture_name();

	bool send_texture();

protected:
	static void _bind_methods();

private:
	Ref<Texture2D> _texture;

	std::unique_ptr<texture_share_client_t> _client = nullptr;

	std::string _shared_texture_name;
	uint32_t _width = 0;
	uint32_t _height = 0;
	Image::Format _format = Image::FORMAT_MAX;

	bool update_shared_texture(uint32_t width, uint32_t height, Image::Format format);
	bool check_and_update_shared_texture(Image::Format format);
};
