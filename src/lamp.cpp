#include "lamp.h"
#include "transform.h"
#include "material.h"
#include "texture.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

NodePtr MakeLamp(const glm::vec3& basePos,
                 float topY,
                 const glm::vec2& headTargetXY,
                 AppearancePtr mat_metal,
                 AppearancePtr mat_white,
                 AppearancePtr tex_white,
                 ShapePtr cylinder,
                 ShapePtr cone)
{
  // Lamp group transform
  TransformPtr trf_lamp = Transform::Make();
  trf_lamp->Translate(basePos.x, basePos.y, basePos.z);

  // Base (thin cylinder)
  const float baseH = 0.05f;
  TransformPtr trf_lamp_base = Transform::Make();
  trf_lamp_base->Translate(0.0f, topY + 0.5f*baseH, 0.0f);
  trf_lamp_base->Scale(0.16f, baseH, 0.16f);
  NodePtr lamp_base = Node::Make(trf_lamp_base, {mat_metal, tex_white}, {cylinder});

  // Two arms (cylinders)
  const float armT = 0.045f;
  const float L1   = 0.48f;
  const float L2   = 0.36f;
  const float ang1 = +55.0f; // degrees
  const float ang2 = -35.0f; // degrees

  const float r1 = glm::radians(ang1);
  const float r2 = glm::radians(ang2);

  // Joint positions in XY plane
  const float j1x = 0.0f, j1y = topY + baseH, j1z = 0.0f;
  const float c1x = j1x + (-sinf(r1)) * 0.5f * L1;
  const float c1y = j1y + ( cosf(r1)) * 0.5f * L1;
  const float j2x = j1x + (-sinf(r1)) * L1;
  const float j2y = j1y + ( cosf(r1)) * L1;
  const float c2x = j2x + (-sinf(r2)) * 0.5f * L2;
  const float c2y = j2y + ( cosf(r2)) * 0.5f * L2;

  TransformPtr trf_arm1 = Transform::Make();
  trf_arm1->Translate(c1x, c1y, j1z);
  trf_arm1->Rotate(ang1, 0.0f, 0.0f, 1.0f);
  trf_arm1->Scale(armT, L1, armT);
  // Arms material: orange
  MaterialPtr mat_arm_orange = Material::Make(1.0f, 0.5f, 0.0f);
  mat_arm_orange->SetShininess(24.0f);
  NodePtr lamp_arm1 = Node::Make(trf_arm1, {mat_arm_orange, tex_white}, {cylinder});

  TransformPtr trf_arm2 = Transform::Make();
  trf_arm2->Translate(c2x, c2y, j1z);
  trf_arm2->Rotate(ang2, 0.0f, 0.0f, 1.0f);
  trf_arm2->Scale(armT, L2, armT);
  NodePtr lamp_arm2 = Node::Make(trf_arm2, {mat_arm_orange, tex_white}, {cylinder});

  // Head (cone) pointing toward target (XY)
  const float j3x = j2x + (-sinf(r2)) * L2;
  const float j3y = j2y + ( cosf(r2)) * L2;

  const float headR = 0.19f;
  const float headH = 0.46f;

  const float vx = headTargetXY.x - j3x;
  const float vy = headTargetXY.y - j3y;
  const float angHead = glm::degrees(atan2f(vx, -vy));

  const float angRad = glm::radians(angHead);
  const float ax = -sinf(angRad);
  const float ay =  cosf(angRad);

  const float cHx = j3x - ax * 0.5f * headH; // apex at j3
  const float cHy = j3y - ay * 0.5f * headH;

  // Split head transform: a pivot with translate+rotate, and a child with scale+geometry.
  TransformPtr trf_head_pivot = Transform::Make();
  trf_head_pivot->Translate(cHx, cHy, 0.0f);
  trf_head_pivot->Rotate(angHead, 0.0f, 0.0f, 1.0f);
  NodePtr lamp_head_pivot = Node::Make(
    trf_head_pivot,
    std::initializer_list<AppearancePtr>{},
    std::initializer_list<NodePtr>{}
  );

  TransformPtr trf_head_geom = Transform::Make();
  trf_head_geom->Scale(headR, headH, headR);
  // Head (cone) material: dark blue
  MaterialPtr mat_head_blue = Material::Make(0.05f, 0.10f, 0.30f);
  mat_head_blue->SetShininess(32.0f);
  NodePtr lamp_head_geom = Node::Make(trf_head_geom, {mat_head_blue, tex_white}, {cone});
  lamp_head_pivot->AddNode(lamp_head_geom);

  // Assemble group
  NodePtr lamp = Node::Make(trf_lamp, { lamp_base, lamp_arm1, lamp_arm2, lamp_head_pivot });
  return lamp;
}

NodePtr MakeLamp(const glm::vec3& basePos,
                 float topY,
                 ShapePtr cylinder,
                 ShapePtr cone)
{
  // default materials/textures
  auto mat_metal = Material::Make(0.75f, 0.75f, 0.78f);
  mat_metal->SetSpecular(1.0f,1.0f,1.0f);
  mat_metal->SetShininess(96.0f);
  auto mat_white = Material::Make(1.0f,1.0f,1.0f);
  auto tex_white = Texture::Make("decal", glm::vec3(1.0f,1.0f,1.0f));
  // default head target: point toward the scene's ball position used in main
  // (ball at x=+0.35, y=topY + 0.06) to preserve previous behavior
  glm::vec2 headTargetXY(0.35f, topY + 0.06f);
  return MakeLamp(basePos, topY, headTargetXY, mat_metal, mat_white, tex_white, cylinder, cone);
}

