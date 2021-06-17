#version 330
// DirectWrite rasterization example: fragment shader
// This file is only included for reference, GLSL code is stuffed into the rasterizer inline.

smooth in vec3 uv;
uniform sampler2DArray tex;
uniform float M_value_table[7];
layout(location = 0) out vec4 mask;

void main(){
    vec3 S = texture(tex, uv).rgb;
    int C0 = int(S.r*6 + 0.1); // + 0.1 just incase we have some small rounding taking us below the integer we should be hitting.
    int C1 = int(S.g*6 + 0.1);
    int C2 = int(S.b*6 + 0.1);
    mask.rgb = vec3(M_value_table[C0],
                    M_value_table[C1],
                    M_value_table[C2]);
    mask.a = 1;
}


