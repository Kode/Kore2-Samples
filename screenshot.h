#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static kope_g5_buffer screenshot_buffer;

static void screenshot_init_buffer(kope_g5_device *device, int width, int height) {
	kope_g5_buffer_parameters buffer_parameters;
	buffer_parameters.size = width * 4 * height;
	buffer_parameters.usage_flags = KOPE_G5_BUFFER_USAGE_CPU_READ;
	kope_g5_device_create_buffer(device, &buffer_parameters, &screenshot_buffer);
}

static void screenshot_take(kope_g5_device *device, kope_g5_command_list *list, kope_g5_texture *framebuffer, int width, int height) {
	kope_g5_image_copy_texture source = {0};
	source.texture = framebuffer;
	source.mip_level = 0;

	kope_g5_image_copy_buffer destination = {0};
	destination.buffer = &screenshot_buffer;
	destination.bytes_per_row = width * 4;

	kope_g5_command_list_copy_texture_to_buffer(list, &source, &destination, width, height, 1);

	kope_g5_device_execute_command_list(device, list);

	kope_g5_device_wait_until_idle(device);

	uint8_t *pixels = (uint8_t *)kope_g5_buffer_lock(&screenshot_buffer);

	stbi_write_png("test.png", width, height, 4, pixels, width * 4);

	kope_g5_buffer_unlock(&screenshot_buffer);

	exit(0);
}
