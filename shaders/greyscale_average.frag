#version 410 core

out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D image;

void main() {
    vec3 c = texture(image, TexCoords).rgb;
    float avg = (c.r+c.g+c.b)/2.0;
    FragColor = vec4(avg, avg, avg, 1.0);
}