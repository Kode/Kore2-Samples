#include <kinc/image.h>
#include <kinc/input/keyboard.h>
#include <kinc/input/mouse.h>
#include <kinc/io/filereader.h>
#include <kinc/log.h>
#include <kinc/system.h>
#include <kinc/window.h>

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
static vertex_in_buffer vertices;
static kope_g5_buffer indices;
static kope_g5_buffer constants;
static kope_g5_texture texture;
static kope_g5_sampler sampler;
static everything_set everything;
static kope_g5_buffer image_buffer;

static uint32_t vertex_count;

float last_time = 0.0;
kinc_vector3_t position = {0, 0, 5};
float horizontal_angle = 3.14f;
float vertical_angle = 0.0;
bool move_forward = false;
bool move_backward = false;
bool strafe_left = false;
bool strafe_right = false;
bool is_mouse_down = false;
float mouse_x = 0.0;
float mouse_y = 0.0;
float mouse_delta_x = 0.0;
float mouse_delta_y = 0.0;

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

float vec4_length(kinc_vector3_t a) {
	return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
}

kinc_vector3_t vec4_normalize(kinc_vector3_t a) {
	float n = vec4_length(a);
	if (n > 0.0) {
		float inv_n = 1.0f / n;
		a.x *= inv_n;
		a.y *= inv_n;
		a.z *= inv_n;
	}
	return a;
}

kinc_vector3_t vec4_sub(kinc_vector3_t a, kinc_vector3_t b) {
	kinc_vector3_t v;
	v.x = a.x - b.x;
	v.y = a.y - b.y;
	v.z = a.z - b.z;
	return v;
}

kinc_vector3_t vec4_cross(kinc_vector3_t a, kinc_vector3_t b) {
	kinc_vector3_t v;
	v.x = a.y * b.z - a.z * b.y;
	v.y = a.z * b.x - a.x * b.z;
	v.z = a.x * b.y - a.y * b.x;
	return v;
}

