#version 450

#extension GL_EXT_debug_printf : enable

layout(local_size_x = 1, local_size_y = 1) in;

layout(std430, binding = 0) buffer lay0 { uint inbuf[]; };
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


float c_x0 = -1.0;
float c_x1 =  0.0;
float c_x2 =  1.0;
float c_x3 =  2.0;

vec4 cubic_lagrange(vec4 A, vec4 B, vec4 C, vec4 D, float t) {
    return
        A * 
        (
            (t - c_x1) / (c_x0 - c_x1) * 
            (t - c_x2) / (c_x0 - c_x2) *
            (t - c_x3) / (c_x0 - c_x3)
        ) +
        B * 
        (
            (t - c_x0) / (c_x1 - c_x0) * 
            (t - c_x2) / (c_x1 - c_x2) *
            (t - c_x3) / (c_x1 - c_x3)
        ) +
        C * 
        (
            (t - c_x0) / (c_x2 - c_x0) * 
            (t - c_x1) / (c_x2 - c_x1) *
            (t - c_x3) / (c_x2 - c_x3)
        ) +       
        D * 
        (
            (t - c_x0) / (c_x3 - c_x0) * 
            (t - c_x1) / (c_x3 - c_x1) *
            (t - c_x2) / (c_x3 - c_x2)
        );
}

vec4 cubic_hermite(vec4 A, vec4 B, vec4 C, vec4 D, float t) {
	float t2 = t*t;
    float t3 = t*t*t;

    vec4 a = -A/2 + (3*B)/2 - (3*C)/2 + D/2;
    vec4 b = A - (5*B)/2 + 2*C - D / 2;
    vec4 c = -A/2 + C/2;
   	vec4 d = B;

    return a*t3 + b*t2 + c*t + d;
}

vec4 get_color(vec2 pixel, int x, int y) {
    int idx = int((pixel.y + y) * scale.iw + (pixel.x + x));
    uint color = inbuf[idx];
    vec4 c = uvec4(
        (color & uint(0xFF000000)) >> 24,
        (color & uint(0x00FF0000)) >> 16,
        (color & uint(0x0000FF00)) >> 8,
        (color & uint(0x000000FF))
    );
    return c;
}

void main() {
    const uint x = gl_GlobalInvocationID.x;
    const uint y = gl_GlobalInvocationID.y;
    const uint odx = y * 1920 + x;

    if (x >= (scale.imaxw) || y >= (scale.imaxh)) return;

    // vec2 uv = gl_GlobalInvocationID.xy / vec2(1920, 1080);
    // P is the UV cord so 0 - 1
    // const uint c_textureSize = 64;
    // vec2 pixel = uv * 1920 + 0.5;
    vec2 pixel = (gl_GlobalInvocationID.xy / scale.r);
    vec2 frac = fract(pixel);
    pixel = floor(pixel);
    // debugPrintfEXT("%f ", pixel.x);

    vec4 C00 = get_color(pixel, -1, -1); // vec2(-c_onePixel ,-c_onePixel))
    vec4 C10 = get_color(pixel,  0, -1); // vec2( 0.0        ,-c_onePixel))
    vec4 C20 = get_color(pixel,  1, -1); // vec2( c_onePixel ,-c_onePixel))
    vec4 C30 = get_color(pixel,  2, -1); // vec2( c_twoPixels,-c_onePixel))

    vec4 C01 = get_color(pixel, -1, 0); // vec2(-c_onePixel , 0.0))
    vec4 C11 = get_color(pixel,  0, 0); // vec2( 0.0        , 0.0))
    vec4 C21 = get_color(pixel,  1, 0); // vec2( c_onePixel , 0.0))
    vec4 C31 = get_color(pixel,  2, 0); // vec2( c_twoPixels, 0.0))

    vec4 C02 = get_color(pixel, -1, 1); // vec2(-c_onePixel , c_onePixel))
    vec4 C12 = get_color(pixel,  0, 1); // vec2( 0.0        , c_onePixel))
    vec4 C22 = get_color(pixel,  1, 1); // vec2( c_onePixel , c_onePixel))
    vec4 C32 = get_color(pixel,  2, 1); // vec2( c_twoPixels, c_onePixel))

    vec4 C03 = get_color(pixel, -1, 2); // vec2(-c_onePixel , c_twoPixels))
    vec4 C13 = get_color(pixel,  0, 2); // vec2( 0.0        , c_twoPixels))
    vec4 C23 = get_color(pixel,  1, 2); // vec2( c_onePixel , c_twoPixels))
    vec4 C33 = get_color(pixel,  2, 2); // vec2( c_twoPixels, c_twoPixels))

    vec4 CP0X = cubic_hermite(C00, C10, C20, C30, frac.x);
    vec4 CP1X = cubic_hermite(C01, C11, C21, C31, frac.x);
    vec4 CP2X = cubic_hermite(C02, C12, C22, C32, frac.x);
    vec4 CP3X = cubic_hermite(C03, C13, C23, C33, frac.x);

    // vec4 CP0X = cubic_lagrange(C00, C10, C20, C30, frac.x);
    // vec4 CP1X = cubic_lagrange(C01, C11, C21, C31, frac.x);
    // vec4 CP2X = cubic_lagrange(C02, C12, C22, C32, frac.x);
    // vec4 CP3X = cubic_lagrange(C03, C13, C23, C33, frac.x);

    // uvec4 c = uvec4(cubic_lagrange(CP0X, CP1X, CP2X, CP3X, frac.y));
    uvec4 c = uvec4(cubic_hermite(CP0X, CP1X, CP2X, CP3X, frac.y));
    
    // uvec4 c = uvec4(
    //     (color & uint(0xFF000000)) >> 24,
    //     (color & uint(0x00FF0000)) >> 16,
    //     (color & uint(0x0000FF00)) >> 8,
    //     (color & uint(0x000000FF))
    // );

	outbuf[odx] = (c.x << 24) | (c.w << 16) | (c.z << 8) | (c.y);
}
