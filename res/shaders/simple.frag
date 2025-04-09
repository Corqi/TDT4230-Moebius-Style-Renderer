#version 430 core

struct LightSource {
    vec3 position;
    vec3 color;
};

in layout(location = 0) vec3 normal;
in layout(location = 1) vec2 textureCoordinates;
in layout(location = 2) vec3 fragment_position;
in layout(location = 3) mat3 TBN_matrix;

#define number_lights 1
uniform LightSource light_source[number_lights];

uniform layout(location = 6) vec3 camera_position;
// uniform layout(location = 7) vec3 ball_position;
// uniform layout(location = 8) double ball_radius;

uniform layout(location = 9) bool is_UI;
uniform layout(location = 10) bool is_normal_map;

layout(binding = 0) uniform sampler2D textureSample;
layout(binding = 1) uniform sampler2D normalSample;

out vec4 color;
out vec4 normalTexture;
out vec4 depthTexture;

float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453); }
float dither(vec2 uv) { return (rand(uv)*2.0-1.0) / 256.0; }
vec3 reject(vec3 from, vec3 onto) {
    return from - onto*dot(from, onto)/dot(onto, onto);
}
vec4 calculateLight(vec3 normal_out){
    // Ambient
    float ambient_intensity = 0.1;
    vec3 ambient_color = vec3(255.0, 255.0, 255.0);
    vec3 ambient = (ambient_color / 255.0) * ambient_intensity;

    // Diffuse and Specular
    vec3 diffuse = vec3(0.0, 0.0, 0.0);
    vec3 specular = vec3(0.0, 0.0, 0.0);

    for (int i = 0; i < number_lights; i++) {
        // Calculate attentuation
        float d = length(light_source[i].position - fragment_position);
        float la = 0.01;
        float lb = 0.005;
        float lc = 0.003;
        float L = 1 / (la + lb*d + lc*pow(d,2));

        // Calculate shadows
        /*
        float soft_shadow_radius = float(ball_radius) * 1.3;

        vec3 light_distance = light_source[i].position - fragment_position;
        vec3 ball_distance = ball_position - fragment_position;
        float shadow_strenght = 1.0;
        float rejection_length = length(reject(ball_distance, light_distance));
        
        if (length(ball_distance) <= length(light_distance) && dot(light_distance, ball_distance) >= 0) {
            // Full shadow
            if (rejection_length <= ball_radius)
                shadow_strenght = 0.0;
            // Soft shadow
            else if (rejection_length <= soft_shadow_radius ) {
                float t = (rejection_length - float(ball_radius)) / (soft_shadow_radius - float(ball_radius));
                shadow_strenght = mix(0.0, 1.0, t);
            }
        }*/

        // Calculate diffuse
        vec3 light_direction = normalize(light_source[i].position - fragment_position);
        float diffuse_intensity = max(dot(light_direction, normal_out), 0.0);
        // vec3 diffuse_color = vec3(255.0, 255.0, 255.0) / 255.0;
        vec3 diffuse_color = light_source[i].color / 255.0;

        diffuse += diffuse_intensity * diffuse_color * L;

        // Calculate specular
        vec3 reflect_direction = reflect(-light_direction, normal_out);
        vec3 view_direction = normalize(camera_position - fragment_position);
        float specular_intensity = pow(max(dot(view_direction, reflect_direction), 0.0), 32);
        // vec3 specular_color = vec3(255.0, 255.0, 255.0) / 255.0;
        vec3 specular_color = light_source[i].color / 255.0;

        specular += specular_intensity * specular_color * L;
    }

    // Dither
    float noise = dither(textureCoordinates);

    // color = vec4(0.5 * normal + 0.5, 1.0);
    color = vec4(ambient + diffuse + specular + noise, 1.0);

    return color;
}

float near = 0.1f;
float far = 100.0f;

float linearizeDepth(float depth)
{
	return (2.0 * near * far) / (far + near - (depth * 2.0 - 1.0) * (far - near));
}

void main()
{
    // Normalize normals 2nd time
    vec3 normal_out = normalize(normal);

    if (is_UI) {
        // color = vec4(1.0, 1.0, 1.0, 1.0);
        color = texture(textureSample, textureCoordinates);
    }
    else if (is_normal_map){
        // Use normal map texture
        //vec3 normalFromMap = texture(normalSample, textureCoordinates).rgb;
        // Convert to [-1, 1] range
        // normal_out = normalize(TBN_matrix * (normalFromMap * 2.0 - 1.0));

        // Compute lighting color
        vec3 litColor = calculateLight(normal_out).rgb;

        // Calculate brightness (luminance)
        float brightness = dot(litColor, vec3(0.299, 0.587, 0.114));

        color =  texture(textureSample, textureCoordinates);

        if (brightness < 0.75) {
            if (mod(gl_FragCoord.y, 5.0) <= 1.0) {
                color = vec4(0.0, 0.0, 0.0, 1.0);
            }
        }

        if (brightness < 0.50) {
            if (mod(gl_FragCoord.x, 5.0) <= 1.0) {
                color = vec4(0.0, 0.0, 0.0, 1.0);
            }
        }
        
        if (brightness < 0.35) {
            if (mod(gl_FragCoord.x + gl_FragCoord.y, 5.0) == 0.0) {
                color = vec4(0.0, 0.0, 0.0, 1.0);
            }
        }
        
        if (brightness < 0.12) {
            if (mod(gl_FragCoord.x - gl_FragCoord.y, 5.0) == 0.0) {
                color = vec4(0.0, 0.0, 0.0, 1.0);
            }
        }

        //debug
        //color = vec4(0.5 * normal_out + 0.5, 1.0);
    }
    else {
        // Compute lighting color
        vec3 litColor = calculateLight(normal_out).rgb;

        // Calculate brightness (luminance)
        float brightness = dot(litColor, vec3(0.299, 0.587, 0.114));
        
        
        color = vec4(1.0, 1.0, 1.0, 1.0);
     
     /*
        if (brightness < 1.00) {
            if (mod(gl_FragCoord.x + gl_FragCoord.y, 10.0) == 0.0) {
                color = vec4(0.0, 0.0, 0.0, 1.0);
            }
        }*/
        
        if (brightness < 0.75) {
            if (mod(gl_FragCoord.y, 5.0) <= 1.0) {
                color = vec4(0.0, 0.0, 0.0, 1.0);
            }
        }

        if (brightness < 0.50) {
            if (mod(gl_FragCoord.x, 5.0) <= 1.0) {
                color = vec4(0.0, 0.0, 0.0, 1.0);
            }
        }
        
        if (brightness < 0.35) {
            if (mod(gl_FragCoord.x + gl_FragCoord.y, 5.0) == 0.0) {
                color = vec4(0.0, 0.0, 0.0, 1.0);
            }
        }
        
        if (brightness < 0.12) {
            if (mod(gl_FragCoord.x - gl_FragCoord.y, 5.0) == 0.0) {
                color = vec4(0.0, 0.0, 0.0, 1.0);
            }
        }

        // debug
        //color = calculateLight(normal_out);
    }

    
    normalTexture = vec4(0.5 * normal_out + 0.5, 1.0);
    depthTexture = vec4(vec3(linearizeDepth(gl_FragCoord.z) / far), 1.0);
}