float vec4_dot(kinc_vector3_t a, kinc_vector3_t b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

kinc_matrix4x4_t matrix4x4_perspective_projection(float fovy, float aspect, float zn, float zf) {
	float uh = 1.0f / tanf(fovy / 2);
	float uw = uh / aspect;
	kinc_matrix4x4_t m = {uw, 0, 0, 0, 0, uh, 0, 0, 0, 0, (zf + zn) / (zn - zf), -1, 0, 0, 2 * zf * zn / (zn - zf), 0};
	return m;
}

kinc_matrix4x4_t matrix4x4_look_at(kinc_vector3_t eye, kinc_vector3_t at, kinc_vector3_t up) {
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

kinc_matrix4x4_t matrix4x4_identity(void) {
	kinc_matrix4x4_t m;
	memset(m.m, 0, sizeof(m.m));
	for (unsigned x = 0; x < 4; ++x) {
		kinc_matrix4x4_set(&m, x, x, 1.0f);
	}
	return m;
}

void key_down(int key, void *data) {
	if (key == KINC_KEY_UP)
		move_forward = true;
	else if (key == KINC_KEY_DOWN)
		move_backward = true;
	else if (key == KINC_KEY_LEFT)
		strafe_left = true;
	else if (key == KINC_KEY_RIGHT)
		strafe_right = true;
}

void key_up(int key, void *data) {
	if (key == KINC_KEY_UP)
		move_forward = false;
	else if (key == KINC_KEY_DOWN)
		move_backward = false;
	else if (key == KINC_KEY_LEFT)
		strafe_left = false;
	else if (key == KINC_KEY_RIGHT)
		strafe_right = false;
}

void mouse_move(int window, int x, int y, int mx, int my, void *data) {
	mouse_delta_x = x - mouse_x;
	mouse_delta_y = y - mouse_y;
	mouse_x = (float)x;
	mouse_y = (float)y;
}

void mouse_down(int window, int button, int x, int y, void *data) {
	is_mouse_down = true;
}

void mouse_up(int window, int button, int x, int y, void *data) {
	is_mouse_down = false;
}

static bool first_update = true;

static void update(void *data) {
	float delta_time = (float)kinc_time() - last_time;
	last_time = (float)kinc_time();

	if (is_mouse_down) {
		horizontal_angle -= 0.005f * mouse_delta_x;
		vertical_angle -= 0.005f * mouse_delta_y;
	}
	mouse_delta_x = 0;
	mouse_delta_y = 0;

	kinc_vector3_t direction = {cosf(vertical_angle) * sinf(horizontal_angle), sinf(vertical_angle), cosf(vertical_angle) * cosf(horizontal_angle)};

	kinc_vector3_t right = {sinf(horizontal_angle - 3.14f / 2.0f), 0, cosf(horizontal_angle - 3.14f / 2.0f)};

	kinc_vector3_t up = vec4_cross(right, direction);

	if (move_forward) {
		position.x += (direction.x + delta_time) * 0.1f;
		position.y += (direction.y + delta_time) * 0.1f;
		position.z += (direction.z + delta_time) * 0.1f;
	}
	if (move_backward) {
		position.x -= (direction.x + delta_time) * 0.1f;
		position.y -= (direction.y + delta_time) * 0.1f;
		position.z -= (direction.z + delta_time) * 0.1f;
	}
	if (strafe_right) {
		position.x += (right.x + delta_time) * 0.1f;
		position.y += (right.y + delta_time) * 0.1f;
		position.z += (right.z + delta_time) * 0.1f;
	}
	if (strafe_left) {
		position.x -= (right.x + delta_time) * 0.1f;
		position.y -= (right.y + delta_time) * 0.1f;
		position.z -= (right.z + delta_time) * 0.1f;
	}

	kinc_vector3_t look = {position.x + direction.x, position.y + direction.y, position.z + direction.z};

	kinc_matrix4x4_t projection = matrix4x4_perspective_projection(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
	kinc_vector3_t v0 = {4, 3, 3};
	kinc_vector3_t v1 = {0, 0, 0};
	kinc_vector3_t v2 = {0, 1, 0};
	kinc_matrix4x4_t view = matrix4x4_look_at(position, look, up);
	kinc_matrix4x4_t model = matrix4x4_identity();
	kinc_matrix4x4_t mvp = matrix4x4_identity();
	mvp = kinc_matrix4x4_multiply(&mvp, &projection);
	mvp = kinc_matrix4x4_multiply(&mvp, &view);
	mvp = kinc_matrix4x4_multiply(&mvp, &model);

	constants_type *constants_data = constants_type_buffer_lock(&constants);
	constants_data->mvp = mvp;
	constants_type_buffer_unlock(&constants);

	if (first_update) {
		kope_g5_image_copy_buffer source = {0};
		source.buffer = &image_buffer;
		source.bytes_per_row = 512 * 4;

		kope_g5_image_copy_texture destination = {0};
		destination.texture = &texture;

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
	clear_color.b = 0.25f;
	clear_color.a = 1.0f;
	parameters.color_attachments[0].clear_value = clear_color;
	parameters.color_attachments[0].texture = framebuffer;
	kope_g5_command_list_begin_render_pass(&list, &parameters);

	kong_set_render_pipeline(&list, &pipeline);

	kong_set_vertex_buffer_vertex_in(&list, &vertices);

	kope_g5_command_list_set_index_buffer(&list, &indices, KOPE_G5_INDEX_FORMAT_UINT16, 0, vertex_count * sizeof(uint16_t));

	kong_set_descriptor_set_everything(&list, &everything);

	kope_g5_command_list_draw_indexed(&list, vertex_count, 1, 0, 0, 0);

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
	kinc_keyboard_set_key_down_callback(key_down, NULL);
	kinc_keyboard_set_key_up_callback(key_up, NULL);
	kinc_mouse_set_move_callback(mouse_move, NULL);
	kinc_mouse_set_press_callback(mouse_down, NULL);
	kinc_mouse_set_release_callback(mouse_up, NULL);

	kope_g5_device_wishlist wishlist = {0};
	kope_g5_device_create(&device, &wishlist);

	kong_init(&device);

	kope_g5_buffer_parameters buffer_parameters;
	buffer_parameters.size = 512 * 512 * 4;
	buffer_parameters.usage_flags = KOPE_G5_BUFFER_USAGE_CPU_WRITE;
	kope_g5_device_create_buffer(&device, &buffer_parameters, &image_buffer);

	kinc_image_t image;
	kinc_image_init_from_file(&image, kope_g5_buffer_lock(&image_buffer), "uvtemplate.png");
	kinc_image_destroy(&image);
	kope_g5_buffer_unlock(&image_buffer);

	kope_g5_texture_parameters texture_parameters;
	texture_parameters.width = 512;
	texture_parameters.height = 512;
	texture_parameters.depth_or_array_layers = 1;
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

	kope_g5_device_create_command_list(&device, &list);

	vertex_count = sizeof(vertices_data) / 3 / 4;
	kong_create_buffer_vertex_in(&device, vertex_count, &vertices);
	{
		vertex_in *v = kong_vertex_in_buffer_lock(&vertices);
		for (uint32_t i = 0; i < vertex_count; ++i) {
			v[i].pos.x = vertices_data[i * 3];
			v[i].pos.y = vertices_data[i * 3 + 1];
			v[i].pos.z = vertices_data[i * 3 + 2];
			v[i].tex.x = tex_data[i * 2];
			v[i].tex.y = tex_data[i * 2 + 1];
		}
		kong_vertex_in_buffer_unlock(&vertices);
	}

	kope_g5_buffer_parameters params;
	params.size = vertex_count * sizeof(uint16_t);
	params.usage_flags = KOPE_G5_BUFFER_USAGE_INDEX | KOPE_G5_BUFFER_USAGE_CPU_WRITE;
	kope_g5_device_create_buffer(&device, &params, &indices);
	{
		uint16_t *id = (uint16_t *)kope_g5_buffer_lock(&indices);
		for (uint32_t i = 0; i < vertex_count; ++i) {
			id[i] = i;
		}
		kope_g5_buffer_unlock(&indices);
	}

	constants_type_buffer_create(&device, &constants);

	{
		everything_parameters parameters;
		parameters.constants = &constants;
		parameters.pix_texture.texture = &texture;
		parameters.pix_texture.base_mip_level = 0;
		parameters.pix_texture.mip_level_count = 1;
		parameters.pix_texture.base_array_layer = 0;
		parameters.pix_texture.array_layer_count = 1;
		parameters.pix_sampler = &sampler;
		kong_create_everything_set(&device, &parameters, &everything);
	}

	kinc_start();

	return 0;
}
