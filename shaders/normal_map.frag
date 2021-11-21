#version 410 core

out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D image;
uniform float scale;
uniform float flipY;

float xk[9] = float[](
     -1, 0, 1,
     -2, 0, 2,
     -1, 0, 1 
);
float yk[9] = float[](
     -1, -2, -1,
      0,  0,  0,
      1,  2,  1 
);

void main() {
    vec2 tex_offset = 1.0 / textureSize(image, 0);
    float xmag = 0;
    float ymag = 0;
    for(int a = 0; a < 3; ++a) {
        for(int b = 0; b < 3; ++b) {
            vec2 offset = vec2(tex_offset.x*(b-1), tex_offset.y*(a-1));
            vec3 pixel = texture(image, TexCoords + offset).rgb;
            xmag += pixel.r * xk[a * 3 + b];
            ymag += pixel.r * yk[a * 3 + b];
        }
    }

    vec3 result = normalize(vec3(xmag*scale, ymag*scale*flipY, 1.0));
    result = result * 0.5 + 0.5;

    FragColor = vec4(result, 1.0);
}