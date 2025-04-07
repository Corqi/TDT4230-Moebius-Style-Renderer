#version 430 core

out vec4 color;
in vec2 texCoords;

uniform sampler2D screenTexture;

const float offset_x = 1.0f / 800.0f;
const float offset_y = 1.0f / 800.0f;

vec2 offsets[9] = vec2[] (
    vec2(-offset_x, offset_y),  vec2(0.0f, offset_y),   vec2(offset_x, offset_y),
    vec2(-offset_x, 0.0f),      vec2(0.0f, 0.0f),       vec2(offset_x, 0.0f),
    vec2(-offset_x, -offset_y), vec2(0.0f, -offset_y),  vec2(offset_x, -offset_y)
);

float kernelX[9] = float[] (
    1.0,    0.0,    -1.0,
    2.0,    0.0,    -2.0,
    1.0,    0.0,    -1.0
);

float kernelY[9] = float[] (
     1.0,  2.0,  1.0,
     0.0,  0.0,  0.0,
    -1.0, -2.0, -1.0
);

void main(){
    float gx = 0.0;
    float gy = 0.0;

    for(int i = 0; i < 9; i++) {
        // convert sampled color to grayscale (luminance)
        float gray = dot(texture(screenTexture, texCoords + offsets[i]).rgb, vec3(0.299, 0.587, 0.114));
        gx += gray * kernelX[i];
        gy += gray * kernelY[i];
    }

    float edge = sqrt(gx * gx + gy * gy);
    // create line with same thickness
    float threshold = 0.25;
    float outline = edge > threshold ? 0.0 : 1.0;

    vec3 originalColor = texture(screenTexture, texCoords).rgb;

    vec3 finalColor = originalColor * outline;

    color = vec4(finalColor, 1.0);
    // color = texture(screenTexture, texCoords); 
}