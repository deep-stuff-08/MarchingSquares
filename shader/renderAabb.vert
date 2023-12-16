#version 460 core

layout(location = 0)uniform vec4 spherePos;
layout(location = 1)uniform mat4 mvpMat;

void main(void) {
	vec2 offset[4] = {
		vec2(1, 1),
		vec2(-1, 1),
		vec2(-1, -1),
		vec2(1, -1)
	};

	gl_Position = mvpMat * vec4(spherePos.xy + offset[gl_VertexID] * spherePos.w, 0.0, 1.0);
}