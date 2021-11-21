#version 410 core

out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D image;

void main() {
    vec3 v = texture(image, TexCoords).rgb * 2.0 - 1.0;
    vec3 n = normalize(v) * 0.5 + 0.5;
    FragColor = vec4(n, 1.0);
}