#include <kinc/graphics1/graphics.h>
#include <kinc/graphics4/graphics.h>
#include <kinc/graphics4/indexbuffer.h>
#include <kinc/graphics4/pipeline.h>
#include <kinc/graphics4/rendertarget.h>
#include <kinc/graphics4/shader.h>
#include <kinc/graphics4/vertexbuffer.h>
#include <kinc/io/filereader.h>
#include <kinc/system.h>

#include <kong.h>

#include <assert.h>
#include <stdlib.h>

static kope_g5_device device;
static kope_g5_command_list list;
static vertex_in_buffer vertices;
static kope_g5_buffer indices;
static kope_g5_texture render_targets[4];

static void update(void *data) {
	kope_g5_render_pass_parameters parameters = {0};
	for (uint32_t i = 0; i < 4; ++i) {
		parameters.color_attachments[i].load_op = KOPE_G5_LOAD_OP_CLEAR;
		kope_g5_color clear_color;
		clear_color.r = 0.0f;
		clear_color.g = 0.0f;
		clear_color.b = 0.0f;
		clear_color.a = 1.0f;
		parameters.color_attachments[i].clear_value = clear_color;
		parameters.color_attachments[i].texture = &render_targets[i];
	}
	parameters.color_attachments_count = 4;
	kope_g5_command_list_begin_render_pass(&list, &parameters);

	kong_set_render_pipeline(&list, &pipeline);

	kong_set_vertex_buffer_vertex_in(&list, &vertices);

	kope_g5_command_list_set_index_buffer(&list, &indices, KOPE_G5_INDEX_FORMAT_UINT16, 0, 3 * sizeof(uint16_t));

	kope_g5_command_list_draw_indexed(&list, 3, 1, 0, 0, 0);

	kope_g5_command_list_end_render_pass(&list);

	kope_g5_texture *framebuffer = kope_g5_device_get_framebuffer(&device);

	kope_g5_image_copy_texture destination = {0};
	destination.texture = framebuffer;
	destination.aspect = KOPE_G5_IMAGE_COPY_ASPECT_ALL;
	destination.mip_level = 0;
	destination.origin_x = 0;
	destination.origin_y = 0;
	destination.origin_z = 0;

	kope_g5_image_copy_texture source = {0};
	source.texture = &render_targets[0];
	source.aspect = KOPE_G5_IMAGE_COPY_ASPECT_ALL;
	source.mip_level = 0;
	source.origin_x = 0;
	source.origin_y = 0;
	source.origin_z = 0;

	kope_g5_command_list_copy_texture_to_texture(&list, &source, &destination, 512, 384, 1);

	destination.origin_x = 512;
	destination.origin_y = 0;
	source.texture = &render_targets[1];

	kope_g5_command_list_copy_texture_to_texture(&list, &source, &destination, 512, 384, 1);

	destination.origin_x = 0;
	destination.origin_y = 384;
	source.texture = &render_targets[2];

	kope_g5_command_list_copy_texture_to_texture(&list, &source, &destination, 512, 384, 1);

	destination.origin_x = 512;
	destination.origin_y = 384;
	source.texture = &render_targets[3];

	kope_g5_command_list_copy_texture_to_texture(&list, &source, &destination, 512, 384, 1);

	kope_g5_command_list_present(&list);

	kope_g5_device_execute_command_list(&device, &list);
}

int kickstart(int argc, char **argv) {
	kinc_init("Example", 1024, 768, NULL, NULL);
	kinc_set_update_callback(update, NULL);

	kope_g5_device_wishlist wishlist = {0};
	kope_g5_device_create(&device, &wishlist);

	kong_init(&device);

	kope_g5_device_create_command_list(&device, &list);

	for (uint32_t i = 0; i < 4; ++i) {
		kope_g5_texture_parameters texture_parameters;
		texture_parameters.width = 512;
		texture_parameters.height = 384;
		texture_parameters.depth_or_array_layers = 1;
		texture_parameters.mip_level_count = 1;
		texture_parameters.sample_count = 1;
		texture_parameters.dimension = KOPE_G5_TEXTURE_DIMENSION_2D;
		texture_parameters.format = KOPE_G5_TEXTURE_FORMAT_RGBA8_UNORM;
		texture_parameters.usage = KONG_G5_TEXTURE_USAGE_RENDER_ATTACHMENT | KONG_G5_TEXTURE_USAGE_COPY_SRC;
		kope_g5_device_create_texture(&device, &texture_parameters, &render_targets[i]);
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
		uint16_t *i = (uint16_t *)kope_g5_buffer_lock(&indices);
		i[0] = 0;
		i[1] = 1;
		i[2] = 2;
		kope_g5_buffer_unlock(&indices);
	}

	kinc_start();

	return 0;
}
