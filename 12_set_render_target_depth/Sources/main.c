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
static fs_vertex_in_buffer vertices_fs;
static kope_g5_buffer indices;
static kope_g5_texture target_depth;
static kope_g5_sampler sampler;
static fs_set set;

static const int width = 800;
static const int height = 600;

static void update(void *data) {
	{
		kope_g5_render_pass_parameters parameters = {0};
		parameters.color_attachments_count = 0;
		parameters.depth_stencil_attachment.texture = &target_depth;
		parameters.depth_stencil_attachment.depth_load_op = KOPE_G5_LOAD_OP_CLEAR;
		parameters.depth_stencil_attachment.depth_clear_value = 1.0f;
		kope_g5_command_list_begin_render_pass(&list, &parameters);

		kong_set_render_pipeline(&list, &pipeline);

		kong_set_vertex_buffer_vertex_in(&list, &vertices);

		kope_g5_command_list_set_index_buffer(&list, &indices, KOPE_G5_INDEX_FORMAT_UINT16, 0, 3 * sizeof(uint16_t));

		kope_g5_command_list_draw_indexed(&list, 3, 1, 0, 0, 0);

		kope_g5_command_list_end_render_pass(&list);
	}

	kope_g5_texture *framebuffer = kope_g5_device_get_framebuffer(&device);

	{
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

		kong_set_render_pipeline(&list, &fs_pipeline);

		kong_set_descriptor_set_fs(&list, &set);

		kong_set_vertex_buffer_fs_vertex_in(&list, &vertices_fs);

		kope_g5_command_list_set_index_buffer(&list, &indices, KOPE_G5_INDEX_FORMAT_UINT16, 0, 3 * sizeof(uint16_t));

		kope_g5_command_list_draw_indexed(&list, 3, 1, 0, 0, 0);

		kope_g5_command_list_end_render_pass(&list);
	}

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

	kope_g5_texture_parameters texture_params = {0};
	texture_params.format = KOPE_G5_TEXTURE_FORMAT_DEPTH32FLOAT;
	texture_params.width = width;
	texture_params.height = height;
	texture_params.depth_or_array_layers = 1;
	texture_params.dimension = KOPE_G5_TEXTURE_DIMENSION_2D;
	texture_params.mip_level_count = 1;
	texture_params.sample_count = 1;
	texture_params.usage = KONG_G5_TEXTURE_USAGE_RENDER_ATTACHMENT;
	kope_g5_device_create_texture(&device, &texture_params, &target_depth);

	kope_g5_sampler_parameters sampler_params = {0};
	kope_g5_device_create_sampler(&device, &sampler_params, &sampler);

	fs_parameters set_parameters = {0};
	set_parameters.fs_texture.texture = &target_depth;
	set_parameters.fs_texture.base_mip_level = 0;
	set_parameters.fs_texture.mip_level_count = 1;
	set_parameters.fs_texture.base_array_layer = 0;
	set_parameters.fs_texture.array_layer_count = 1;
	set_parameters.fs_sampler = &sampler;
	kong_create_fs_set(&device, &set_parameters, &set);

	{
		kong_create_buffer_vertex_in(&device, 3, &vertices);
		vertex_in *v = kong_vertex_in_buffer_lock(&vertices);

		v[0].pos.x = -1.0f;
		v[0].pos.y = 1.0f;
		v[0].pos.z = 0.5f;

		v[1].pos.x = 1.0f;
		v[1].pos.y = 1.0f;
		v[1].pos.z = 0.5f;

		v[2].pos.x = 0.0f;
		v[2].pos.y = -1.0f;
		v[2].pos.z = 0.5f;

		kong_vertex_in_buffer_unlock(&vertices);
	}

	{
		kong_create_buffer_fs_vertex_in(&device, 3, &vertices_fs);
		fs_vertex_in *v = kong_fs_vertex_in_buffer_lock(&vertices_fs);

		v[0].pos.x = -1.0f;
		v[0].pos.y = -1.0f;

		v[1].pos.x = 3.0f;
		v[1].pos.y = -1.0f;

		v[2].pos.x = -1.0f;
		v[2].pos.y = 3.0f;

		kong_fs_vertex_in_buffer_unlock(&vertices_fs);
	}

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

	kinc_start();

	return 0;
}
