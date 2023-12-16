#version 460 core

layout(location = 0)out vec4 FragColor;
layout(location = 2)uniform vec3 color;

void main(void) {
	FragColor = vec4(color, 1.0);
}