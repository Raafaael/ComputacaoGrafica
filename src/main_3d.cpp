
#ifdef _WIN32
#define GLAD_GL_IMPLEMENTATION // Necessary for headeronly version.
#include <glad/gl.h>
#elif __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif
#include <GLFW/glfw3.h>

#include "arcball.h"
#include "scene.h"
#include "state.h"
#include "camera3d.h"
#include "material.h"
#include "texture.h"
#include "transform.h"
#include "cube.h"
#include "quad.h"
#include "sphere.h"
#include "cylinder.h"
#include "error.h"
#include "shader.h"
#include "light.h"
#include "polyoffset.h"
#include "mesh.h"
#include "cone.h"
#include "lamp.h"
#include "table.h"
#include "texcube.h"

#include <iostream>
#include <cassert>
#include <cmath>
#include <glm/glm.hpp>
#include <vector>

static float viewer_pos[3] = {2.0f, 3.5f, 4.0f};

static ScenePtr scene;
static Camera3DPtr camera;
static ArcballPtr arcball;
static ShaderPtr g_shader;
static LightPtr g_light;          // spotlight on lamp head
// Skybox
static ShaderPtr g_skyShader;
static NodePtr   g_skyNode;
static TransformPtr g_skyTrf;
static bool g_skyLockToCamera = true; // when true, skybox follows camera translation (no parallax)
static bool g_clipEnabled = false;    // desable clip by default
static bool g_clipKeepAbove = true;  // for table-plane: keep ABOVE the tabletop
static float g_topY = 1.1f;          // table top height
// Fog controls (linear fog)
static bool  g_fogEnabled = true;
static float g_fogStart   = 3.0f;
static float g_fogEnd     = 12.0f;
// Camera zoom (field of view in degrees)
static float g_fovDeg = 30.0f; // default zoomed-in
// Roughness visualization/control
static float g_roughFactor = 1.0f; // 1.0 = as-authored; >1 amplifies roughness, <1 reduces
// Spotlight controls (desk lamp)
static float g_spotCutoffDeg = 14.0f; // cone half-angle in degrees
static float g_spotExponent  = 32.0f; // focus

static void UpdateFogUniforms()
{
  if (!g_shader) return;
  g_shader->UseProgram();
  // Disable fog by collapsing the range
  if (!g_fogEnabled) {
    g_shader->SetUniform("fogStart", 1e9f);
    g_shader->SetUniform("fogEnd",   1e9f + 1.0f);
  } else {
    g_shader->SetUniform("fogStart", g_fogStart);
    g_shader->SetUniform("fogEnd",   g_fogEnd);
  }
}

