
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
static bool g_clipEnabled = false;    // desable clip by default
static bool g_clipKeepAbove = true;  // for table-plane: keep ABOVE the tabletop
static float g_topY = 1.1f;          // table top height

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

  // Light that will be attached to the lamp head (use world space; shader uses "camera" and will convert)
  LightPtr light = Light::Make(-1.5f, 2.5f, 2.2f, 1.0f, "world");

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
  shader->SetUniform("fogStart", 3.0f);
  shader->SetUniform("fogEnd",   8.0f);
  // Initialize clipping (no planes active by default)
  shader->SetUniform("clipCount", 0);
  std::vector<glm::vec4> clipPlanes(4, glm::vec4(0,0,0,0));
  shader->SetUniform("clipPlane", clipPlanes);

  // Textures
  AppearancePtr tex_white = Texture::Make("decal", glm::vec3(1.0f,1.0f,1.0f));
  AppearancePtr tex_earth = Texture::Make("decal", "textures/earth.jpg");
  AppearancePtr tex_wood  = Texture::Make("decal", "textures/wood.jpg");
  AppearancePtr tex_moon  = Texture::Make("decal", "textures/moon.jpg");
  AppearancePtr tex_paper = Texture::Make("decal", "images/paper.jpg");
  AppearancePtr poly_off  = PolygonOffset::Make(-1.0f, -1.0f);
  // Roughness maps (1x1) bound as sampler2D 'roughness'
  AppearancePtr rough_default = Texture::Make("roughness", glm::vec3(0.8f, 0.8f, 0.8f)); // fairly rough
  AppearancePtr rough_metal   = Texture::Make("roughness", glm::vec3(0.2f, 0.2f, 0.2f)); // glossy metal
  AppearancePtr rough_wood    = Texture::Make("roughness", glm::vec3(0.6f, 0.6f, 0.6f)); // semi-rough wood

  // ---------- Scene Graph ----------
  // Table dimensions
  const float topY = 1.1f;
  g_topY = topY;
  // Aim the camera at the table area so arcball orbit feels natural
  camera->SetCenter(0.0f, topY, 0.0f);
  arcball = camera->CreateArcball();
  // Build table via helper (defaults for dimensions) and wrap with wood roughness
  NodePtr table = MakeTable(topY, mat_wood, tex_wood, cube);
  NodePtr table_wrapped = Node::Make({ rough_wood }, { table });

  // Luminária via helper (versão curta, com defaults internos) e com luz acoplada ao cabeçote
  NodePtr lamp = MakeLamp(
    /*basePos*/  glm::vec3(-0.55f, 0.0f, -0.10f),
    /*topY*/     topY,
    /*shapes*/   cylinder,
                 cone,
    /*light*/    light);
  // Wrap lamp with glossy metal roughness
  NodePtr lamp_wrapped = Node::Make({ rough_metal }, { lamp });

  // Ball on table (menor que o cilindro) — cor verde
  TransformPtr trf_ball = Transform::Make();
  trf_ball->Translate(+0.35f, topY + 0.1f, -0.20f);
  trf_ball->Scale(0.06f, 0.06f, 0.06f);
  NodePtr ball = Node::Make(trf_ball, {mat_green, tex_white}, {sphere});

  // Extra cylinder near the ball on the table
  TransformPtr trf_cyl_obj = Transform::Make();
  const float cylH = 0.18f; // height after scaling
  trf_cyl_obj->Translate(+0.10f, topY + 0.5f*cylH, -0.05f);
  trf_cyl_obj->Scale(0.06f, cylH, 0.06f);
  NodePtr cyl_obj = Node::Make(trf_cyl_obj, {mat_orange, tex_white}, {cylinder});

  // Rectangular page lying on the table
  TransformPtr trf_page = Transform::Make();
  trf_page->Translate(-0.05f, topY + 0.04f, -0.15f);
  trf_page->Rotate(-90.0f, 1.0f, 0.0f, 0.0f); // lay on XZ plane (normal +Y)
  trf_page->Scale(0.21f, 0.30f, 1.0f); // paper-like size (x,z), y ignored for quad
  NodePtr page = Node::Make(trf_page, {poly_off, mat_white, tex_paper}, {quad});

  // Assemble root with shader and a default roughness (fallback)
  NodePtr root = Node::Make(shader, { rough_default }, { table_wrapped, lamp_wrapped, ball, cyl_obj, page });
  scene = Scene::Make(root);
}

static void display (GLFWwindow* win)
{ 
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear window 
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

