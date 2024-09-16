#include <kinc/io/filereader.h>
#include <kinc/math/matrix.h>
#include <kinc/system.h>

#include <kong.h>

#include <assert.h>
#include <string.h>

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

#define WIDTH 1024
#define HEIGHT 768

static bool first = true;

void update(void *data) {
	kope_g5_texture *framebuffer = kope_g5_device_get_framebuffer(&device);

	if (first) {
		first = false;

		kope_g5_command_list_prepare_raytracing_volume(&list, &cubeBlas);
		kope_g5_command_list_prepare_raytracing_volume(&list, &quadBlas);
		kope_g5_command_list_prepare_raytracing_hierarchy(&list, &hierarchy);
	}

	kong_set_ray_pipeline(&list, &ray_pipe);

	kong_set_descriptor_set_rayset(&list, &rayset);

	kope_g5_command_list_trace_rays(&list);

	kope_uint3 size;
	size.x = 1024;
	size.y = 768;
	size.z = 1;
	kope_g5_command_list_copy_texture_to_texture(&list, &texture, framebuffer, size);

	kope_g5_command_list_present(&list);

	kope_g5_device_execute_command_list(&device, &list);
}

int kickstart(int argc, char **argv) {
	kinc_init("Raytracing", 1024, 768, NULL, NULL);
	kinc_set_update_callback(update, NULL);

	kope_g5_device_wishlist wishlist = {0};
	kope_g5_device_create(&device, &wishlist);

	kong_init(&device);

	kope_g5_texture_parameters render_target_parameters;
	render_target_parameters.width = 1024;
	render_target_parameters.height = 768;
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

	{
		rayset_parameters parameters;
		parameters.scene = &hierarchy.d3d12.acceleration_structure;
		parameters.render_target = &texture;
		kong_create_rayset_set(&device, &parameters, &rayset);
	}

	kinc_start();

	return 0;
}
