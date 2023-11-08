#version 450

#extension GL_EXT_debug_printf : enable

layout(local_size_x = 1, local_size_y = 1) in;

layout(std430, binding = 0) buffer lay0 { int inbuf[]; };
layout(std430, binding = 1) buffer lay1 { uint outbuf[]; };

void main() {
	// drop threads outside the buffer dimensions.
	/* if(params.Width <= gl_GlobalInvocationID.x || params.Height <= gl_GlobalInvocationID.y){ */
	/* 	return; */
	/* } */

    const uint x = gl_GlobalInvocationID.x;
    const uint y = gl_GlobalInvocationID.y;

    // a - r - g - b
    uvec4 c = uvec4(255, 255, 0, 0);
	outbuf[y * 1920 + x] = (c.r << 24) | (c.g << 16) | (c.b << 8) | (c.a);
}
