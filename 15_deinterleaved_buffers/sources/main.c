#include <kinc/image.h>
#include <kinc/io/filereader.h>
#include <kinc/system.h>

#include <kope/graphics5/device.h>

#include <kong.h>

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef SCREENSHOT
#include "../../screenshot.h"
#endif

static kope_g5_device device;
static kope_g5_command_list list;
static vertex_in_buffer pos_vertices;
static vertex_tex_in_buffer tex_vertices;
static kope_g5_buffer indices;
static kope_g5_texture texture;
static kope_g5_buffer image_buffer;
static kope_g5_buffer constants;
static kope_g5_sampler sampler;
static everything_set everything;

static const int width = 800;
static const int height = 600;

/* clang-format off */
static float vertices_data[] = {
    -1.0,-1.0,-1.0,
	-1.0,-1.0, 1.0,
	-1.0, 1.0, 1.0,
	 1.0, 1.0,-1.0,
	-1.0,-1.0,-1.0,
	-1.0, 1.0,-1.0,
	 1.0,-1.0, 1.0,
	-1.0,-1.0,-1.0,
	 1.0,-1.0,-1.0,
	 1.0, 1.0,-1.0,
	 1.0,-1.0,-1.0,
	-1.0,-1.0,-1.0,
	-1.0,-1.0,-1.0,
	-1.0, 1.0, 1.0,
	-1.0, 1.0,-1.0,
	 1.0,-1.0, 1.0,
	-1.0,-1.0, 1.0,
	-1.0,-1.0,-1.0,
	-1.0, 1.0, 1.0,
	-1.0,-1.0, 1.0,
	 1.0,-1.0, 1.0,
	 1.0, 1.0, 1.0,
	 1.0,-1.0,-1.0,
	 1.0, 1.0,-1.0,
	 1.0,-1.0,-1.0,
	 1.0, 1.0, 1.0,
	 1.0,-1.0, 1.0,
	 1.0, 1.0, 1.0,
	 1.0, 1.0,-1.0,
	-1.0, 1.0,-1.0,
	 1.0, 1.0, 1.0,
	-1.0, 1.0,-1.0,
	-1.0, 1.0, 1.0,
	 1.0, 1.0, 1.0,
	-1.0, 1.0, 1.0,
	 1.0,-1.0, 1.0
};

static float tex_data[] = {
    0.000059f, 0.000004f,
	0.000103f, 0.336048f,
	0.335973f, 0.335903f,
	1.000023f, 0.000013f,
	0.667979f, 0.335851f,
	0.999958f, 0.336064f,
	0.667979f, 0.335851f,
	0.336024f, 0.671877f,
	0.667969f, 0.671889f,
	1.000023f, 0.000013f,
	0.668104f, 0.000013f,
	0.667979f, 0.335851f,
	0.000059f, 0.000004f,
	0.335973f, 0.335903f,
	0.336098f, 0.000071f,
	0.667979f, 0.335851f,
	0.335973f, 0.335903f,
	0.336024f, 0.671877f,
	1.000004f, 0.671847f,
	0.999958f, 0.336064f,
	0.667979f, 0.335851f,
	0.668104f, 0.000013f,
	0.335973f, 0.335903f,
	0.667979f, 0.335851f,
	0.335973f, 0.335903f,
	0.668104f, 0.000013f,
	0.336098f, 0.000071f,
	0.000103f, 0.336048f,
	0.000004f, 0.671870f,
	0.336024f, 0.671877f,
	0.000103f, 0.336048f,
	0.336024f, 0.671877f,
	0.335973f, 0.335903f,
	0.667969f, 0.671889f,
	1.000004f, 0.671847f,
	0.667979f, 0.335851f
};
/* clang-format on */

static float vec4_length(kinc_vector3_t a) {
	return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
}

static kinc_vector3_t vec4_normalize(kinc_vector3_t a) {
	float n = vec4_length(a);
	if (n > 0.0) {
		float inv_n = 1.0f / n;
		a.x *= inv_n;
		a.y *= inv_n;
		a.z *= inv_n;
	}
	return a;
}

static kinc_vector3_t vec4_sub(kinc_vector3_t a, kinc_vector3_t b) {
	kinc_vector3_t v;
	v.x = a.x - b.x;
	v.y = a.y - b.y;
	v.z = a.z - b.z;
	return v;
}

