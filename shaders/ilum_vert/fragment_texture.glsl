#version 410

in VS_OUT {
  vec3 veye;
  vec3 neye;
  vec2 uv;
} f;

out vec4 color;

uniform sampler2D decal;
uniform sampler2D roughness; // roughness map (R channel)
uniform float roughFactor;   // multiplier to amplify/attenuate roughness (debug/control)
// Normal mapping
uniform sampler2D normalMap; // tangent-space normal map (RGB in [0,1], Z up)

// Environment reflection (cubemap)
uniform samplerCube envMap;  // environment map
uniform float envStrength;   // [0..1] blend weight
// Camera-space vector for world up (V * (0,1,0))
uniform vec3 worldUpEye;

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
uniform vec3 fogColor;      // fog color in eye space output
uniform float fogStart;     // distance where fog starts
uniform float fogEnd;       // distance where fog fully covers

void main (void)
{
  // Light vector from point to light
  vec3 L = (lpos.w == 0.0) ? normalize(vec3(lpos))
                           : normalize(vec3(lpos) - f.veye);

  // Distance attenuation (for positional light)
  float attenuation = 1.0;
  if (lpos.w != 0.0) {
    float d = length(vec3(lpos) - f.veye);
    attenuation = 1.0 / max(0.0001, att.x + att.y*d + att.z*d*d);
  }

  // Spotlight factor
  float spot = 1.0;
  if (useSpot == 1 && lpos.w != 0.0) {
    vec3 toFrag = normalize(f.veye - vec3(lpos)); // from light to fragment
    float cosAng = dot(normalize(ldir.xyz), toFrag);
    float s = smoothstep(spotCutoff, spotCutoff + 0.02, cosAng); // soft edge
    spot = pow(s, spotExponent);
  }

  // Base normal in eye space
  vec3 Ngeom = normalize(f.neye);
  // Build TBN basis in eye space on-the-fly using screen-space derivatives
  vec3 dp1 = dFdx(f.veye);
  vec3 dp2 = dFdy(f.veye);
  vec2 duv1 = dFdx(f.uv);
  vec2 duv2 = dFdy(f.uv);
  float det = duv1.x * duv2.y - duv1.y * duv2.x;
  // Avoid division by zero for degenerate UVs
  vec3 T = normalize((dp1 * duv2.y - dp2 * duv1.y) / max(abs(det), 1e-6));
  vec3 B = normalize(cross(Ngeom, T));
  mat3 TBN = mat3(T, B, Ngeom);
  // Sample normal map and bring from [0,1] to [-1,1]
  vec3 nTex = texture(normalMap, f.uv).rgb * 2.0 - 1.0;
  // Final shaded normal in eye space
  vec3 N = normalize(TBN * nTex);
  float ndotl = max(0.0, dot(N, L));
  float vis = attenuation * spot;

  // Roughness mapping (scaled for visibility via roughFactor)
  float rough = clamp(roughFactor * texture(roughness, f.uv).r, 0.0, 1.0);
  float gloss = 1.0 - rough; // 0 = fully rough, 1 = fully glossy
  float mshi_eff = mix(8.0, 256.0, gloss);

  vec4 lit = mamb*lamb + mdif * ldif * (ndotl * vis);
  if (ndotl > 0.0) {
  vec3 R = normalize(reflect(-L, N));
    float specTerm = pow(max(0.0, dot(R, normalize(-f.veye))), mshi_eff);
    lit += mspe * lspe * (specTerm * vis * gloss);
  }
  vec4 tex = texture(decal, f.uv);
  vec4 shaded = lit * tex;

  // Environment reflection using cubemap (view-space reflection vector)
  vec3 V = normalize(-f.veye);
  vec3 Nn = N;
  // Use reflection of the incoming view vector (-V) to get outgoing env lookup
  vec3 R = reflect(-V, Nn);
  vec3 envCol = texture(envMap, R).rgb;
  // Hemisphere gating relative to WORLD UP (eye space)
  float upFacing = clamp(dot(Nn, normalize(worldUpEye)), 0.0, 1.0);
  // Be permissive: start contributing even com leve inclinação
  float hemi = smoothstep(-0.05, 0.10, upFacing);
  // Fade reflections near the horizon instead of hard-cutting
  float aboveHorizon = dot(normalize(R), normalize(worldUpEye));
  float skyMask = smoothstep(-0.10, 0.05, aboveHorizon);
  // Blend more on glossy, up-facing surfaces, and only for rays pointing to the sky
  float reflWeight = clamp(envStrength, 0.0, 1.0) * gloss * hemi * skyMask;
  shaded.rgb = mix(shaded.rgb, envCol, reflWeight);

  // Linear fog based on eye-space distance to camera origin
  float dist = length(f.veye);
  float fogFactor = clamp((fogEnd - dist) / max(0.0001, fogEnd - fogStart), 0.0, 1.0);
  color = mix(vec4(fogColor, 1.0), shaded, fogFactor);
}

