#version 330
// DirectWrite rasterization example: vertex shader
// This file is only included for reference, GLSL code is stuffed into the rasterizer inline.

uniform mat3 pixel_to_normal;
in vec2 position;
in vec3 tex_position;
smooth out vec3 uv;
void main(){
    gl_Position.xy = (pixel_to_normal*vec3(position, 1.f)).xy;
    gl_Position.z = 0.f;
    gl_Position.w = 1.f;
    uv = tex_position;
}

