#include <kinc/io/filereader.h>
#include <kinc/math/matrix.h>
#include <kinc/system.h>

#include <kong.h>

#include <assert.h>

#ifdef SCREENSHOT
#include "../../screenshot.h"
#endif

static kope_g5_device device;
static kope_g5_command_list list;
static vertex_in_buffer vertices;
static kope_g5_buffer indices;
static kope_g5_buffer constants;
static kope_g5_buffer compute_constants;
static kope_g5_texture texture;
static kope_g5_sampler sampler;
static everything_set everything;
static compute_set compute;
static kope_g5_buffer image_buffer;

static const int width = 800;
static const int height = 600;

void update(void *data) {
	constants_type *constants_data = constants_type_buffer_lock(&constants);
	kinc_matrix3x3_t matrix = kinc_matrix3x3_rotation_z(0);
	constants_data->mvp = matrix;
	constants_type_buffer_unlock(&constants);

	compute_constants_type *compute_constants_data = compute_constants_type_buffer_lock(&compute_constants);
	compute_constants_data->roll = 0;
	compute_constants_type_buffer_unlock(&compute_constants);

	kope_g5_texture *framebuffer = kope_g5_device_get_framebuffer(&device);

	kong_set_compute_pipeline(&list, &comp);
	kong_set_descriptor_set_compute(&list, &compute);
	kope_g5_command_list_compute(&list, 256 / 16, 256 / 16, 1);

	kope_g5_render_pass_parameters parameters = {0};
	parameters.color_attachments_count = 1;
	parameters.color_attachments[0].load_op = KOPE_G5_LOAD_OP_CLEAR;
	kope_g5_color clear_color;
	clear_color.r = 0.0f;
	clear_color.g = 0.0f;
	clear_color.b = 0.25f;
	clear_color.a = 1.0f;
	parameters.color_attachments[0].clear_value = clear_color;
	parameters.color_attachments[0].texture = framebuffer;
	kope_g5_command_list_begin_render_pass(&list, &parameters);

	kong_set_render_pipeline(&list, &pipeline);

	kong_set_vertex_buffer_vertex_in(&list, &vertices);

	kope_g5_command_list_set_index_buffer(&list, &indices, KOPE_G5_INDEX_FORMAT_UINT16, 0, 3 * sizeof(uint16_t));

	kong_set_descriptor_set_everything(&list, &everything);

	kope_g5_command_list_draw_indexed(&list, 3, 1, 0, 0, 0);

	kope_g5_command_list_end_render_pass(&list);

	kope_g5_command_list_present(&list);

	kope_g5_device_execute_command_list(&device, &list);

#ifdef SCREENSHOT
	screenshot_take(&device, &list, framebuffer, width, height);
#endif
}

int kickstart(int argc, char **argv) {
	kinc_init("Compute", width, height, NULL, NULL);
	kinc_set_update_callback(update, NULL);

	kope_g5_device_wishlist wishlist = {0};
	kope_g5_device_create(&device, &wishlist);

	kong_init(&device);

	kope_g5_texture_parameters texture_parameters;
	texture_parameters.width = 256;
	texture_parameters.height = 256;
	texture_parameters.depth_or_array_layers = 1;
	texture_parameters.mip_level_count = 1;
	texture_parameters.sample_count = 1;
	texture_parameters.dimension = KOPE_G5_TEXTURE_DIMENSION_2D;
	texture_parameters.format = KOPE_G5_TEXTURE_FORMAT_RGBA32_FLOAT;
	texture_parameters.usage = KONG_G5_TEXTURE_USAGE_SAMPLE | KONG_G5_TEXTURE_USAGE_READ_WRITE;
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

	kope_g5_device_create_command_list(&device, &list);

	kong_create_buffer_vertex_in(&device, 3, &vertices);
	{
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
	}

	kope_g5_buffer_parameters params;
	params.size = 3 * sizeof(uint16_t);
	params.usage_flags = KOPE_G5_BUFFER_USAGE_INDEX | KOPE_G5_BUFFER_USAGE_CPU_WRITE;
	kope_g5_device_create_buffer(&device, &params, &indices);
	{
		uint16_t *id = (uint16_t *)kope_g5_buffer_lock(&indices);
		id[0] = 0;
		id[1] = 1;
		id[2] = 2;
		kope_g5_buffer_unlock(&indices);
	}

	constants_type_buffer_create(&device, &constants);

	{
		everything_parameters parameters;
		parameters.constants = &constants;
		parameters.comp_texture = &texture;
		parameters.comp_texture_highest_mip_level = 0;
		parameters.comp_texture_mip_count = 1;
		parameters.comp_sampler = &sampler;
		kong_create_everything_set(&device, &parameters, &everything);
	}

	compute_constants_type_buffer_create(&device, &compute_constants);

	{
		compute_parameters parameters;
		parameters.compute_constants = &compute_constants;
		parameters.dest_texture = &texture;
		parameters.dest_texture_mip_level = 0;
		kong_create_compute_set(&device, &parameters, &compute);
	}

	kinc_start();

	return 0;
}
