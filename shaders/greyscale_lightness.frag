#version 410 core

out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D image;

void main() {
    vec3 c = texture(image, TexCoords).rgb;
    float r = max(c.r, max(c.g, c.b));
    FragColor = vec4(r, r, r, 1.0);
}