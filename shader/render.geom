#version 460 core

layout (points) in;
layout (line_strip, max_vertices = 4) out;

in V2G {
	ivec2 cellTypeCoords;
} gs_in[];

layout(location = 0)uniform ivec2 cellCount;
layout(location = 1)uniform mat4 mvpMat;
layout(location = 2)uniform float isolevel;
layout(location = 3)uniform bool isLerp;
layout(binding = 0, r32f) uniform restrict image2D scalarDataFieldTex;
layout(binding = 1, r8ui) uniform restrict uimage2D cellTypeTex;

void outputLine(vec2 P1, vec2 P2) {
	gl_Position = mvpMat * vec4(P1, 0.0, 1.0);
	EmitVertex();
	gl_Position = mvpMat * vec4(P2, 0.0, 1.0);
	EmitVertex();
	EndPrimitive();
}

float getLerp(float S1, float S2) {
	if(isLerp) {
		float delta = abs(S1 - S2);
		if(delta > 0.0001) {
			return abs(S2 - isolevel) / delta;
			//return 0.5;
		} else {
			return 0.5;
		}
	} else {
		return 0.5;
	}
}

void main(void) {
	ivec2 C1 = (gs_in[0].cellTypeCoords + ivec2(0, 0));
	ivec2 C2 = (gs_in[0].cellTypeCoords + ivec2(0, 1));
	ivec2 C3 = (gs_in[0].cellTypeCoords + ivec2(1, 0));
	ivec2 C4 = (gs_in[0].cellTypeCoords + ivec2(1, 1));

	float S1 = imageLoad(scalarDataFieldTex, C1).r;
	float S2 = imageLoad(scalarDataFieldTex, C2).r;
	float S3 = imageLoad(scalarDataFieldTex, C3).r;
	float S4 = imageLoad(scalarDataFieldTex, C4).r;

	vec2 P1 = C1 / vec2(cellCount) * 2.0 - 1.0;
	vec2 P2 = C2 / vec2(cellCount) * 2.0 - 1.0;
	vec2 P3 = C3 / vec2(cellCount) * 2.0 - 1.0;
	vec2 P4 = C4 / vec2(cellCount) * 2.0 - 1.0;

	float L1, L2;

	uint cellType = imageLoad(cellTypeTex, gs_in[0].cellTypeCoords).r;
	switch(cellType) {
	case 0: case 15: default:
		// outputLine(P1, P2);
		// outputLine(P2, P4);
		break;
	case 1: case 14:
		L1 = getLerp(S1, S2);
		L2 = getLerp(S1, S3);
		outputLine(mix(P2, P1, L1), mix(P3, P1, L2));
		break;
	case 2: case 13:
		L1 = getLerp(S1, S3);
		L2 = getLerp(S3, S4);
		outputLine(mix(P3, P1, L1), mix(P4, P3, L2));
		break;
	case 3: case 12:
		L1 = getLerp(S1, S2);
		L2 = getLerp(S3, S4);
		outputLine(mix(P2, P1, L1), mix(P4, P3, L2));
		break;
	case 4: case 11:
		L1 = getLerp(S1, S2);
		L2 = getLerp(S2, S4);
		outputLine(mix(P2, P1, L1), mix(P4, P2, L2));
		break;
	case 5: case 10:
		L1 = getLerp(S1, S3);
		L2 = getLerp(S2, S4);
		outputLine(mix(P3, P1, L1), mix(P4, P2, L2));
		break;
	case 6: 
		L1 = getLerp(S1, S3);
		L2 = getLerp(S3, S4);
		outputLine(mix(P3, P1, L1), mix(P4, P3, L2));
		L1 = getLerp(S1, S2);
		L2 = getLerp(S2, S4);
		outputLine(mix(P2, P1, L1), mix(P4, P2, L2));
		break;
	case 9:
		L1 = getLerp(S1, S2);
		L2 = getLerp(S1, S3);
		outputLine(mix(P2, P1, L1), mix(P3, P1, L2));
		L1 = getLerp(S2, S4);
		L2 = getLerp(S3, S4);
		outputLine(mix(P4, P2, L1), mix(P4, P3, L2));
		break;
	case 7: case 8:
		L1 = getLerp(S3, S4);
		L2 = getLerp(S2, S4);
		outputLine(mix(P4, P3, L1), mix(P4, P2, L2));
		break;
	}
}