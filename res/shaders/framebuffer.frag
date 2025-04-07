#version 430 core

out vec4 color;
in vec2 texCoords;

uniform sampler2D screenTexture;
uniform sampler2D normalTexture;
uniform sampler2D depthTexture;

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

// Simple noise
float hash(vec2 p) {
    return fract(sin(dot(p ,vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 uv) {
    vec2 i = floor(uv);
    vec2 f = fract(uv);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f); // smoothstep interpolation

    return mix(a, b, u.x) +
           (c - a)* u.y * (1.0 - u.x) +
           (d - b) * u.x * u.y;
}

void main(){
    // apply noise on texCoords
    vec2 uvJitter = texCoords + 0.003 * vec2(
        noise(texCoords * 10.0),
        noise(texCoords * 10.0)
    );

    vec3 centerNormal = texture(normalTexture, texCoords).rgb;
    centerNormal = normalize(centerNormal * 2.0 - 1.0); // Convert from [0,1] to [-1,1]
    float centerDepth = texture(depthTexture, texCoords).r;

    float normalEdge = 0.0;
    float depthEdge = 0.0;
    float gx = 0.0;
    float gy = 0.0;
    float depthGx = 0.0;
    float depthGy = 0.0;

    for(int i = 0; i < 9; i++) {
        vec3 sampleNormal = texture(normalTexture, uvJitter + offsets[i]).rgb;
        sampleNormal = normalize(sampleNormal * 2.0 - 1.0);
        
        // Calculate how much the normal differs from center
        float normalDiff = 1.0 - dot(centerNormal, sampleNormal);
        
        // Apply your original kernels
        gx += normalDiff * kernelX[i];
        gy += normalDiff * kernelY[i];

        // Depth edge (using same kernel)
        float sampleDepth = texture(depthTexture, texCoords + offsets[i]).r;
        float depthDiff = abs(centerDepth - sampleDepth) * 50.0; // Scaled for visibility

        depthGx += depthDiff * kernelX[i];
        depthGy += depthDiff * kernelY[i];
    }

    // Calculate edge strengths
    float normalEdgeStrength = sqrt(gx*gx + gy*gy);
    float depthEdgeStrength = sqrt(depthGx*depthGx + depthGy*depthGy);
    
    // Combine edges (using max to keep strongest signal)
    float combinedEdge = max(normalEdgeStrength, depthEdgeStrength);
    
    // Apply threshold (using your original style)
    float edge = combinedEdge > 0.3 ? 1.0 : 0.0;
    
    // Final output
    vec3 originalColor = texture(screenTexture, texCoords).rgb;
    color = vec4(originalColor * (1.0 - edge), 1.0);
    
    // Debug views (uncomment to see different components):
    //color = vec4(vec3(normalEdgeStrength), 1.0); // Normal edges only
    //color = vec4(vec3(depthEdgeStrength), 1.0);  // Depth edges only
    //color = vec4(centerNormal*0.5+0.5, 1.0);     // Visualize normals
    //color = vec4(vec3(centerDepth), 1.0);        // Visualize depth
    //color = texture(screenTexture, texCoords);  // Visualize initial render
    //color = vec4(vec3(linearizeDepth(centerDepth) / far), 1.0f);
    // color = vec4(vec3(centerDepth), 1.0f);
}