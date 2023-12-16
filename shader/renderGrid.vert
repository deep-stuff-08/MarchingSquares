#version 460 core

layout(binding = 0, r32f)readonly restrict uniform image2D scalarDataField;

layout(location = 0)uniform ivec2 gridDim;
layout(location = 1)uniform mat4 mvpMat;
layout(location = 2)uniform bool isLinear;
layout(location = 3)uniform float isolevel;

out vec3 vs_color;

void main(void) {
	ivec2 coords = ivec2(int(gl_VertexID % gridDim.x), int(gl_VertexID / gridDim.x));
	float intensity = imageLoad(scalarDataField, coords).r;
	vs_color = isLinear ? (intensity > isolevel ? vec3(1.0) : vec3(0.0)) : vec3(1.0) * intensity;
	gl_Position = mvpMat * vec4((vec2(coords) / vec2(gridDim-1)) * 2.0 - 1.0, 0.0, 1.0);
}