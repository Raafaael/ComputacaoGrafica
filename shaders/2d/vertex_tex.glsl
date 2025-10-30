#version 410

layout (location=0) in vec4 vertex;
layout (location=3) in vec2 texcoord;

uniform mat4 Mvp;

out vec2 v_texcoord;

void main() {
  gl_Position = Mvp * vertex;
  v_texcoord = texcoord;
}
