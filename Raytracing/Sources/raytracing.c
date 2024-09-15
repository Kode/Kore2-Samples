#include <kinc/io/filereader.h>
#include <kinc/math/matrix.h>
#include <kinc/system.h>

#include <kong.h>

#include <assert.h>
#include <string.h>

static kope_g5_device device;
static kope_g5_command_list list;
static vertex_in_buffer vertices;
static kope_g5_buffer indices;
static kope_g5_buffer constants;
static kope_g5_buffer compute_constants;
static kope_g5_texture texture;
static kope_g5_sampler sampler;
static everything_set everything;
static rayset_set rayset;

static float quadVtx[] = {-1, 0, -1, -1, 0, 1, 1, 0, 1, -1, 0, -1, 1, 0, -1, 1, 0, 1};
static float cubeVtx[] = {-1, -1, -1, 1, -1, -1, -1, 1, -1, 1, 1, -1, -1, -1, 1, 1, -1, 1, -1, 1, 1, 1, 1, 1};
static uint16_t cubeIdx[] = {4, 6, 0, 2, 0, 6, 0, 1, 4, 5, 4, 1, 0, 2, 1, 3, 1, 2, 1, 3, 5, 7, 5, 3, 2, 6, 3, 7, 3, 6, 4, 5, 6, 7, 6, 5};

static kope_g5_buffer quadVB;
static kope_g5_buffer cubeVB;
static kope_g5_buffer cubeIB;

static kope_g5_raytracing_volume cubeBlas;
static kope_g5_raytracing_volume quadBlas;

static kope_g5_raytracing_hierarchy hierarchy;
static kope_g5_texture render_target;

#define WIDTH 1024
#define HEIGHT 768

static bool first = true;

void update(void *data) {
	constants_type *constants_data = constants_type_buffer_lock(&constants);
	kinc_matrix3x3_t matrix = kinc_matrix3x3_rotation_z(0);
	constants_data->mvp = matrix;
	constants_type_buffer_unlock(&constants);

	kope_g5_texture *framebuffer = kope_g5_device_get_framebuffer(&device);

	if (first) {
		first = false;

		kope_g5_command_list_prepare_raytracing_volume(&list, &cubeBlas);
		kope_g5_command_list_prepare_raytracing_volume(&list, &quadBlas);
		kope_g5_command_list_prepare_raytracing_hierarchy(&list, &hierarchy);
	}

	kong_set_ray_pipeline(&list, &RayPipe);

	kong_set_descriptor_set_rayset(&list, &rayset);

	kope_g5_command_list_trace_rays(&list);

	kope_g5_render_pass_parameters parameters = {0};
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
}

int kickstart(int argc, char **argv) {
	kinc_init("Raytracing", 1024, 768, NULL, NULL);
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

	{
		kope_g5_buffer_parameters params;
		params.size = sizeof(quadVtx);
		params.usage_flags = KOPE_G5_BUFFER_USAGE_CPU_WRITE;
		kope_g5_device_create_buffer(&device, &params, &quadVB);

		void *data = kope_g5_buffer_lock(&quadVB);
		memcpy(data, quadVtx, sizeof(quadVtx));
		kope_g5_buffer_unlock(&quadVB);
	}

	kope_g5_device_create_raytracing_volume(&device, &quadVB, sizeof(quadVtx) / 4 / 3, NULL, 0, &quadBlas);

	{
		kope_g5_buffer_parameters params;
		params.size = sizeof(cubeVtx);
		params.usage_flags = KOPE_G5_BUFFER_USAGE_CPU_WRITE;
		kope_g5_device_create_buffer(&device, &params, &cubeVB);

		void *data = kope_g5_buffer_lock(&cubeVB);
		memcpy(data, cubeVtx, sizeof(cubeVtx));
		kope_g5_buffer_unlock(&cubeVB);
	}

	{
		kope_g5_buffer_parameters params;
		params.size = sizeof(cubeIdx);
		params.usage_flags = KOPE_G5_BUFFER_USAGE_CPU_WRITE;
		kope_g5_device_create_buffer(&device, &params, &cubeIB);

		void *data = kope_g5_buffer_lock(&cubeIB);
		memcpy(data, cubeIdx, sizeof(cubeIdx));
		kope_g5_buffer_unlock(&cubeIB);
	}

	kope_g5_device_create_raytracing_volume(&device, &cubeVB, sizeof(cubeVtx) / 4 / 3, NULL, 0, &cubeBlas);

	kope_g5_raytracing_volume *volumes[] = {&cubeBlas, &quadBlas, &quadBlas};
	kope_g5_device_create_raytracing_hierarchy(&device, volumes, 3, &hierarchy);

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
		parameters.comp_sampler = &sampler;
		kong_create_everything_set(&device, &parameters, &everything);
	}

	kope_g5_texture_parameters render_target_parameters;
	render_target_parameters.width = 1024;
	render_target_parameters.height = 768;
	render_target_parameters.depth_or_array_layers = 1;
	render_target_parameters.mip_level_count = 1;
	render_target_parameters.sample_count = 1;
	render_target_parameters.dimension = KOPE_G5_TEXTURE_DIMENSION_2D;
	render_target_parameters.format = KOPE_G5_TEXTURE_FORMAT_RGBA8_UNORM;
	render_target_parameters.usage = KONG_G5_TEXTURE_USAGE_READ_WRITE;
	kope_g5_device_create_texture(&device, &render_target_parameters, &render_target);

	{
		rayset_parameters parameters;
		parameters.scene = &hierarchy.d3d12.acceleration_structure;
		parameters.uav = &render_target;
		kong_create_rayset_set(&device, &parameters, &rayset);
	}

	kinc_start();

	return 0;
}
