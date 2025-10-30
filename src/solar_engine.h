#pragma once
#include "engine.h"
#include "transform.h"

class SolarEngine : public Engine {
  TransformPtr m_earthOrbit;
  TransformPtr m_earthSpin;
  TransformPtr m_moonOrbit;
  TransformPtr m_mercuryOrbit;
public:
  SolarEngine(TransformPtr earthOrbit, TransformPtr earthSpin, TransformPtr moonOrbit, TransformPtr mercuryOrbit);
  void Update(float dt) override;
};
