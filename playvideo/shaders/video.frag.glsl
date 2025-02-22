#version 450

in vec2 texcoord;

out vec4 FragColor;

uniform samplerVideo video_sampler;

void main() {
	vec4 color = texture(video_sampler, texcoord);
	FragColor = vec4(color.r, color.g, color.b, 1.0);
}
