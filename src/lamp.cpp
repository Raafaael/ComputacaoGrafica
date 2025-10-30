#include "lamp.h"
#include "transform.h"
#include "material.h"
#include "texture.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

// -----------------------------------------------------------------------------
// Builder compacto, reutilizado pelos overloads
// -----------------------------------------------------------------------------
static NodePtr BuildLamp(const glm::vec3& basePos,
                         float topY,
                         const glm::vec2& headTargetXY,
                         const AppearancePtr& mat_metal,
                         const AppearancePtr& mat_white,
                         const AppearancePtr& tex_white,
                         const ShapePtr& cylinder,
                         const ShapePtr& cone,
                         const LightPtr& attachLight)
{
  // ----- parâmetros simples (fáceis de ajustar) -----
  const float baseH = 0.05f;   // altura da base (cilindro)
  const float armR  = 0.045f;  // raio/espessura das hastes (X/Z do scale)
  const float L1    = 0.48f;   // comprimento 1ª haste
  const float L2    = 0.36f;   // comprimento 2ª haste

  const float ang1  = 14.0f;   // 1ª haste: leve inclinação (graus)
  const float ang2  = -65.0f;  // 2ª haste: “quebra” para frente (graus)

  // cúpula (cone) metade do tamanho
  const float headScale = 0.65f;
  const float headR0    = 0.19f;
  const float headH0    = 0.46f;
  const float headR     = headR0 * headScale; // 0.095
  const float headH     = headH0 * headScale; // 0.23

  const float eps   = 0.003f;  // folga para não intersectar visualmente

  // ----- grupo + base -----
  TransformPtr trfLamp = Transform::Make();
  trfLamp->Translate(basePos.x, basePos.y, basePos.z);

  TransformPtr trfBase = Transform::Make();
  trfBase->Translate(0.0f, topY + 0.5f*baseH, 0.0f);
  trfBase->Scale(0.16f, baseH, 0.16f);
  NodePtr lampBase = Node::Make(trfBase, {mat_metal, tex_white}, {cylinder});

  // ----- cinemática planar XY -----
  const float r1 = glm::radians(ang1);
  const float r2 = glm::radians(ang2);

  const glm::vec2 d1(-sinf(r1),  cosf(r1));  // direção da 1ª haste
  const glm::vec2 n1( cosf(r1),  sinf(r1));  // lateral da 1ª haste (perpendicular)

  const glm::vec2 j1(0.0f, topY + baseH);    // junta base
  const glm::vec2 c1 = j1 + d1 * (0.5f * L1);
  const glm::vec2 j2c = j1 + d1 * L1;                // fim central da 1ª haste
  const glm::vec2 j2  = j2c + n1 * (armR + eps);     // *** SAÍDA PELA LATERAL ***

  const glm::vec2 d2(-sinf(r2),  cosf(r2));  // direção da 2ª haste
  const glm::vec2 c2 = j2 + d2 * (0.5f * L2);

  // ----- hastes -----
  AppearancePtr matArm = Material::Make(1.0f, 0.5f, 0.0f);
  std::static_pointer_cast<Material>(matArm)->SetShininess(24.0f);

  TransformPtr trfArm1 = Transform::Make();
  trfArm1->Translate(c1.x, c1.y, 0.0f);
  trfArm1->Rotate(ang1, 0.0f, 0.0f, 1.0f);
  trfArm1->Scale(armR, L1, armR);
  NodePtr arm1 = Node::Make(trfArm1, {matArm, tex_white}, {cylinder});

  // offset único da 2ª haste (aplicado igualmente no j3 e no cone)
  const glm::vec3 arm2Off(-0.1f, -0.1f, -0.05f);

  TransformPtr trfArm2 = Transform::Make();
  trfArm2->Translate(c2.x + arm2Off.x, c2.y + arm2Off.y, arm2Off.z);
  trfArm2->Rotate(ang2, 0.0f, 0.0f, 1.0f);
  trfArm2->Scale(armR, L2, armR);
  NodePtr arm2 = Node::Make(trfArm2, {matArm, tex_white}, {cylinder});

  // ----- cabeça (cone) — ÁPICE em j3 -----
  // ponta da 2ª haste (com o mesmo offset aplicado)
  const glm::vec2 j3_xy = (j2 + d2 * L2) + glm::vec2(arm2Off.x, arm2Off.y);
  const float     j3z   = arm2Off.z;

  // Ângulo pedido: atan2(vx, -vy)
  const glm::vec2 v = headTargetXY - j3_xy;
  const float angHead = glm::degrees(atan2f(v.x, -v.y));
  const float angRad  = glm::radians(angHead);

  // +Y local do cone após rotação de Z
  const glm::vec2 yLocal(-sinf(angRad), cosf(angRad));

  // Se o mesh tem ÁPICE em +0.5*H no +Y local, o centro vai para o lado oposto:
  // centro = j3 - yLocal * 0.5*H  (assim o ápice coincide com j3)
  const glm::vec2 cH_xy = j3_xy - yLocal * (0.5f * headH);

  // Pivot do cone (Translate+Rotate) e geometria (Scale+mesh)
  TransformPtr trfHeadPivot = Transform::Make();
  trfHeadPivot->Translate(cH_xy.x-0.03f, cH_xy.y+0.03f, j3z);
  trfHeadPivot->Rotate(angHead, 0.0f, 0.0f, 1.0f);
  NodePtr headPivot = Node::Make(
      trfHeadPivot,
      std::initializer_list<AppearancePtr>{},
      std::initializer_list<NodePtr>{}
  );

  TransformPtr trfHeadGeom = Transform::Make();
  trfHeadGeom->Scale(headR, headH, headR);
  AppearancePtr matHead = Material::Make(0.05f, 0.10f, 0.30f);
  std::static_pointer_cast<Material>(matHead)->SetShininess(32.0f);
  NodePtr headGeom = Node::Make(trfHeadGeom, {matHead, tex_white}, {cone});
  headPivot->AddNode(headGeom);

  // ----- luz opcional acoplada ao cabeçote -----
  if (attachLight) {
    attachLight->SetReference(headPivot);
    // posição local na "boca" do cone (+Y local), w=1 => posicional/spot
    attachLight->SetPosition(0.0f, +0.5f * headH, 0.0f, 1.0f);
  }

  // ----- monta grupo final -----
  return Node::Make(trfLamp, { lampBase, arm1, arm2, headPivot });
}

