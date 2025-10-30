#pragma once

#include "node.h"
#include "appearance.h"
#include "shape.h"
#include <glm/glm.hpp>

// Build a desk lamp: base + two cylindrical arms + conical head.
// Parameters:
// - basePos: translation applied to the lamp group (relative to table center)
// - topY: table top Y position (used to place base atop the table)
// - headTargetXY: target point (x,y) in the XY plane the head should point to
// - appearances: metal material, white material, and a white decal texture
// - shapes: references to cylinder and cone shapes (reused VAOs)
NodePtr MakeLamp(const glm::vec3& basePos,
                 float topY,
                 const glm::vec2& headTargetXY,
                 AppearancePtr mat_metal,
                 AppearancePtr mat_white,
                 AppearancePtr tex_white,
                 ShapePtr cylinder,
                 ShapePtr cone);

// Convenience overload with sensible defaults:
// - Creates default metal/white materials and a white decal internally
// - Points the head slightly forward by default
NodePtr MakeLamp(const glm::vec3& basePos,
                 float topY,
                 ShapePtr cylinder,
                 ShapePtr cone);

// Short overload that also attaches a Light to the lamp head (apex) so that
// lpos/ldir come from the lamp's cúpula; the light is transformed with the head node.
NodePtr MakeLamp(const glm::vec3& basePos,
                 float topY,
                 ShapePtr cylinder,
                 ShapePtr cone,
                 LightPtr attachLight);
