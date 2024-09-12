#include <kinc/io/filereader.h>
#include <kinc/math/matrix.h>
#include <kinc/system.h>

#include <kong.h>

#include <assert.h>

static kope_g5_device device;
static kope_g5_command_list list;
static vertex_in_buffer vertices;
static kope_g5_buffer indices;
static kope_g5_buffer constants;
static kope_g5_texture texture;
static kope_g5_sampler sampler;
static everything_set everything;
static kope_g5_buffer image_buffer;

#define WIDTH 1024
#define HEIGHT 768

void update(void *data) {
	/*kinc_g4_begin(0);
	kinc_g4_clear(KINC_G4_CLEAR_COLOR | KINC_G4_CLEAR_DEPTH, 0, 0.0f, 0);

	kinc_compute_set_shader(&computeShader);
	kinc_compute_set_texture(computeTexunit, &texture, KINC_COMPUTE_ACCESS_READ_WRITE);
	kinc_compute_set_float(computeLocation, 0);
	kinc_compute(texture.tex_width / 16, texture.tex_height / 16, 1);

	kinc_g4_set_pipeline(&pipeline);
	kinc_matrix3x3_t matrix = kinc_matrix3x3_rotation_z(0);
	kinc_g4_set_matrix3(offset, &matrix);
	kinc_g4_set_vertex_buffer(&vertices);
	kinc_g4_set_index_buffer(&indices);
	kinc_g4_set_texture(texunit, &texture);
	kinc_g4_draw_indexed_vertices();

	kinc_g4_end(0);
	kinc_g4_swap_buffers();*/
}

int kickstart(int argc, char **argv) {
	/*kinc_init("Compute", WIDTH, HEIGHT, NULL, NULL);
	kinc_set_update_callback(update, NULL);

	kinc_g4_texture_init(&texture, 256, 256, KINC_IMAGE_FORMAT_RGBA128);

	void *shaderSource = malloc(MAX_SHADER_SIZE);

	{
	    kinc_file_reader_t cs;
	    kinc_file_reader_open(&cs, "test.comp", KINC_FILE_TYPE_ASSET);
	    assert(kinc_file_reader_size(&cs) <= MAX_SHADER_SIZE);
	    kinc_file_reader_read(&cs, shaderSource, kinc_file_reader_size(&cs));
	    kinc_compute_shader_init(&computeShader, shaderSource, (int)kinc_file_reader_size(&cs));
	    kinc_file_reader_close(&cs);
	}

	computeTexunit = kinc_compute_shader_get_texture_unit(&computeShader, "destTex");
	computeLocation = kinc_compute_shader_get_constant_location(&computeShader, "roll");

	{
	    kinc_file_reader_t vs;
	    kinc_file_reader_open(&vs, "shader.vert", KINC_FILE_TYPE_ASSET);
	    assert(kinc_file_reader_size(&vs) <= MAX_SHADER_SIZE);
	    kinc_file_reader_read(&vs, shaderSource, kinc_file_reader_size(&vs));
	    kinc_g4_shader_init(&vertexShader, shaderSource, kinc_file_reader_size(&vs), KINC_G4_SHADER_TYPE_VERTEX);
	    kinc_file_reader_close(&vs);
	}

	{
	    kinc_file_reader_t fs;
	    kinc_file_reader_open(&fs, "shader.frag", KINC_FILE_TYPE_ASSET);
	    assert(kinc_file_reader_size(&fs) <= MAX_SHADER_SIZE);
	    kinc_file_reader_read(&fs, shaderSource, kinc_file_reader_size(&fs));
	    kinc_g4_shader_init(&fragmentShader, shaderSource, kinc_file_reader_size(&fs), KINC_G4_SHADER_TYPE_FRAGMENT);
	    kinc_file_reader_close(&fs);
	}

	kinc_g4_vertex_structure_t structure;
	kinc_g4_vertex_structure_init(&structure);
	kinc_g4_vertex_structure_add(&structure, "pos", KINC_G4_VERTEX_DATA_FLOAT3);
	kinc_g4_vertex_structure_add(&structure, "tex", KINC_G4_VERTEX_DATA_FLOAT2);
	kinc_g4_pipeline_init(&pipeline);
	pipeline.input_layout[0] = &structure;
	pipeline.input_layout[1] = NULL;
	pipeline.vertex_shader = &vertexShader;
	pipeline.fragment_shader = &fragmentShader;
	kinc_g4_pipeline_compile(&pipeline);

	texunit = kinc_g4_pipeline_get_texture_unit(&pipeline, "texsampler");
	offset = kinc_g4_pipeline_get_constant_location(&pipeline, "mvp");

	kinc_g4_vertex_buffer_init(&vertices, 3, &structure, KINC_G4_USAGE_STATIC, 0);
	float *v = kinc_g4_vertex_buffer_lock_all(&vertices);
	v[0] = -1.0f;
	v[1] = -1.0f;
	v[2] = 0.5f;
	v[3] = 0.0f;
	v[4] = 1.0f;
	v[5] = 1.0f;
	v[6] = -1.0f;
	v[7] = 0.5f;
	v[8] = 1.0f;
	v[9] = 1.0f;
	v[10] = -1.0f;
	v[11] = 1.0f;
	v[12] = 0.5f;
	v[13] = 0.0f;
	v[14] = 0.0f;
	kinc_g4_vertex_buffer_unlock_all(&vertices);

	kinc_g4_index_buffer_init(&indices, 3, KINC_G4_INDEX_BUFFER_FORMAT_32BIT, KINC_G4_USAGE_STATIC);
	uint32_t *i = (uint32_t *)kinc_g4_index_buffer_lock_all(&indices);
	i[0] = 0;
	i[1] = 1;
	i[2] = 2;
	kinc_g4_index_buffer_unlock_all(&indices);

	kinc_start();*/

	return 0;
}
