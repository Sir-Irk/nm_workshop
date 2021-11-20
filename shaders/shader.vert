#version 460
layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec3 iColor;
layout(location = 2) in vec2 iTexcoord;

uniform mat4 MVP;
out vec3 color; 

void main() {
    gl_Position = MVP * vec4(iPosition, 1.0);
    color = iColor;
}