static void initialize (void)
{
  // set background color: white 
  glClearColor(1.0f,1.0f,1.0f,1.0f);
  // enable depth test 
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  // enable up to 4 user clipping planes (we'll control count via uniforms)
#ifdef GL_CLIP_DISTANCE0
  glEnable(GL_CLIP_DISTANCE0);
#endif
#ifdef GL_CLIP_DISTANCE1
  glEnable(GL_CLIP_DISTANCE1);
#endif
#ifdef GL_CLIP_DISTANCE2
  glEnable(GL_CLIP_DISTANCE2);
#endif
#ifdef GL_CLIP_DISTANCE3
  glEnable(GL_CLIP_DISTANCE3);
#endif

  // create camera (arcball created after scene center is known)
  camera = Camera3D::Make(viewer_pos[0],viewer_pos[1],viewer_pos[2]);
  // Set initial field of view (zoom)
  camera->SetAngle(g_fovDeg);

  // Light that will be attached to the lamp head (use world space; shader uses "camera" and will convert)
  LightPtr light = Light::Make(-1.5f, 2.5f, 2.2f, 1.0f, "world");
  g_light = light; // keep handle for runtime tweaks
  // Initialize spotlight parameters to match defaults used in lamp helper
  g_light->SetSpotlight(g_spotCutoffDeg, g_spotExponent);

  // Materials (colors) and their glossiness
  MaterialPtr mat_white  = Material::Make(1.0f,1.0f,1.0f);
  MaterialPtr mat_beige  = Material::Make(0.90f, 0.86f, 0.80f);
  MaterialPtr mat_wood   = Material::Make(0.55f, 0.36f, 0.20f);
  MaterialPtr mat_metal  = Material::Make(0.75f, 0.75f, 0.78f);
  MaterialPtr mat_neutral= Material::Make(1.0f,1.0f,1.0f);
  MaterialPtr mat_green  = Material::Make(0.20f, 0.80f, 0.20f);
  MaterialPtr mat_orange = Material::Make(1.00f, 0.50f, 0.00f);
  // tweak gloss
  mat_beige->SetShininess(8.0f);
  mat_wood->SetShininess(16.0f);
  mat_metal->SetSpecular(1.0f,1.0f,1.0f); mat_metal->SetShininess(96.0f);
  mat_neutral->SetShininess(32.0f);
  mat_green->SetShininess(32.0f);
  mat_orange->SetShininess(24.0f);

  // Shapes
  Error::Check("before shapes");
  ShapePtr cube = Cube::Make();
  ShapePtr sphere = Sphere::Make();
  ShapePtr cylinder = Cylinder::Make(64,64,true);
  ShapePtr cone = Cone::Make(64,64,true);
  ShapePtr quad = Quad::Make();
  Error::Check("after shapes");

  // Shader with textured lighting
  ShaderPtr shader = Shader::Make(light,"camera");
  shader->AttachVertexShader("shaders/ilum_vert/vertex_texture.glsl");
  shader->AttachFragmentShader("shaders/ilum_vert/fragment_texture.glsl");
  shader->Link();
  g_shader = shader;

  // Set fog defaults (linear fog)
  shader->UseProgram();
  shader->SetUniform("fogColor", glm::vec3(1.0f, 1.0f, 1.0f)); // match background
  UpdateFogUniforms();
  // Roughness control default
  shader->SetUniform("roughFactor", g_roughFactor);
  // Initialize clipping (no planes active by default)
  shader->SetUniform("clipCount", 0);
  std::vector<glm::vec4> clipPlanes(4, glm::vec4(0,0,0,0));
  shader->SetUniform("clipPlane", clipPlanes);
  // Environment reflection strength (default)
  shader->SetUniform("envStrength", 0.30f);

  // Textures
  AppearancePtr tex_white = Texture::Make("decal", glm::vec3(1.0f,1.0f,1.0f));
  AppearancePtr tex_earth = Texture::Make("decal", "textures/earth.jpg");
  AppearancePtr tex_wood  = Texture::Make("decal", "textures/wood.jpg");
  AppearancePtr tex_sand  = Texture::Make("decal", "textures/sand_rocks.jpg");
  AppearancePtr tex_moon  = Texture::Make("decal", "textures/moon.jpg");
  AppearancePtr tex_paper = Texture::Make("decal", "images/paper.jpg");
  AppearancePtr poly_off  = PolygonOffset::Make(-1.0f, -1.0f);
  // Environment cubemap from 4x3 cross atlas
  AppearancePtr env_cube = TexCube::Make("envMap", "images/StandardCubeMap.png");
  // Roughness bound as sampler2D 'roughness'
  // Default flat roughness for objects without a specific map (sphere will override)
  AppearancePtr rough_default = Texture::Make("roughness", glm::vec3(0.8f, 0.8f, 0.8f));
  // Fully rough (matte) for objects that should have no specular/reflection
  AppearancePtr rough_full    = Texture::Make("roughness", glm::vec3(1.0f, 1.0f, 1.0f));
  // Default normal map (flat): (0.5,0.5,1.0) so that sampling does nothing where no normal map is bound
  AppearancePtr normal_default = Texture::Make("normalMap", glm::vec3(0.5f, 0.5f, 1.0f));
  // Sand rocks normal and roughness maps for the sphere
  AppearancePtr normal_sand = Texture::Make("normalMap", "textures/sand_rocks_normal.jpg");
  // If a dedicated roughness exists for sand_rocks, replace below path with it; using 'rugosidade.png' for now
  AppearancePtr rough_sand  = Texture::Make("roughness", "textures/rugosidade.png");

  // ---------- Scene Graph ----------
  // Table dimensions
  const float topY = 1.1f;
  g_topY = topY;
  // Aim the camera at the table area so arcball orbit feels natural
  camera->SetCenter(0.0f, topY, 0.0f);
  arcball = camera->CreateArcball();
  // Build table via helper (defaults for dimensions) — no specific roughness map (uses default)
  NodePtr table = MakeTable(topY, mat_wood, tex_wood, cube);

  // Luminária via helper (versão curta, com defaults internos) e com luz acoplada ao cabeçote
  NodePtr lamp = MakeLamp(
    /*basePos*/  glm::vec3(-0.55f, 0.0f, -0.10f),
    /*topY*/     topY,
    /*shapes*/   cylinder,
                 cone,
    /*light*/    light);

  // Ball on table (textured sand-rocks with normal + roughness maps)
  TransformPtr trf_ball = Transform::Make();
  trf_ball->Translate(+0.35f, topY + 0.1f, +0.1f);
  trf_ball->Scale(0.06f, 0.06f, 0.06f);
  NodePtr ball = Node::Make(trf_ball, {mat_neutral, tex_sand, normal_sand, rough_sand}, {sphere});

  // Extra cylinder near the ball on the table
  TransformPtr trf_cyl_obj = Transform::Make();
  const float cylH = 0.18f; // height after scaling
  trf_cyl_obj->Translate(-0.3f, topY + 0.5f*cylH, -0.05f);
  trf_cyl_obj->Scale(0.06f, cylH, 0.06f);
  NodePtr cyl_obj = Node::Make(trf_cyl_obj, {mat_orange, tex_white}, {cylinder});
  // Make cylinder fully matte (no specular/reflection)
  cyl_obj = Node::Make({ rough_full }, { cyl_obj });

  // Rectangular page lying on the table
  TransformPtr trf_page = Transform::Make();
  trf_page->Translate(-0.25f, topY + 0.04f, -0.1f);
  trf_page->Rotate(-90.0f, 1.0f, 0.0f, 0.0f); // lay on XZ plane (normal +Y)
  trf_page->Scale(0.21f, 0.30f, 1.0f); // paper-like size (x,z), y ignored for quad
  NodePtr page = Node::Make(trf_page, {poly_off, mat_white, tex_paper}, {quad});
  // Make paper fully matte (no specular/reflection)
  page = Node::Make({ rough_full }, { page });

  // Assemble root with shader and defaults (flat roughness, flat normal, env cube)
  NodePtr root = Node::Make(shader, { rough_default, normal_default, env_cube }, { table, lamp, ball, cyl_obj, page });
  scene = Scene::Make(root);

  // --- Skybox setup ---
  // Separate shader that samples envMap and renders a cube centered at the camera
  g_skyShader = Shader::Make(nullptr, "camera");
  g_skyShader->AttachVertexShader("shaders/skybox/vertex.glsl");
  g_skyShader->AttachFragmentShader("shaders/skybox/fragment.glsl");
  g_skyShader->Link();
  g_skyTrf = Transform::Make();
  g_skyTrf->LoadIdentity();
  g_skyTrf->Scale(1.0f, 1.0f, 1.0f);
  // Skybox node: its own shader + env cube + cube mesh
  g_skyNode = Node::Make(g_skyShader, g_skyTrf, { env_cube }, { cube });
}

