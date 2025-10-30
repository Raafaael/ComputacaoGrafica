#pragma once

#include "node.h"
#include "appearance.h"
#include "shape.h"

// Build a wooden table composed of a top (cube) and 4 legs (cubes).
// Dimensions are specified in scene units and match the conventions used in main_3d.
// Parameters:
// - topY: height of the top surface center along Y (the top cube is centered at topY - 0.5*topH)
// - topHX, topHZ: half-sizes of the top along X and Z
// - topH: thickness of the top
// - legH: height of the legs
// - legS: thickness (square section) of each leg
// - mat_wood, tex_wood: material and decal for wood appearance
// - cube: reusable cube shape (VAO)
NodePtr MakeTable(float topY,
                  float topHX,
                  float topHZ,
                  float topH,
                  float legH,
                  float legS,
                  AppearancePtr mat_wood,
                  AppearancePtr tex_wood,
                  ShapePtr cube);

// Convenience overload with sensible default dimensions used in the sample scene.
NodePtr MakeTable(float topY,
                  AppearancePtr mat_wood,
                  AppearancePtr tex_wood,
                  ShapePtr cube);
