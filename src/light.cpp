#include "light.h"
#include "node.h"
#include "state.h"
#include "shader.h"
#include "camera.h"

#ifdef _WIN32
//#define GLAD_GL_IMPLEMENTATION // Necessary for headeronly version.
#include <glad/gl.h>
#elif __APPLE__
#include <OpenGL/gl3.h>
#endif

LightPtr Light::Make (float x, float y, float z, float w, const std::string& space)
{
  return LightPtr(new Light(x,y,z,w,space));
}

Light::Light (float x, float y, float z, float w, const std::string& space)
: m_space(space),
  m_amb{0.3f,0.3f,0.3f,1.0f},
  m_dif{0.7f,0.7f,0.7f,1.0f},
  m_spe{1.0f,1.0f,1.0f,1.0f},
  m_pos{x,y,z,w},
  m_reference(nullptr),
  m_spotCutoffDeg(14.0f),   // tighter beam for a desk lamp
  m_spotExponent(32.0f),    // focused center
  m_att(1.0f, 0.22f, 0.20f),// noticeable falloff at scene scale
  m_useSpotOverride(-1)
{
}

Light::~Light ()
{
}

void Light::SetAmbient (float r, float g, float b)
{
  m_amb[0] = r;
  m_amb[1] = g;
  m_amb[2] = b;
}

void Light::SetDiffuse (float r, float g, float b)
{
  m_dif[0] = r;
  m_dif[1] = g;
  m_dif[2] = b;
}

void Light::SetSpecular (float r, float g, float b)
{
  m_spe[0] = r;
  m_spe[1] = g;
  m_spe[2] = b;
}

void Light::SetReference (NodePtr reference)
{
  m_reference = reference;
}

NodePtr Light::GetReference () const
{
  return m_reference;
}

void Light::SetPosition (float x, float y, float z, float w)
{
  m_pos[0] = x;
  m_pos[1] = y;
  m_pos[2] = z;
  m_pos[3] = w;
}

void Light::SetSpotlight (float cutoffDegrees, float exponent)
{
  m_spotCutoffDeg = cutoffDegrees;
  m_spotExponent  = exponent;
}

void Light::SetAttenuation (float constant, float linear, float quadratic)
{
  m_att = glm::vec3(constant, linear, quadratic);
}

void Light::SetUseSpot (bool enable)
{
  m_useSpotOverride = enable ? 1 : 0;
}

void Light::Load (StatePtr st) const
{
  ShaderPtr shd = st->GetShader();
  shd->SetUniform("lamb",m_amb);
  shd->SetUniform("ldif",m_dif);
  shd->SetUniform("lspe",m_spe);

  // Set position in the lighting space
  glm::mat4 M(1.0f);
  if (m_space == "world" && shd->GetLightingSpace() == "camera") {
    M = st->GetCamera()->GetViewMatrix();
  }
  else if (m_space == "camera" && shd->GetLightingSpace() == "world") {
    M = glm::inverse(st->GetCamera()->GetViewMatrix());
  }
  if (GetReference()) {
    M = M * GetReference()->GetModelMatrix();
  }
  glm::vec4 pos = M * m_pos;
  shd->SetUniform("lpos",pos);

  // Spotlight/directional support: compute direction from reference's +Y
  // If there's a reference, use its +Y axis transformed to lighting space.
  // Otherwise default to -Z (camera forward) in the chosen space.
  glm::vec3 ldir(0.0f, 0.0f, -1.0f);
  if (GetReference()) {
    // Aim along the head's -Y axis (from apex toward the cone opening / table)
    glm::vec4 d = M * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
    ldir = glm::normalize(glm::vec3(d));
  }
  shd->SetUniform("ldir", glm::vec4(ldir, 0.0f)); // vec4 overload, fragment uses xyz
  // Enable spotlight only when we have a positional light and a reference,
  // unless overridden by user.
  int useSpot = (pos.w != 0.0f && GetReference() != nullptr) ? 1 : 0;
  if (m_useSpotOverride != -1) useSpot = m_useSpotOverride;
  shd->SetUniform("useSpot", useSpot);
  // Spotlight params (degrees -> cosine cutoff)
  shd->SetUniform("spotCutoff", cosf(m_spotCutoffDeg * 3.14159265f/180.0f));
  shd->SetUniform("spotExponent", m_spotExponent);
  // Distance attenuation (constant, linear, quadratic)
  shd->SetUniform("att", m_att);
}