static void display (GLFWwindow* win)
{ 
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear window 
  // Draw skybox first with standard settings: depth test LEQUAL, no depth write
  if (g_skyNode) {
  glDepthMask(GL_FALSE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
  // Disable user clip distances for skybox to avoid accidental clipping
#ifdef GL_CLIP_DISTANCE0
  glDisable(GL_CLIP_DISTANCE0);
#endif
#ifdef GL_CLIP_DISTANCE1
  glDisable(GL_CLIP_DISTANCE1);
#endif
#ifdef GL_CLIP_DISTANCE2
  glDisable(GL_CLIP_DISTANCE2);
#endif
#ifdef GL_CLIP_DISTANCE3
  glDisable(GL_CLIP_DISTANCE3);
#endif
    // Build skybox model transform: scale large and recenter Y; no camera translation here
    glm::mat4 V = camera->GetViewMatrix();
  g_skyTrf->LoadIdentity();
    g_skyTrf->Scale(200.0f, 200.0f, 200.0f);
  g_skyTrf->Translate(0.0f, -0.5f, 0.0f);
  // Provide custom MVP: rotation-only view when locked to camera (no parallax), full view for world-anchored
    glm::mat4 P = camera->GetProjMatrix();
    glm::mat4 VrotOnly = glm::mat4(glm::mat3(V));
    glm::mat4 SkyMVP = (g_skyLockToCamera ? (P * VrotOnly * g_skyTrf->GetMatrix())
                                          : (P * V * g_skyTrf->GetMatrix()));
  g_skyShader->UseProgram();
  g_skyShader->SetUniform("SkyMVP", SkyMVP);
    // Render only the skybox node with its own state
    StatePtr skyState = State::Make(camera);
    g_skyNode->Render(skyState);
    // Restore states
  glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
  // Restore clip distances
#ifdef GL_CLIP_DISTANCE0
  glEnable(GL_CLIP_DISTANCE0);
#endif
#ifdef GL_CLIP_DISTANCE1
  glEnable(GL_CLIP_DISTANCE1);
#endif
#ifdef GL_CLIP_DISTANCE2
  glEnable(GL_CLIP_DISTANCE2);
#endif
#ifdef GL_CLIP_DISTANCE3
  glEnable(GL_CLIP_DISTANCE3);
#endif
  }
  // Update clipping plane (horizontal at table height) in eye space
  if (g_shader) {
    // World-space plane for y = g_topY
    // Keep above: n=(0,-1,0), d=+g_topY  => -y + g_topY < 0  -> y > g_topY
    // Keep below: n=(0,+1,0), d=-g_topY  =>  y - g_topY < 0  -> y < g_topY
    glm::vec3 n_world = g_clipKeepAbove ? glm::vec3(0.0f,-1.0f,0.0f)
                                        : glm::vec3(0.0f, 1.0f,0.0f);
    float d_world = g_clipKeepAbove ? (+g_topY) : (-g_topY);
    glm::vec4 plane_world(n_world, d_world);
    // Transform plane to eye space
    glm::mat4 V = camera->GetViewMatrix();
    glm::mat4 invTransV = glm::transpose(glm::inverse(V));
    glm::vec4 plane_eye = invTransV * plane_world;
    // Set uniforms
    g_shader->UseProgram();
    // Provide world up in eye space for environment reflection gating
    glm::vec3 worldUpEye = glm::normalize(glm::mat3(V) * glm::vec3(0.0f, 1.0f, 0.0f));
    g_shader->SetUniform("worldUpEye", worldUpEye);
    g_shader->SetUniform("clipCount", g_clipEnabled ? 1 : 0);
    std::vector<glm::vec4> planes(4, glm::vec4(0.0f));
    planes[0] = plane_eye;
    g_shader->SetUniform("clipPlane", planes);
  }
  Error::Check("before render");
  scene->Render(camera);
  Error::Check("after render");
}

static void error (int code, const char* msg)
{
  printf("GLFW error %d: %s\n", code, msg);
  glfwTerminate();
  exit(0);
}

static void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_Q && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  if (key == GLFW_KEY_C && action == GLFW_PRESS) {
    g_clipEnabled = !g_clipEnabled;
    printf("Clip %s\n", g_clipEnabled ? "ON (table plane)" : "OFF");
  }
  if (key == GLFW_KEY_B && action == GLFW_PRESS) {
    g_clipKeepAbove = !g_clipKeepAbove;
    printf("Clip keep %s table\n", g_clipKeepAbove ? "ABOVE" : "BELOW");
  }
  // Roughness factor controls: K (down), L (up), R (reset)
  if ((key == GLFW_KEY_K) && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    g_roughFactor = std::max(0.10f, g_roughFactor - 0.10f);
    if (g_shader) { g_shader->UseProgram(); g_shader->SetUniform("roughFactor", g_roughFactor); }
    printf("roughFactor = %.2f\n", g_roughFactor);
  }
  if ((key == GLFW_KEY_L) && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    g_roughFactor = std::min(3.00f, g_roughFactor + 0.10f);
    if (g_shader) { g_shader->UseProgram(); g_shader->SetUniform("roughFactor", g_roughFactor); }
    printf("roughFactor = %.2f\n", g_roughFactor);
  }
  if (key == GLFW_KEY_R && action == GLFW_PRESS) {
    g_roughFactor = 1.0f;
    if (g_shader) { g_shader->UseProgram(); g_shader->SetUniform("roughFactor", g_roughFactor); }
    printf("roughFactor reset to 1.00\n");
  }
  if (key == GLFW_KEY_F && action == GLFW_PRESS) {
    g_fogEnabled = !g_fogEnabled;
    UpdateFogUniforms();
    printf("Fog %s\n", g_fogEnabled ? "ON" : "OFF");
  }
  if ((key == GLFW_KEY_KP_ADD || key == GLFW_KEY_EQUAL) && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    g_fogEnd = std::min(g_fogEnd + 0.5f, 50.0f);
    UpdateFogUniforms();
    printf("Fog range: [%.2f, %.2f]\n", g_fogStart, g_fogEnd);
  }
  if ((key == GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_MINUS) && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
    g_fogEnd = std::max(g_fogEnd - 0.5f, g_fogStart + 0.5f);
    UpdateFogUniforms();
    printf("Fog range: [%.2f, %.2f]\n", g_fogStart, g_fogEnd);
  }
  if (key == GLFW_KEY_V && action == GLFW_PRESS) {
    g_skyLockToCamera = !g_skyLockToCamera;
    printf("Skybox parallax %s\n", g_skyLockToCamera ? "OFF (locked to camera)" : "ON (world-anchored)");
  }
}

