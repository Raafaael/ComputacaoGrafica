#version 410

layout(location = 0) in vec4 coord;
layout(location = 1) in vec3 normal;
layout(location = 3) in vec2 texcoord;

uniform mat4 Mv; 
uniform mat4 Mn; 
uniform mat4 Mvp;

// User clip planes in eye space
uniform int  clipCount;            // number of active planes [0..4]
uniform vec4 clipPlane[4];         // plane eq: n.xyz, d (n.p + d = 0), keep where < 0

out VS_OUT {
  vec3 veye;
  vec3 neye;
  vec2 uv;
} v;

void main (void) 
{
  v.veye = vec3(Mv*coord);
  v.neye = normalize(vec3(Mn*vec4(normal,0.0f)));
  v.uv = texcoord;
  gl_Position = Mvp*coord; 

  // Compute clip distances for up to 4 planes
  vec4 eyePos = vec4(v.veye, 1.0);
  if (clipCount > 0) gl_ClipDistance[0] = dot(eyePos, clipPlane[0]);
  if (clipCount > 1) gl_ClipDistance[1] = dot(eyePos, clipPlane[1]);
  if (clipCount > 2) gl_ClipDistance[2] = dot(eyePos, clipPlane[2]);
  if (clipCount > 3) gl_ClipDistance[3] = dot(eyePos, clipPlane[3]);
}

