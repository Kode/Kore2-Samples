#include <kore3/io/filereader.h>
#include <kore3/system.h>

#include <kong.h>

#include <assert.h>
#include <stdlib.h>

#ifdef SCREENSHOT
#include "../../screenshot.h"
#endif

static kore_gpu_device       device;
static kore_gpu_command_list list;
static vertex_in_buffer      vertices;
static kore_gpu_buffer       indices;
static kore_gpu_texture      render_target;

static const uint32_t width  = 800;
static const uint32_t height = 600;

static void update(void *data) {
	kore_gpu_render_pass_parameters parameters = {
	    .color_attachments_count = 1,
	    .color_attachments =
	        {
	            {
	                .load_op = KORE_GPU_LOAD_OP_CLEAR,
	                .clear_value =
	                    {
	                        .r = 0.0f,
	                        .g = 0.0f,
	                        .b = 0.0f,
	                        .a = 1.0f,
	                    },
	                .texture =
	                    {
	                        .texture           = &render_target,
	                        .array_layer_count = 1,
	                        .mip_level_count   = 1,
	                        .format            = KORE_GPU_TEXTURE_FORMAT_BGRA8_UNORM,
	                        .dimension         = KORE_GPU_TEXTURE_VIEW_DIMENSION_2D,
	                    },
	            },
	        },
	};
	kore_gpu_command_list_begin_render_pass(&list, &parameters);

	kong_set_render_pipeline_pipeline(&list);

	kong_set_vertex_buffer_vertex_in(&list, &vertices);

	kore_gpu_command_list_set_index_buffer(&list, &indices, KORE_GPU_INDEX_FORMAT_UINT16, 0, 3 * sizeof(uint16_t));

	kore_gpu_command_list_draw_indexed(&list, 3, 1, 0, 0, 0);

	kore_gpu_command_list_end_render_pass(&list);

	kore_gpu_texture *framebuffer = kore_gpu_device_get_framebuffer(&device);

	kore_gpu_image_copy_texture source = {
	    .texture = &render_target,
	};

	kore_gpu_image_copy_texture destination = {
	    .texture = framebuffer,
	};

	kore_gpu_command_list_copy_texture_to_texture(&list, &source, &destination, width, height, 1);

#ifdef SCREENSHOT
	screenshot_take(&device, &list, framebuffer, width, height);
#endif

	kore_gpu_command_list_present(&list);

	kore_gpu_device_execute_command_list(&device, &list);
}

int kickstart(int argc, char **argv) {
	kore_init("06_render_targets", width, height, NULL, NULL);
	kore_set_update_callback(update, NULL);

	kore_gpu_device_wishlist wishlist = {0};
	kore_gpu_device_create(&device, &wishlist);

	kong_init(&device);

	kore_gpu_device_create_command_list(&device, KORE_GPU_COMMAND_LIST_TYPE_GRAPHICS, &list);

	kore_gpu_texture_parameters texture_parameters = {
	    .width                 = width,
	    .height                = height,
	    .depth_or_array_layers = 1,
	    .mip_level_count       = 1,
	    .sample_count          = 1,
	    .dimension             = KORE_GPU_TEXTURE_DIMENSION_2D,
	    .format                = KORE_GPU_TEXTURE_FORMAT_RGBA8_UNORM,
	    .usage                 = KONG_G5_TEXTURE_USAGE_RENDER_ATTACHMENT | KONG_G5_TEXTURE_USAGE_COPY_SRC,
	};
	kore_gpu_device_create_texture(&device, &texture_parameters, &render_target);

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

	kore_gpu_buffer_parameters params;
	params.size        = 3 * sizeof(uint16_t);
	params.usage_flags = KORE_GPU_BUFFER_USAGE_INDEX | KORE_GPU_BUFFER_USAGE_CPU_WRITE;
	kore_gpu_device_create_buffer(&device, &params, &indices);
	{
		uint16_t *i = (uint16_t *)kore_gpu_buffer_lock_all(&indices);
		i[0]        = 0;
		i[1]        = 1;
		i[2]        = 2;
		kore_gpu_buffer_unlock(&indices);
	}

	kore_start();

	return 0;
}
