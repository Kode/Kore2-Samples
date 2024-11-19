#include <kinc/io/filereader.h>
#include <kinc/math/matrix.h>
#include <kinc/system.h>

#include <kong.h>

#include <assert.h>
#include <math.h>
#include <string.h>

#ifdef SCREENSHOT
#include "../../screenshot.h"
#endif

static kope_g5_device device;
static kope_g5_command_list list;
static kope_g5_texture texture;
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

static const uint32_t width = 1920;
static const uint32_t height = 1080;

static kinc_matrix4x4_t transforms[3];

static void update_transforms(void) {
	float time = (float)kinc_time();

	{
		kinc_matrix4x4_t cube = kinc_matrix4x4_rotation_y(time / 3);
		kinc_matrix4x4_t a = kinc_matrix4x4_rotation_x(time / 2);
		cube = kinc_matrix4x4_multiply(&a, &cube);
		kinc_matrix4x4_t b = kinc_matrix4x4_rotation_z(time / 5);
		cube = kinc_matrix4x4_multiply(&b, &cube);
		kinc_matrix4x4_t c = kinc_matrix4x4_translation(-1.5, 2, 2);
		cube = kinc_matrix4x4_multiply(&c, &cube);

		transforms[0] = cube;
	}

	{
		kinc_matrix4x4_t mirror = kinc_matrix4x4_rotation_x(-1.8f);
		kinc_matrix4x4_t a = kinc_matrix4x4_rotation_y(sinf(time) / 8 + 1);
		mirror = kinc_matrix4x4_multiply(&a, &mirror);
		kinc_matrix4x4_t b = kinc_matrix4x4_translation(2, 2, 2);
		mirror = kinc_matrix4x4_multiply(&b, &mirror);

		transforms[1] = mirror;
	}

	{
		kinc_matrix4x4_t floor = kinc_matrix4x4_scale(5, 5, 5);
		kinc_matrix4x4_t a = kinc_matrix4x4_translation(0, 0, 2);
		floor = kinc_matrix4x4_multiply(&a, &floor);

		transforms[2] = floor;
	}
}

static bool first = true;
static uint64_t frame = 0;

void update(void *data) {
	kope_g5_texture *framebuffer = kope_g5_device_get_framebuffer(&device);

	if (first) {
		first = false;

		kope_g5_command_list_prepare_raytracing_volume(&list, &cubeBlas);
		kope_g5_command_list_prepare_raytracing_volume(&list, &quadBlas);
		kope_g5_command_list_prepare_raytracing_hierarchy(&list, &hierarchy);
	}

	update_transforms();
	kope_g5_command_list_update_raytracing_hierarchy(&list, transforms, 3, &hierarchy);

	kong_set_ray_pipeline(&list, &ray_pipe);

	kong_set_descriptor_set_rayset(&list, &rayset);

	kope_g5_command_list_trace_rays(&list, width, height, 1);

	kope_g5_image_copy_texture source = {0};
	source.texture = &texture;

	kope_g5_image_copy_texture destination = {0};
	destination.texture = framebuffer;

	kope_g5_command_list_copy_texture_to_texture(&list, &source, &destination, width, height, 1);

	kope_g5_command_list_present(&list);

	kope_g5_device_execute_command_list(&device, &list);

#ifdef SCREENSHOT
	if (frame == 3) {
		screenshot_take(&device, &list, framebuffer, width, height);
	}
#endif

	++frame;
}

int kickstart(int argc, char **argv) {
	kinc_init("Raytracing", width, height, NULL, NULL);
	kinc_set_update_callback(update, NULL);

	kope_g5_device_wishlist wishlist = {0};
	kope_g5_device_create(&device, &wishlist);

	kong_init(&device);

	kope_g5_texture_parameters render_target_parameters;
	render_target_parameters.width = width;
	render_target_parameters.height = height;
	render_target_parameters.depth_or_array_layers = 1;
	render_target_parameters.mip_level_count = 1;
	render_target_parameters.sample_count = 1;
	render_target_parameters.dimension = KOPE_G5_TEXTURE_DIMENSION_2D;
	render_target_parameters.format = KOPE_G5_TEXTURE_FORMAT_RGBA8_UNORM;
	render_target_parameters.usage = KONG_G5_TEXTURE_USAGE_READ_WRITE;
	kope_g5_device_create_texture(&device, &render_target_parameters, &texture);

	kope_g5_device_create_command_list(&device, &list);

	{
		kope_g5_buffer_parameters params;
		params.size = sizeof(quadVtx);
		params.usage_flags = KOPE_G5_BUFFER_USAGE_CPU_WRITE;
		kope_g5_device_create_buffer(&device, &params, &quadVB);

		void *data = kope_g5_buffer_lock_all(&quadVB);
		memcpy(data, quadVtx, sizeof(quadVtx));
		kope_g5_buffer_unlock(&quadVB);
	}

	kope_g5_device_create_raytracing_volume(&device, &quadVB, sizeof(quadVtx) / 4 / 3, NULL, 0, &quadBlas);

	{
		kope_g5_buffer_parameters params;
		params.size = sizeof(cubeVtx);
		params.usage_flags = KOPE_G5_BUFFER_USAGE_CPU_WRITE;
		kope_g5_device_create_buffer(&device, &params, &cubeVB);

		void *data = kope_g5_buffer_lock_all(&cubeVB);
		memcpy(data, cubeVtx, sizeof(cubeVtx));
		kope_g5_buffer_unlock(&cubeVB);
	}

	{
		kope_g5_buffer_parameters params;
		params.size = sizeof(cubeIdx);
		params.usage_flags = KOPE_G5_BUFFER_USAGE_CPU_WRITE;
		kope_g5_device_create_buffer(&device, &params, &cubeIB);

		void *data = kope_g5_buffer_lock_all(&cubeIB);
		memcpy(data, cubeIdx, sizeof(cubeIdx));
		kope_g5_buffer_unlock(&cubeIB);
	}

	kope_g5_device_create_raytracing_volume(&device, &cubeVB, sizeof(cubeVtx) / 4 / 3, &cubeIB, sizeof(cubeIdx) / 2, &cubeBlas);

	kope_g5_raytracing_volume *volumes[] = {&cubeBlas, &quadBlas, &quadBlas};

	update_transforms();

	kope_g5_device_create_raytracing_hierarchy(&device, volumes, transforms, 3, &hierarchy);

	{
		rayset_parameters parameters = {0};
		parameters.scene = &hierarchy;
		parameters.render_target.texture = &texture;
		parameters.render_target.base_mip_level = 0;
		parameters.render_target.mip_level_count = 1;
		parameters.render_target.base_array_layer = 0;
		parameters.render_target.array_layer_count = 1;
		kong_create_rayset_set(&device, &parameters, &rayset);
	}

	kinc_start();

	return 0;
}
