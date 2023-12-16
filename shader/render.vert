#version 460 core

out V2G {
	ivec2 cellTypeCoords;
} vs_out;

layout(location = 0)uniform ivec2 cellCount;

void main() {
	int id = gl_VertexID;
	vs_out.cellTypeCoords = ivec2(int(id % cellCount.x), int(id / cellCount.x));
}