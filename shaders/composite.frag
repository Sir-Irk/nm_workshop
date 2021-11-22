#version 410 core

out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D image0;
uniform sampler2D image1;

uniform sampler2D[5] images;
uniform int numImages;

void main() {
    vec3 accum = vec3(0,0,0);
    for(int i = 0; i < numImages; ++i) {
        accum += texture(images[i], TexCoords).rgb * 2.0 - 1.0;
    }
    vec3 n = normalize(accum) * 0.5 + 0.5;
    //vec3 n = accum*0.5;
    FragColor = vec4(n, 1.0);
}