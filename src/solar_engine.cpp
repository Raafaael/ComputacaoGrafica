#include "solar_engine.h"

#ifdef _WIN32
#include <glad/gl.h>
#elif __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

SolarEngine::SolarEngine(TransformPtr earthOrbit, TransformPtr earthSpin, TransformPtr moonOrbit, TransformPtr mercuryOrbit)
: m_earthOrbit(earthOrbit), m_earthSpin(earthSpin), m_moonOrbit(moonOrbit), m_mercuryOrbit(mercuryOrbit)
{}

static constexpr float EARTH_ORBIT_SPEED   = 15.0f;  // deg/s
static constexpr float EARTH_SPIN_SPEED    = -45.0f; // negativo para sentido anti-horário visual
static constexpr float MOON_ORBIT_SPEED    = 60.0f;  // deg/s
static constexpr float MERCURY_ORBIT_SPEED = 70.0f;  // deg/s
void SolarEngine::Update(float dt) {
  if (m_earthOrbit)   m_earthOrbit->Rotate(EARTH_ORBIT_SPEED * dt,   0,0,1);
  if (m_earthSpin)    m_earthSpin->Rotate(EARTH_SPIN_SPEED * dt,     0,0,1);
  if (m_moonOrbit)    m_moonOrbit->Rotate(MOON_ORBIT_SPEED * dt,     0,0,1);
  if (m_mercuryOrbit) m_mercuryOrbit->Rotate(MERCURY_ORBIT_SPEED * dt,0,0,1);
}
