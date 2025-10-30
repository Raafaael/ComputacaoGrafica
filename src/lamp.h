#pragma once
#include <glm/glm.hpp>
#include "node.h"
#include "appearance.h"
#include "shape.h"
#include "light.h"

// Overload com target e materiais/textura explícitos
NodePtr MakeLamp(const glm::vec3& basePos,
                 float topY,
                 const glm::vec2& headTargetXY,
                 AppearancePtr mat_metal,
                 AppearancePtr mat_white,
                 AppearancePtr tex_white,
                 ShapePtr cylinder,
                 ShapePtr cone);

// Overload simples (defaults internos)
NodePtr MakeLamp(const glm::vec3& basePos,
                 float topY,
                 ShapePtr cylinder,
                 ShapePtr cone);

// Overload com luz acoplada ao cabeçote
NodePtr MakeLamp(const glm::vec3& basePos,
                 float topY,
                 ShapePtr cylinder,
                 ShapePtr cone,
                 LightPtr attachLight);
