#include <kinc/io/filereader.h>
#include <kinc/system.h>

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
static kope_g5_texture float_render_target;
static kope_g5_texture render_target; // intermediate target because D3D12 doesn't seem to like uav framebuffer access
static compute_set set;

const static int width = 800;
const static int height = 600;

static void update(void *data) {
	kope_g5_render_pass_parameters parameters = {0};
	parameters.color_attachments[0].load_op = KOPE_G5_LOAD_OP_CLEAR;
	kope_g5_color clear_color;
	clear_color.r = 0.0f;
	clear_color.g = 0.0f;
	clear_color.b = 0.0f;
	clear_color.a = 1.0f;
	parameters.color_attachments[0].clear_value = clear_color;
	parameters.color_attachments[0].texture.texture = &float_render_target;
	parameters.color_attachments[0].texture.array_layer_count = 1;
	parameters.color_attachments[0].texture.mip_level_count = 1;
	parameters.color_attachments[0].texture.format = KOPE_G5_TEXTURE_FORMAT_BGRA8_UNORM;
	parameters.color_attachments[0].texture.dimension = KOPE_G5_TEXTURE_VIEW_DIMENSION_2D;
	parameters.color_attachments_count = 1;
	kope_g5_command_list_begin_render_pass(&list, &parameters);

	kong_set_render_pipeline(&list, &pipeline);

	kong_set_vertex_buffer_vertex_in(&list, &vertices);

	kope_g5_command_list_set_index_buffer(&list, &indices, KOPE_G5_INDEX_FORMAT_UINT16, 0, 3 * sizeof(uint16_t));

	kope_g5_command_list_draw_indexed(&list, 3, 1, 0, 0, 0);

	kope_g5_command_list_end_render_pass(&list);

	kope_g5_texture *framebuffer = kope_g5_device_get_framebuffer(&device);

	kong_set_compute_pipeline(&list, &comp);

	kong_set_descriptor_set_compute(&list, &set);

	copy_constants_type copy_constants;
	copy_constants.size.x = width;
	copy_constants.size.y = height;
	kong_set_root_constants_copy_constants(&list, &copy_constants);

	kope_g5_command_list_compute(&list, (width + 7) / 8, (height + 7) / 8, 1);

	kope_g5_image_copy_texture source = {0};
	source.texture = &render_target;

	kope_g5_image_copy_texture destination = {0};
	destination.texture = framebuffer;

	kope_g5_command_list_copy_texture_to_texture(&list, &source, &destination, width, height, 1);

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
		kope_g5_texture_parameters texture_parameters;
		texture_parameters.width = width;
		texture_parameters.height = height;
		texture_parameters.depth_or_array_layers = 1;
		texture_parameters.mip_level_count = 1;
		texture_parameters.sample_count = 1;
		texture_parameters.dimension = KOPE_G5_TEXTURE_DIMENSION_2D;
		texture_parameters.format = KOPE_G5_TEXTURE_FORMAT_RGBA32_FLOAT;
		texture_parameters.usage = KONG_G5_TEXTURE_USAGE_RENDER_ATTACHMENT;
		kope_g5_device_create_texture(&device, &texture_parameters, &float_render_target);
	}

	{
		kope_g5_texture_parameters texture_parameters;
		texture_parameters.width = width;
		texture_parameters.height = height;
		texture_parameters.depth_or_array_layers = 1;
		texture_parameters.mip_level_count = 1;
		texture_parameters.sample_count = 1;
		texture_parameters.dimension = KOPE_G5_TEXTURE_DIMENSION_2D;
		texture_parameters.format = KOPE_G5_TEXTURE_FORMAT_RGBA8_UNORM;
		texture_parameters.usage = KONG_G5_TEXTURE_USAGE_READ_WRITE;
		kope_g5_device_create_texture(&device, &texture_parameters, &render_target);
	}

	kong_create_buffer_vertex_in(&device, 3, &vertices);
	{
		vertex_in *v = kong_vertex_in_buffer_lock(&vertices);

		v[0].pos.x = -1.0;
		v[0].pos.y = -1.0;
		v[0].pos.z = 0.0;

		v[1].pos.x = 1.0;
		v[1].pos.y = -1.0;
		v[1].pos.z = 0.0;

		v[2].pos.x = 0.0;
		v[2].pos.y = 1.0;
		v[2].pos.z = 0.0;

		kong_vertex_in_buffer_unlock(&vertices);
	}

	kope_g5_buffer_parameters params;
	params.size = 3 * sizeof(uint16_t);
	params.usage_flags = KOPE_G5_BUFFER_USAGE_INDEX | KOPE_G5_BUFFER_USAGE_CPU_WRITE;
	kope_g5_device_create_buffer(&device, &params, &indices);
	{
		uint16_t *i = (uint16_t *)kope_g5_buffer_lock_all(&indices);
		i[0] = 0;
		i[1] = 1;
		i[2] = 2;
		kope_g5_buffer_unlock(&indices);
	}

	compute_parameters cparams = {0};
	cparams.copy_source_texture.texture = &float_render_target;
	cparams.copy_source_texture.base_mip_level = 0;
	cparams.copy_source_texture.mip_level_count = 1;
	cparams.copy_source_texture.base_array_layer = 0;
	cparams.copy_source_texture.array_layer_count = 1;
	cparams.copy_destination_texture.texture = &render_target;
	cparams.copy_destination_texture.base_mip_level = 0;
	cparams.copy_destination_texture.mip_level_count = 1;
	cparams.copy_destination_texture.base_array_layer = 0;
	cparams.copy_destination_texture.array_layer_count = 1;
	kong_create_compute_set(&device, &cparams, &set);

	kinc_start();

	return 0;
}
