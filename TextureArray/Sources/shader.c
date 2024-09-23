#include <kinc/image.h>
#include <kinc/io/filereader.h>
#include <kinc/system.h>

#include <kope/graphics5/device.h>

#include <kong.h>

#include <assert.h>
#include <stdlib.h>

#ifdef SCREENSHOT
#include "../../screenshot.h"
#endif

static kope_g5_device device;
static kope_g5_command_list list;
static vertex_in_buffer vertices;
static kope_g5_buffer indices;
static kope_g5_buffer image_buffer;
static kope_g5_buffer image_buffer2;
static kope_g5_texture texture;
static kope_g5_sampler sampler;
static kope_g5_buffer constants;
static everything_set everything;

static const int width = 1024;
static const int height = 768;

static bool first_update = true;

static void update(void *data) {
	kinc_matrix3x3_t matrix = kinc_matrix3x3_rotation_z((float)kinc_time());

	constants_type *constants_data = constants_type_buffer_lock(&constants);
	constants_data->mvp = matrix;
	constants_type_buffer_unlock(&constants);

	if (first_update) {
		{
			kope_g5_image_copy_buffer source = {0};
			source.buffer = &image_buffer;
			source.bytes_per_row = 250 * 4;

			kope_g5_image_copy_texture destination = {0};
			destination.texture = &texture;
			destination.mip_level = 0;

			kope_g5_command_list_copy_buffer_to_texture(&list, &source, &destination, 250, 250, 1);
		}

		{
			kope_g5_image_copy_buffer source = {0};
			source.buffer = &image_buffer2;
			source.bytes_per_row = 250 * 4;

			kope_g5_image_copy_texture destination = {0};
			destination.texture = &texture;
			destination.mip_level = 0;
			destination.origin_z = 1;

			kope_g5_command_list_copy_buffer_to_texture(&list, &source, &destination, 250, 250, 1);
		}

		first_update = false;
	}

	kope_g5_texture *framebuffer = kope_g5_device_get_framebuffer(&device);

	kope_g5_render_pass_parameters parameters = {0};
	parameters.color_attachments_count = 1;
	parameters.color_attachments[0].load_op = KOPE_G5_LOAD_OP_CLEAR;
	kope_g5_color clear_color;
	clear_color.r = 0.0f;
	clear_color.g = 0.0f;
	clear_color.b = 0.0f;
	clear_color.a = 1.0f;
	parameters.color_attachments[0].clear_value = clear_color;
	parameters.color_attachments[0].texture = framebuffer;
	kope_g5_command_list_begin_render_pass(&list, &parameters);

	kong_set_render_pipeline(&list, &pipeline);

	kong_set_descriptor_set_everything(&list, &everything);

	kong_set_vertex_buffer_vertex_in(&list, &vertices);

	kope_g5_command_list_set_index_buffer(&list, &indices, KOPE_G5_INDEX_FORMAT_UINT16, 0, 3 * sizeof(uint16_t));

	kope_g5_command_list_draw_indexed(&list, 3, 1, 0, 0, 0);

	kope_g5_command_list_end_render_pass(&list);

	kope_g5_command_list_present(&list);

	kope_g5_device_execute_command_list(&device, &list);

#ifdef SCREENSHOT
	screenshot_take(&device, &list, framebuffer, width, height);
#endif
}

