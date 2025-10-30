#version 410

uniform vec4 color = vec4(1.0);
uniform sampler2D decal;

in vec2 v_texcoord;
out vec4 outcolor;

void main() {
  vec4 tex = texture(decal, v_texcoord);
  outcolor = color * tex;
  // (discard removido para evitar sumiço de pixels em texturas sem canal alpha)
}