// -----------------------------------------------------------------------------
// Overloads públicos
// -----------------------------------------------------------------------------
NodePtr MakeLamp(const glm::vec3& basePos,
                 float topY,
                 const glm::vec2& headTargetXY,
                 AppearancePtr mat_metal,
                 AppearancePtr mat_white,
                 AppearancePtr tex_white,
                 ShapePtr cylinder,
                 ShapePtr cone)
{
  return BuildLamp(basePos, topY, headTargetXY,
                   mat_metal, mat_white, tex_white,
                   cylinder, cone, nullptr);
}

NodePtr MakeLamp(const glm::vec3& basePos,
                 float topY,
                 ShapePtr cylinder,
                 ShapePtr cone)
{
  // defaults internos
  AppearancePtr mat_metal = Material::Make(0.75f, 0.75f, 0.78f);
  std::static_pointer_cast<Material>(mat_metal)->SetSpecular(1.0f,1.0f,1.0f);
  std::static_pointer_cast<Material>(mat_metal)->SetShininess(96.0f);

  AppearancePtr mat_white = Material::Make(1.0f,1.0f,1.0f);
  AppearancePtr tex_white = Texture::Make("decal", glm::vec3(1.0f,1.0f,1.0f));

  // alvo padrão coerente com a cena (ex.: bola em x=0.35, y=topY+0.06)
  glm::vec2 target(0.35f, topY + 0.06f);

  return BuildLamp(basePos, topY, target,
                   mat_metal, mat_white, tex_white,
                   cylinder, cone, nullptr);
}

NodePtr MakeLamp(const glm::vec3& basePos,
                 float topY,
                 ShapePtr cylinder,
                 ShapePtr cone,
                 LightPtr attachLight)
{
  // defaults internos
  AppearancePtr mat_metal = Material::Make(0.75f, 0.75f, 0.78f);
  std::static_pointer_cast<Material>(mat_metal)->SetSpecular(1.0f,1.0f,1.0f);
  std::static_pointer_cast<Material>(mat_metal)->SetShininess(96.0f);

  AppearancePtr mat_white = Material::Make(1.0f,1.0f,1.0f);
  AppearancePtr tex_white = Texture::Make("decal", glm::vec3(1.0f,1.0f,1.0f));

  glm::vec2 target(0.35f, topY + 0.06f);

  return BuildLamp(basePos, topY, target,
                   mat_metal, mat_white, tex_white,
                   cylinder, cone, attachLight);
}