int kickstart(int argc, char **argv) {
	kinc_init("Example", width, height, NULL, NULL);
	kinc_set_update_callback(update, NULL);

	kope_g5_device_wishlist wishlist = {0};
	kope_g5_device_create(&device, &wishlist);

	kong_init(&device);

	kope_g5_device_create_command_list(&device, &list);

#ifdef SCREENSHOT
	screenshot_init_buffer(&device, width, height);
#endif

	{
		kope_g5_buffer_parameters buffer_parameters;
		buffer_parameters.size = 250 * 250 * 4;
		buffer_parameters.usage_flags = KOPE_G5_BUFFER_USAGE_CPU_WRITE;
		kope_g5_device_create_buffer(&device, &buffer_parameters, &image_buffer);

		kinc_image_t image;
		kinc_image_init_from_file(&image, kope_g5_buffer_lock(&image_buffer), "parrot.png");
		kinc_image_destroy(&image);
		kope_g5_buffer_unlock(&image_buffer);
	}

	{
		kope_g5_buffer_parameters buffer_parameters;
		buffer_parameters.size = 250 * 250 * 4;
		buffer_parameters.usage_flags = KOPE_G5_BUFFER_USAGE_CPU_WRITE;
		kope_g5_device_create_buffer(&device, &buffer_parameters, &image_buffer2);

		kinc_image_t image;
		kinc_image_init_from_file(&image, kope_g5_buffer_lock(&image_buffer2), "parrot2.png");
		kinc_image_destroy(&image);
		kope_g5_buffer_unlock(&image_buffer2);
	}

	kope_g5_texture_parameters texture_parameters;
	texture_parameters.width = 250;
	texture_parameters.height = 250;
	texture_parameters.depth_or_array_layers = 2;
	texture_parameters.mip_level_count = 1;
	texture_parameters.sample_count = 1;
	texture_parameters.dimension = KOPE_G5_TEXTURE_DIMENSION_2D;
	texture_parameters.format = KOPE_G5_TEXTURE_FORMAT_RGBA8_UNORM;
	texture_parameters.usage = KONG_G5_TEXTURE_USAGE_SAMPLE | KONG_G5_TEXTURE_USAGE_COPY_DST;
	kope_g5_device_create_texture(&device, &texture_parameters, &texture);

	kope_g5_sampler_parameters sampler_parameters;
	sampler_parameters.address_mode_u = KOPE_G5_ADDRESS_MODE_REPEAT;
	sampler_parameters.address_mode_v = KOPE_G5_ADDRESS_MODE_REPEAT;
	sampler_parameters.address_mode_w = KOPE_G5_ADDRESS_MODE_REPEAT;
	sampler_parameters.mag_filter = KOPE_G5_FILTER_MODE_LINEAR;
	sampler_parameters.min_filter = KOPE_G5_FILTER_MODE_LINEAR;
	sampler_parameters.mipmap_filter = KOPE_G5_MIPMAP_FILTER_MODE_NEAREST;
	sampler_parameters.lod_min_clamp = 1;
	sampler_parameters.lod_max_clamp = 32;
	sampler_parameters.compare = KOPE_G5_COMPARE_FUNCTION_ALWAYS;
	sampler_parameters.max_anisotropy = 1;
	kope_g5_device_create_sampler(&device, &sampler_parameters, &sampler);

	kong_create_buffer_vertex_in(&device, 3, &vertices);
	vertex_in *v = kong_vertex_in_buffer_lock(&vertices);

	v[0].pos.x = -1.0f;
	v[0].pos.y = -1.0f;
	v[0].pos.z = 0.5f;

	v[0].tex.x = 0.0f;
	v[0].tex.y = 1.0f;

	v[1].pos.x = 1.0f;
	v[1].pos.y = -1.0f;
	v[1].pos.z = 0.5f;

	v[1].tex.x = 1.0f;
	v[1].tex.y = 1.0f;

	v[2].pos.x = -1.0f;
	v[2].pos.y = 1.0f;
	v[2].pos.z = 0.5f;

	v[2].tex.x = 0.0f;
	v[2].tex.y = 0.0f;

	kong_vertex_in_buffer_unlock(&vertices);

	kope_g5_buffer_parameters params;
	params.size = 3 * sizeof(uint16_t);
	params.usage_flags = KOPE_G5_BUFFER_USAGE_INDEX | KOPE_G5_BUFFER_USAGE_CPU_WRITE;
	kope_g5_device_create_buffer(&device, &params, &indices);
	{
		uint16_t *i = (uint16_t *)kope_g5_buffer_lock(&indices);
		i[0] = 0;
		i[1] = 1;
		i[2] = 2;
		kope_g5_buffer_unlock(&indices);
	}

	constants_type_buffer_create(&device, &constants);

	{
		everything_parameters parameters;
		parameters.constants = &constants;
		parameters.tex = &texture;
		parameters.tex_highest_mip_level = 0;
		parameters.tex_mip_count = 1;
		parameters.sam = &sampler;
		kong_create_everything_set(&device, &parameters, &everything);
	}

	kinc_start();

	return 0;
}
