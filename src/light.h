#include <memory>
class Light;
using LightPtr = std::shared_ptr<Light>; 

#ifndef LIGHT_H
#define LIGHT_H

#include "node.h"
#include <glm/glm.hpp>
#include <string>

class Light {
  std::string m_space;
  glm::vec4 m_amb;
  glm::vec4 m_dif;
  glm::vec4 m_spe;
  glm::vec4 m_pos;
  NodePtr m_reference;
  // Spotlight and attenuation parameters
  float m_spotCutoffDeg;   // cutoff angle in degrees
  float m_spotExponent;    // focus exponent
  glm::vec3 m_att;         // (constant, linear, quadratic)
  int m_useSpotOverride;   // -1 = auto, 0 = force off, 1 = force on
protected:
  Light (float x, float y, float z, float w, const std::string& space);
public:
  static LightPtr Make (float x, float y, float z, float w=1.0f, const std::string& space="world");
  virtual ~Light ();
  void SetPosition (float x, float y, float z,float w=1.0f);
  void SetAmbient (float r, float g, float b);
  void SetDiffuse (float r, float g, float b);
  void SetSpecular (float r, float g, float b);
  void SetReference (NodePtr reference);
  NodePtr GetReference () const;
  // Spotlight/attenuation controls
  void SetSpotlight (float cutoffDegrees, float exponent);
  void SetAttenuation (float constant, float linear, float quadratic);
  void SetUseSpot (bool enable); // force enable/disable; call with no effect to return to auto
  void Load (StatePtr st) const;
};

#endif