static kinc_vector3_t vec4_cross(kinc_vector3_t a, kinc_vector3_t b) {
	kinc_vector3_t v;
	v.x = a.y * b.z - a.z * b.y;
	v.y = a.z * b.x - a.x * b.z;
	v.z = a.x * b.y - a.y * b.x;
	return v;
}

static float vec4_dot(kinc_vector3_t a, kinc_vector3_t b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

static kinc_matrix4x4_t matrix4x4_perspective_projection(float fovy, float aspect, float zn, float zf) {
	float uh = 1.0f / tanf(fovy / 2);
	float uw = uh / aspect;
	kinc_matrix4x4_t m = {uw, 0, 0, 0, 0, uh, 0, 0, 0, 0, (zf + zn) / (zn - zf), -1, 0, 0, 2 * zf * zn / (zn - zf), 0};
	return m;
}

static kinc_matrix4x4_t matrix4x4_look_at(kinc_vector3_t eye, kinc_vector3_t at, kinc_vector3_t up) {
	kinc_vector3_t zaxis = vec4_normalize(vec4_sub(at, eye));
	kinc_vector3_t xaxis = vec4_normalize(vec4_cross(zaxis, up));
	kinc_vector3_t yaxis = vec4_cross(xaxis, zaxis);
	kinc_matrix4x4_t m = {xaxis.x,
	                      yaxis.x,
	                      -zaxis.x,
	                      0,
	                      xaxis.y,
	                      yaxis.y,
	                      -zaxis.y,
	                      0,
	                      xaxis.z,
	                      yaxis.z,
	                      -zaxis.z,
	                      0,
	                      -vec4_dot(xaxis, eye),
	                      -vec4_dot(yaxis, eye),
	                      vec4_dot(zaxis, eye),
	                      1};
	return m;
}

static kinc_matrix4x4_t matrix4x4_identity(void) {
	kinc_matrix4x4_t m;
	memset(m.m, 0, sizeof(m.m));
	for (unsigned x = 0; x < 4; ++x) {
		kinc_matrix4x4_set(&m, x, x, 1.0f);
	}
	return m;
}

static bool first_update = true;

static size_t vertex_count = 0;

static void update(void *data) {
	kinc_matrix4x4_t projection = matrix4x4_perspective_projection(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
	kinc_vector3_t v0 = {4, 3, 3};
	kinc_vector3_t v1 = {0, 0, 0};
	kinc_vector3_t v2 = {0, 1, 0};
	kinc_matrix4x4_t view = matrix4x4_look_at(v0, v1, v2);
	kinc_matrix4x4_t model = matrix4x4_identity();
	kinc_matrix4x4_t mvp = matrix4x4_identity();
	mvp = kinc_matrix4x4_multiply(&mvp, &projection);
	mvp = kinc_matrix4x4_multiply(&mvp, &view);
	mvp = kinc_matrix4x4_multiply(&mvp, &model);

	constants_type *constants_data = constants_type_buffer_lock(&constants, 0, 1);
	constants_data->mvp = mvp;
	constants_type_buffer_unlock(&constants);

	if (first_update) {
		kope_g5_image_copy_buffer source = {0};
		source.buffer = &image_buffer;
		source.bytes_per_row = 4 * 512;

		kope_g5_image_copy_texture destination = {0};
		destination.texture = &texture;
		destination.mip_level = 0;

		kope_g5_command_list_copy_buffer_to_texture(&list, &source, &destination, 512, 512, 1);

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
	parameters.color_attachments[0].texture.texture = framebuffer;
	parameters.color_attachments[0].texture.array_layer_count = 1;
	parameters.color_attachments[0].texture.mip_level_count = 1;
	parameters.color_attachments[0].texture.format = KOPE_G5_TEXTURE_FORMAT_BGRA8_UNORM;
	parameters.color_attachments[0].texture.dimension = KOPE_G5_TEXTURE_VIEW_DIMENSION_2D;
	kope_g5_command_list_begin_render_pass(&list, &parameters);

	kong_set_render_pipeline(&list, &pipeline);

	kong_set_descriptor_set_everything(&list, &everything);

	kong_set_vertex_buffer_vertex_in(&list, &pos_vertices);

	kong_set_vertex_buffer_vertex_tex_in(&list, &tex_vertices);

	kope_g5_command_list_set_index_buffer(&list, &indices, KOPE_G5_INDEX_FORMAT_UINT16, 0, vertex_count * sizeof(uint16_t));

	kope_g5_command_list_draw_indexed(&list, (uint32_t)vertex_count, 1, 0, 0, 0);

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

	kope_g5_device_create_command_list(&device, KOPE_G5_COMMAND_LIST_TYPE_GRAPHICS, &list);

	{
		kope_g5_buffer_parameters buffer_parameters;
		buffer_parameters.size = kope_g5_device_align_texture_row_bytes(&device, 512 * 4) * 512;
		buffer_parameters.usage_flags = KOPE_G5_BUFFER_USAGE_CPU_WRITE;
		kope_g5_device_create_buffer(&device, &buffer_parameters, &image_buffer);

		kinc_image_t image;
		kinc_image_init_from_file_with_stride(&image, kope_g5_buffer_lock_all(&image_buffer), "uvtemplate.png",
		                                      kope_g5_device_align_texture_row_bytes(&device, 512 * 4));
		kinc_image_destroy(&image);
		kope_g5_buffer_unlock(&image_buffer);
	}

	vertex_count = sizeof(vertices_data) / 3 / 4;

	{
		kong_create_buffer_vertex_in(&device, vertex_count, &pos_vertices);
		vertex_in *v = kong_vertex_in_buffer_lock(&pos_vertices);

		for (int i = 0; i < vertex_count; ++i) {
			v[i].pos.x = vertices_data[i * 3];
			v[i].pos.y = vertices_data[i * 3 + 1];
			v[i].pos.z = vertices_data[i * 3 + 2];
		}

		kong_vertex_in_buffer_unlock(&pos_vertices);
	}

	{
		kong_create_buffer_vertex_tex_in(&device, vertex_count, &tex_vertices);
		vertex_tex_in *v = kong_vertex_tex_in_buffer_lock(&tex_vertices);

		for (int i = 0; i < vertex_count; ++i) {
			v[i].tex.x = tex_data[i * 2];
			v[i].tex.y = tex_data[i * 2 + 1];
		}

		kong_vertex_tex_in_buffer_unlock(&tex_vertices);
	}

	kope_g5_texture_parameters texture_params = {0};
	texture_params.width = 512;
	texture_params.height = 512;
	texture_params.depth_or_array_layers = 1;
	texture_params.mip_level_count = 1;
	texture_params.sample_count = 1;
	texture_params.dimension = KOPE_G5_TEXTURE_DIMENSION_2D;
	texture_params.format = KOPE_G5_TEXTURE_FORMAT_RGBA8_UNORM;
	texture_params.usage = KONG_G5_TEXTURE_USAGE_SAMPLE;
	kope_g5_device_create_texture(&device, &texture_params, &texture);

	kope_g5_sampler_parameters sampler_params = {0};
	sampler_params.lod_min_clamp = 0;
	sampler_params.lod_max_clamp = 0;
	kope_g5_device_create_sampler(&device, &sampler_params, &sampler);

	kope_g5_buffer_parameters params;
	params.size = vertex_count * sizeof(uint16_t);
	params.usage_flags = KOPE_G5_BUFFER_USAGE_INDEX | KOPE_G5_BUFFER_USAGE_CPU_WRITE;
	kope_g5_device_create_buffer(&device, &params, &indices);
	{
		uint16_t *id = (uint16_t *)kope_g5_buffer_lock_all(&indices);
		for (int i = 0; i < vertex_count; ++i) {
			id[i] = i;
		}
		kope_g5_buffer_unlock(&indices);
	}

	constants_type_buffer_create(&device, &constants, 1);

	{
		everything_parameters parameters;
		parameters.constants = &constants;
		parameters.tex.texture = &texture;
		parameters.tex.base_mip_level = 0;
		parameters.tex.mip_level_count = 1;
		parameters.tex.base_array_layer = 0;
		parameters.tex.array_layer_count = 1;
		parameters.sam = &sampler;
		kong_create_everything_set(&device, &parameters, &everything);
	}

	kinc_start();

	return 0;
}
