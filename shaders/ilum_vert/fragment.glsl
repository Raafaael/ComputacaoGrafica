#version 410

in VS_OUT {
  vec3 veye;
  vec3 neye;
} f;

out vec4 fcolor;

uniform vec4 lpos;  // light pos in eye space
uniform vec4 lamb;
uniform vec4 ldif;
uniform vec4 lspe;

// Spotlight and attenuation controls
uniform int useSpot;            // 0 = off, 1 = spotlight
uniform vec4 ldir;              // light direction in eye space (xyz used)
uniform float spotCutoff;       // cosine of cutoff angle
uniform float spotExponent;     // spot focus exponent
uniform vec3 att;               // (constant, linear, quadratic)

uniform vec4 mamb;
uniform vec4 mdif;
uniform vec4 mspe;
uniform float mshi;

// Fog controls (linear fog)
uniform vec3 fogColor;
uniform float fogStart;
uniform float fogEnd;

void main (void)
{
  vec3 L = (lpos.w == 0.0) ? normalize(vec3(lpos))
                           : normalize(vec3(lpos) - f.veye);

  float attenuation = 1.0;
  if (lpos.w != 0.0) {
    float d = length(vec3(lpos) - f.veye);
    attenuation = 1.0 / max(0.0001, att.x + att.y*d + att.z*d*d);
  }

  float spot = 1.0;
  if (useSpot == 1 && lpos.w != 0.0) {
    vec3 toFrag = normalize(f.veye - vec3(lpos));
    float cosAng = dot(normalize(ldir.xyz), toFrag);
    float s = smoothstep(spotCutoff, spotCutoff + 0.02, cosAng);
    spot = pow(s, spotExponent);
  }

  vec3 N = normalize(f.neye);
  float ndotl = max(0.0, dot(N, L));
  float vis = attenuation * spot;

  vec4 col = mamb*lamb + mdif * ldif * (ndotl * vis);
  if (ndotl > 0.0) {
    vec3 R = normalize(reflect(-L, N));
    col += mspe * lspe * pow(max(0.0, dot(R, normalize(-f.veye))), mshi) * vis;
  }
  float dist = length(f.veye);
  float fogFactor = clamp((fogEnd - dist) / max(0.0001, fogEnd - fogStart), 0.0, 1.0);
  fcolor = mix(vec4(fogColor, 1.0), col, fogFactor);
}

