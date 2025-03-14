#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <kore3/util/align.h>

static kore_gpu_buffer screenshot_buffer;

static void screenshot_take(kore_gpu_device *device, kore_gpu_command_list *list, kore_gpu_texture *framebuffer, int width, int height) {
	uint32_t row_bytes = kore_gpu_device_align_texture_row_bytes(device, width * 4);

	kore_gpu_buffer_parameters buffer_parameters = {
	    .size        = row_bytes * height,
	    .usage_flags = KORE_GPU_BUFFER_USAGE_CPU_READ,
	};
	kore_gpu_device_create_buffer(device, &buffer_parameters, &screenshot_buffer);

	kore_gpu_image_copy_texture source = {
	    .texture   = framebuffer,
	    .mip_level = 0,
	};

	kore_gpu_image_copy_buffer destination = {
	    .buffer        = &screenshot_buffer,
	    .bytes_per_row = row_bytes,
	};

	kore_gpu_command_list_copy_texture_to_buffer(list, &source, &destination, width, height, 1);

	kore_gpu_device_execute_command_list(device, list);

	kore_gpu_device_wait_until_idle(device);

	uint8_t *pixels = (uint8_t *)kore_gpu_buffer_lock_all(&screenshot_buffer);

	stbi_write_png("test.png", width, height, 4, pixels, row_bytes);

	kore_gpu_buffer_unlock(&screenshot_buffer);

	exit(0);
}