NodePtr MakeLamp(const glm::vec3& basePos,
                 float topY,
                 ShapePtr cylinder,
                 ShapePtr cone,
                 LightPtr attachLight)
{
  // Build with defaults first, but we need to attach the light to the lamp head apex.
  // Rebuild here to gain access to lamp_head and set light reference.
  auto mat_metal = Material::Make(0.75f, 0.75f, 0.78f);
  mat_metal->SetSpecular(1.0f,1.0f,1.0f);
  mat_metal->SetShininess(96.0f);
  auto mat_white = Material::Make(1.0f,1.0f,1.0f);
  auto tex_white = Texture::Make("decal", glm::vec3(1.0f,1.0f,1.0f));

  // Lamp group transform
  TransformPtr trf_lamp = Transform::Make();
  trf_lamp->Translate(basePos.x, basePos.y, basePos.z);

  // Base
  const float baseH = 0.05f;
  TransformPtr trf_lamp_base = Transform::Make();
  trf_lamp_base->Translate(0.0f, topY + 0.5f*baseH, 0.0f);
  trf_lamp_base->Scale(0.16f, baseH, 0.16f);
  NodePtr lamp_base = Node::Make(trf_lamp_base, {mat_metal, tex_white}, {cylinder});

  // Arms
  const float armT = 0.045f;
  const float L1   = 0.48f;
  const float L2   = 0.36f;
  const float ang1 = +55.0f;
  const float ang2 = -35.0f;
  const float r1 = glm::radians(ang1);
  const float r2 = glm::radians(ang2);
  const float j1x = 0.0f, j1y = topY + baseH, j1z = 0.0f;
  const float c1x = j1x + (-sinf(r1)) * 0.5f * L1;
  const float c1y = j1y + ( cosf(r1)) * 0.5f * L1;
  const float j2x = j1x + (-sinf(r1)) * L1;
  const float j2y = j1y + ( cosf(r1)) * L1;
  const float c2x = j2x + (-sinf(r2)) * 0.5f * L2;
  const float c2y = j2y + ( cosf(r2)) * 0.5f * L2;

  TransformPtr trf_arm1 = Transform::Make();
  trf_arm1->Translate(c1x, c1y, j1z);
  trf_arm1->Rotate(ang1, 0.0f, 0.0f, 1.0f);
  trf_arm1->Scale(armT, L1, armT);
  MaterialPtr mat_arm_orange = Material::Make(1.0f, 0.5f, 0.0f);
  mat_arm_orange->SetShininess(24.0f);
  NodePtr lamp_arm1 = Node::Make(trf_arm1, {mat_arm_orange, tex_white}, {cylinder});

  TransformPtr trf_arm2 = Transform::Make();
  trf_arm2->Translate(c2x, c2y, j1z);
  trf_arm2->Rotate(ang2, 0.0f, 0.0f, 1.0f);
  trf_arm2->Scale(armT, L2, armT);
  NodePtr lamp_arm2 = Node::Make(trf_arm2, {mat_arm_orange, tex_white}, {cylinder});

  // Head apex aiming default ball position
  const float j3x = j2x + (-sinf(r2)) * L2;
  const float j3y = j2y + ( cosf(r2)) * L2;
  const float headR = 0.19f;
  const float headH = 0.46f;
  glm::vec2 headTargetXY(0.35f, topY + 0.06f);
  const float vx = headTargetXY.x - j3x;
  const float vy = headTargetXY.y - j3y;
  float angHead = glm::degrees(atan2f(vx, -vy));
  const float angRad = glm::radians(angHead);
  const float ax = -sinf(angRad);
  const float ay =  cosf(angRad);
  const float cHx = j3x - ax * 0.5f * headH;
  const float cHy = j3y - ay * 0.5f * headH;
  // Head pivot (translate+rotate) and geometry (scale+mesh)
  TransformPtr trf_head_pivot = Transform::Make();
  trf_head_pivot->Translate(cHx, cHy, 0.0f);
  trf_head_pivot->Rotate(angHead, 0.0f, 0.0f, 1.0f);
  NodePtr lamp_head_pivot = Node::Make(
    trf_head_pivot,
    std::initializer_list<AppearancePtr>{},
    std::initializer_list<NodePtr>{}
  );

  TransformPtr trf_head_geom = Transform::Make();
  trf_head_geom->Scale(headR, headH, headR);
  MaterialPtr mat_head_blue = Material::Make(0.05f, 0.10f, 0.30f);
  mat_head_blue->SetShininess(32.0f);
  NodePtr lamp_head_geom = Node::Make(trf_head_geom, {mat_head_blue, tex_white}, {cone});
  lamp_head_pivot->AddNode(lamp_head_geom);

  // Attach light to the head and make it DIRECTIONAL along the head's +Y axis.
  // Using w=0 turns it into a directional light; with a reference node, its
  // direction follows the head orientation (no distance attenuation).
  if (attachLight) {
    attachLight->SetReference(lamp_head_pivot);
    // Use head's +Y as directional vector, matching shader's L = normalize(vec3(lpos)).
    // If this looks inverted in your scene, flip the sign to (0,-1,0,0).
    attachLight->SetPosition(0.0f, 1.0f, 0.0f, 0.0f);
  }

  NodePtr lamp = Node::Make(trf_lamp, { lamp_base, lamp_arm1, lamp_arm2, lamp_head_pivot });
  return lamp;
}