static void resize (GLFWwindow* win, int width, int height)
{
  glViewport(0,0,width,height);
}

static void cursorpos (GLFWwindow* win, double x, double y)
{
  // convert screen pos (upside down) to framebuffer pos (e.g., retina displays)
  int wn_w, wn_h, fb_w, fb_h;
  glfwGetWindowSize(win, &wn_w, &wn_h);
  glfwGetFramebufferSize(win, &fb_w, &fb_h);
  x = x * fb_w / wn_w;
  y = (wn_h - y) * fb_h / wn_h;
  arcball->AccumulateMouseMotion(int(x),int(y));
}
static void cursorinit (GLFWwindow* win, double x, double y)
{
  // convert screen pos (upside down) to framebuffer pos (e.g., retina displays)
  int wn_w, wn_h, fb_w, fb_h;
  glfwGetWindowSize(win, &wn_w, &wn_h);
  glfwGetFramebufferSize(win, &fb_w, &fb_h);
  x = x * fb_w / wn_w;
  y = (wn_h - y) * fb_h / wn_h;
  arcball->InitMouseMotion(int(x),int(y));
  glfwSetCursorPosCallback(win, cursorpos);     // cursor position callback
}
static void mousebutton (GLFWwindow* win, int button, int action, int mods)
{
  if (action == GLFW_PRESS) {
    glfwSetCursorPosCallback(win, cursorinit);     // cursor position callback
  }
  else // GLFW_RELEASE 
    glfwSetCursorPosCallback(win, nullptr);      // callback disabled
}

