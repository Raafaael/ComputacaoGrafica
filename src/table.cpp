#include "table.h"
#include "transform.h"

NodePtr MakeTable(float topY,
                  float topHX,
                  float topHZ,
                  float topH,
                  float legH,
                  float legS,
                  AppearancePtr mat_wood,
                  AppearancePtr tex_wood,
                  ShapePtr cube)
{
  // Group transform for the table (identity; caller can place it by wrapping in another node)
  TransformPtr trf_table = Transform::Make();

  // Top
  TransformPtr trf_top = Transform::Make();
  trf_top->Translate(0.0f, topY - 0.5f*topH, 0.0f);
  trf_top->Scale(2.0f*topHX, topH, 2.0f*topHZ);
  NodePtr table_top = Node::Make(trf_top, {mat_wood, tex_wood}, {cube});

  // Legs (4)
  const float legYc = 0.5f * legH; // centered on floor
  const float dx = topHX - 0.5f*legS;
  const float dz = topHZ - 0.5f*legS;

  TransformPtr trf_leg1 = Transform::Make(); trf_leg1->Translate(+dx, legYc, +dz); trf_leg1->Scale(legS, legH, legS);
  TransformPtr trf_leg2 = Transform::Make(); trf_leg2->Translate(+dx, legYc, -dz); trf_leg2->Scale(legS, legH, legS);
  TransformPtr trf_leg3 = Transform::Make(); trf_leg3->Translate(-dx, legYc, +dz); trf_leg3->Scale(legS, legH, legS);
  TransformPtr trf_leg4 = Transform::Make(); trf_leg4->Translate(-dx, legYc, -dz); trf_leg4->Scale(legS, legH, legS);

  NodePtr leg1 = Node::Make(trf_leg1, {mat_wood, tex_wood}, {cube});
  NodePtr leg2 = Node::Make(trf_leg2, {mat_wood, tex_wood}, {cube});
  NodePtr leg3 = Node::Make(trf_leg3, {mat_wood, tex_wood}, {cube});
  NodePtr leg4 = Node::Make(trf_leg4, {mat_wood, tex_wood}, {cube});

  NodePtr table = Node::Make(trf_table, { table_top, leg1, leg2, leg3, leg4 });
  return table;
}

NodePtr MakeTable(float topY,
                  AppearancePtr mat_wood,
                  AppearancePtr tex_wood,
                  ShapePtr cube)
{
  const float topHX = 0.9f;
  const float topHZ = 0.6f;
  const float topH  = 0.08f;
  const float legH  = 0.72f;
  const float legS  = 0.10f;
  return MakeTable(topY, topHX, topHZ, topH, legH, legS, mat_wood, tex_wood, cube);
}
