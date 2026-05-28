#version 330 core
uniform sampler2D tex_y;
uniform sampler2D tex_u;
uniform sampler2D tex_v;
in vec2 textureOut;
out vec4 fragColor;

void main() {
    float y = texture(tex_y, textureOut).r;
    float u = texture(tex_u, textureOut).r - 0.5;
    float v = texture(tex_v, textureOut).r - 0.5;
    vec3 rgb;
    rgb.r = y + 1.402 * v;
    rgb.g = y - 0.344 * u - 0.714 * v;
    rgb.b = y + 1.772 * u;
    fragColor = vec4(rgb, 1.0);
}