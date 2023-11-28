#version 450

layout(local_size_x = 1, local_size_y = 1) in;

layout(std430, binding = 0) buffer lay0 { int inbuf[]; };
layout(std430, binding = 1) buffer lay1 { uint outbuf[]; };

layout(binding = 2) uniform uniform_data {
    float zoom;
    uint iw;
    uint ih;
    uint ow;
    uint oh;
    float wr;
    float hr;
    float r;
    uint imaxw;
    uint imaxh;
    uint offset_x;
    uint offset_y;
} scale;

void main() {
    const uint x = gl_GlobalInvocationID.x;
    const uint y = gl_GlobalInvocationID.y;

    if (x >= (scale.imaxw) || y >= (scale.imaxh)) return;

    const uint row = uint(y / scale.r);
    const uint col = uint(x / scale.r);
    const uint idx = row * scale.iw + col;
    const uint odx = y * 1920 + x;

    const uint color = inbuf[row * scale.iw + col];
    uvec4 c = uvec4(
        (color & uint(0xFF000000)) >> 24,
        (color & uint(0x00FF0000)) >> 16,
        (color & uint(0x0000FF00)) >> 8,
        (color & uint(0x000000FF))
    );

	outbuf[odx] = (c.x << 24) | (c.w << 16) | (c.z << 8) | (c.y);
}
