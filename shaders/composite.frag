#version 410 core

out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D image0;
uniform sampler2D image1;

void main() {
    vec3 a = texture(image0, TexCoords).rgb;
    vec3 b = texture(image1, TexCoords).rgb;
    FragColor = vec4((a+b)/2.0, 1.0);
}