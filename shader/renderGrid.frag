#version 460 core

layout(location = 0)out vec4 FragColor;

in vec3 vs_color;

void main(void) {
	FragColor = vec4(vs_color, 1.0);
}