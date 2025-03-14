#include <kinc/image.h>
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
static fs_vertex_in_buffer vertices_fs;
static kope_g5_buffer indices;
static kope_g5_texture texture;
static kope_g5_sampler sampler;
static fs_set set;
static kope_g5_buffer image_buffer0;
static kope_g5_buffer image_buffer1;

static const int width  = 800;
static const int height = 600;

static bool first = true;

static void update(void *data) {
	if (first) {
		{
			kope_g5_image_copy_buffer source = {0};
			source.bytes_per_row             = kope_g5_device_align_texture_row_bytes(&device, 512 * 4);
			source.buffer                    = &image_buffer0;

			kope_g5_image_copy_texture destination = {0};
			destination.mip_level                  = 0;
			destination.texture                    = &texture;

			kope_g5_command_list_copy_buffer_to_texture(&list, &source, &destination, 512, 512, 1);
		}

		{
			kope_g5_image_copy_buffer source = {0};
			source.bytes_per_row             = kope_g5_device_align_texture_row_bytes(&device, 256 * 4);
			source.buffer                    = &image_buffer1;

			kope_g5_image_copy_texture destination = {0};
			destination.mip_level                  = 1;
			destination.texture                    = &texture;

			kope_g5_command_list_copy_buffer_to_texture(&list, &source, &destination, 256, 256, 1);
		}

		first = false;
	}

	kope_g5_texture *framebuffer = kope_g5_device_get_framebuffer(&device);

	{
		kope_g5_render_pass_parameters parameters = {0};
		parameters.color_attachments[0].load_op   = KOPE_G5_LOAD_OP_CLEAR;
		kope_g5_color clear_color;
		clear_color.r                                             = 0.0f;
		clear_color.g                                             = 0.0f;
		clear_color.b                                             = 0.0f;
		clear_color.a                                             = 1.0f;
		parameters.color_attachments[0].clear_value               = clear_color;
		parameters.color_attachments[0].texture.texture           = framebuffer;
		parameters.color_attachments[0].texture.array_layer_count = 1;
		parameters.color_attachments[0].texture.mip_level_count   = 1;
		parameters.color_attachments[0].texture.format            = KOPE_G5_TEXTURE_FORMAT_BGRA8_UNORM;
		parameters.color_attachments[0].texture.dimension         = KOPE_G5_TEXTURE_VIEW_DIMENSION_2D;
		parameters.color_attachments_count                        = 1;
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

	kope_g5_device_create_command_list(&device, KOPE_G5_COMMAND_LIST_TYPE_GRAPHICS, &list);

	{
		kope_g5_buffer_parameters buffer_parameters;
		buffer_parameters.size        = kope_g5_device_align_texture_row_bytes(&device, 512 * 4) * 512;
		buffer_parameters.usage_flags = KOPE_G5_BUFFER_USAGE_CPU_WRITE;
		kope_g5_device_create_buffer(&device, &buffer_parameters, &image_buffer0);

		kinc_image_t image;
		kinc_image_init_from_file_with_stride(&image, kope_g5_buffer_lock_all(&image_buffer0), "uvtemplate.png",
		                                      kope_g5_device_align_texture_row_bytes(&device, 512 * 4));
		kinc_image_destroy(&image);
		kope_g5_buffer_unlock(&image_buffer0);
	}

	{
		kope_g5_buffer_parameters buffer_parameters;
		buffer_parameters.size        = kope_g5_device_align_texture_row_bytes(&device, 256 * 4) * 256;
		buffer_parameters.usage_flags = KOPE_G5_BUFFER_USAGE_CPU_WRITE;
		kope_g5_device_create_buffer(&device, &buffer_parameters, &image_buffer1);

		kinc_image_t image;
		kinc_image_init_from_file_with_stride(&image, kope_g5_buffer_lock_all(&image_buffer1), "uvtemplate2.png",
		                                      kope_g5_device_align_texture_row_bytes(&device, 256 * 4));
		kinc_image_destroy(&image);
		kope_g5_buffer_unlock(&image_buffer1);
	}

	{
		kope_g5_texture_parameters texture_parameters = {0};
		texture_parameters.width                      = 512;
		texture_parameters.height                     = 512;
		texture_parameters.depth_or_array_layers      = 1;
		texture_parameters.mip_level_count            = 2;
		texture_parameters.sample_count               = 1;
		texture_parameters.dimension                  = KOPE_G5_TEXTURE_DIMENSION_2D;
		texture_parameters.format                     = KOPE_G5_TEXTURE_FORMAT_RGBA8_UNORM;
		texture_parameters.usage                      = KONG_G5_TEXTURE_USAGE_SAMPLE | KONG_G5_TEXTURE_USAGE_COPY_DST;
		kope_g5_device_create_texture(&device, &texture_parameters, &texture);
	}

	kope_g5_sampler_parameters sampler_params = {0};
	sampler_params.lod_min_clamp              = 0;
	sampler_params.lod_max_clamp              = 1;
	kope_g5_device_create_sampler(&device, &sampler_params, &sampler);

	kong_create_buffer_fs_vertex_in(&device, 3, &vertices_fs);
	{
		fs_vertex_in *v = kong_fs_vertex_in_buffer_lock(&vertices_fs);

		v[0].pos.x = -1.0;
		v[0].pos.y = -1.0;

		v[1].pos.x = 3.0;
		v[1].pos.y = -1.0;

		v[2].pos.x = -1.0;
		v[2].pos.y = 3.0;

		kong_fs_vertex_in_buffer_unlock(&vertices_fs);
	}

	kope_g5_buffer_parameters params;
	params.size        = 3 * sizeof(uint16_t);
	params.usage_flags = KOPE_G5_BUFFER_USAGE_INDEX | KOPE_G5_BUFFER_USAGE_CPU_WRITE;
	kope_g5_device_create_buffer(&device, &params, &indices);
	{
		uint16_t *i = (uint16_t *)kope_g5_buffer_lock_all(&indices);
		i[0]        = 0;
		i[1]        = 1;
		i[2]        = 2;
		kope_g5_buffer_unlock(&indices);
	}

	fs_parameters fsparams                = {0};
	fsparams.fs_texture.texture           = &texture;
	fsparams.fs_texture.base_mip_level    = 0;
	fsparams.fs_texture.mip_level_count   = 2;
	fsparams.fs_texture.base_array_layer  = 0;
	fsparams.fs_texture.array_layer_count = 1;
	fsparams.fs_sampler                   = &sampler;
	kong_create_fs_set(&device, &fsparams, &set);

	kinc_start();

	return 0;
}