// Mouse scroll: adjust camera FOV (zoom). Scroll up -> zoom in (smaller FOV).
static void scroll (GLFWwindow* win, double xoffset, double yoffset)
{
  // Each notch changes FOV by 2 degrees (tweak as desired)
  g_fovDeg = std::max(15.0f, std::min(90.0f, g_fovDeg - float(yoffset) * 2.0f));
  if (camera) {
    camera->SetAngle(g_fovDeg);
  }
}

int main ()
{
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);
  glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);       // required for mac os
  glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER,GLFW_TRUE);  // option for mac os
#endif

  glfwSetErrorCallback(error);

  GLFWwindow* win = glfwCreateWindow(1000,800,"Simple Scene",nullptr,nullptr);
  assert(win);
  glfwSetFramebufferSizeCallback(win, resize);  // resize callback
  glfwSetKeyCallback(win, keyboard);            // keyboard callback
  glfwSetMouseButtonCallback(win, mousebutton); // mouse button callback
  glfwSetScrollCallback(win, scroll);           // mouse wheel zoom callback
  
  glfwMakeContextCurrent(win);
#ifdef _WIN32
  if (!gladLoadGL(glfwGetProcAddress)) {
      printf("Failed to initialize GLAD OpenGL context\n");
      exit(1);
  }
#elif !defined(__APPLE__)
  // Initialize GLEW on Linux/WSL
  glewExperimental = GL_TRUE; // ensure core profiles get function pointers
  if (glewInit() != GLEW_OK) {
      printf("Failed to initialize GLEW\n");
      exit(1);
  }
  // glewInit can generate GL_INVALID_ENUM; clear it
  while (glGetError() != GL_NO_ERROR) {}
#endif
  printf("OpenGL version: %s\n", glGetString(GL_VERSION));

  initialize();

  while(!glfwWindowShouldClose(win)) {
    display(win);
    glfwSwapBuffers(win);
    glfwPollEvents();
  }
  glfwTerminate();
  return 0;
}

