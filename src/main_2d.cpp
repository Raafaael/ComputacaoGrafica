#ifdef _WIN32
#define GLAD_GL_IMPLEMENTATION // Necessary for headeronly version.
#include <glad/gl.h>
#elif __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif
#include <GLFW/glfw3.h>

#include "scene.h"
#include "state.h"
#include "camera2d.h"
#include "color.h"
#include "transform.h"
#include "error.h"
#include "shader.h"
#include "disk.h"
#include "quad.h"
#include "texture.h"
#include "solar_engine.h"

#include <iostream>

static ScenePtr scene;
static CameraPtr camera;

// ===================== Configuração =====================
namespace Config {
  // Velocidades (deg/s) ajustadas para movimento mais suave
  constexpr float EarthOrbitSpeed = 15.0f;   
  constexpr float EarthSpinSpeed  = 45.0f;   
  constexpr float MoonOrbitSpeed  = 60.0f;   
  constexpr float MercuryOrbitSpeed = 70.0f; 
  constexpr float SunX = 5.0f;
  constexpr float SunY = 5.0f;
  // Raio das órbitas (distâncias de translação)
  constexpr float EarthRadius   = 3.7f;   // mais longe para abrir espaço ao Mercúrio
  constexpr float MercuryRadius = 2.0f;   // órbita interna
  constexpr float MoonRadius    = 0.9f;   // distância da lua à Terra
}

struct SolarSystemNodes {
  TransformPtr earthOrbit;   // Terra translação em torno do sol
  TransformPtr earthSpin;    // Terra rotação em torno do próprio eixo
  TransformPtr moonOrbit;    // Órbita da lua em torno da Terra
  TransformPtr mercuryOrbit; // Órbita de Mercúrio/Vênus
};

// Cria shader 2D simples
static ShaderPtr CreateColorShader2D() {
  auto shader = Shader::Make();
  shader->AttachVertexShader("shaders/2d/vertex.glsl");
  shader->AttachFragmentShader("shaders/2d/fragment.glsl");
  shader->Link();
  return shader;
}

static ShaderPtr CreateTexShader2D() {
  auto shader = Shader::Make();
  shader->AttachVertexShader("shaders/2d/vertex_tex.glsl");
  shader->AttachFragmentShader("shaders/2d/fragment_tex.glsl");
  shader->Link();
  return shader;
}

// Constrói a cena do sistema solar e retorna também os transforms relevantes
static std::pair<ScenePtr,SolarSystemNodes> BuildSolarSystem(ShaderPtr shdColor, ShaderPtr shdTex) {
  SolarSystemNodes nodes{};

  // Sun
  auto trfSun = Transform::Make();
  trfSun->Translate(Config::SunX,Config::SunY,0.0f);
  auto trfSunScale = Transform::Make();
  trfSunScale->Scale(1.3f,1.3f,1.0f);
  // Textured sun (optional color tint)
  auto sunTex = Texture::Make("decal","textures/sun.jpg");
  auto sunGeom = Node::Make(shdTex, trfSunScale, {Color::Make(1.0f,0.95f,0.3f), sunTex},{Disk::Make(64)});
  auto sun = Node::Make(trfSun,{sunGeom});

  // Earth orbit root (rotation center at sun)
  nodes.earthOrbit = Transform::Make();
  auto earthOrbit = Node::Make(nodes.earthOrbit, std::initializer_list<NodePtr>{});

  // Earth translation to orbit radius
  auto trfEarthTranslate = Transform::Make();
  trfEarthTranslate->Translate(Config::EarthRadius,0.0f,0.0f);
  auto earthTranslate = Node::Make(trfEarthTranslate, std::initializer_list<NodePtr>{});

  // Earth spin
  nodes.earthSpin = Transform::Make();
  auto earthSpin = Node::Make(nodes.earthSpin, std::initializer_list<NodePtr>{});

  // Earth geometry
  auto trfEarthGeom = Transform::Make();
  trfEarthGeom->Scale(0.6f,0.6f,1.0f);
  auto earthTex = Texture::Make("decal","textures/earth.jpg");
  auto earthGeom = Node::Make(shdTex, trfEarthGeom, {Color::Make(1.0f,1.0f,1.0f), earthTex},{Disk::Make(48)});
  earthSpin->AddNode(earthGeom);
  earthTranslate->AddNode(earthSpin);
  earthOrbit->AddNode(earthTranslate);
  sun->AddNode(earthOrbit);

  // Moon orbit
  nodes.moonOrbit = Transform::Make();
  auto moonOrbit = Node::Make(nodes.moonOrbit, std::initializer_list<NodePtr>{});
  auto trfMoon = Transform::Make();
  trfMoon->Translate(Config::MoonRadius,0.0f,0.0f);
  trfMoon->Scale(0.25f,0.25f,1.0f);
  auto moonTex = Texture::Make("decal","textures/moon.jpg");
  auto moon = Node::Make(shdTex, trfMoon,{Color::Make(1.0f,1.0f,1.0f), moonTex},{Disk::Make(36)});
  moonOrbit->AddNode(moon);
  earthTranslate->AddNode(moonOrbit);

  // Mercury - órbita mais interna
  nodes.mercuryOrbit = Transform::Make();
  auto mercuryOrbit = Node::Make(nodes.mercuryOrbit, std::initializer_list<NodePtr>{});
  auto trfMercuryTranslate = Transform::Make();
  trfMercuryTranslate->Translate(Config::MercuryRadius, 0.0f, 0.0f); // raio menor fixo
  auto mercuryTex = Texture::Make("decal","textures/mercury.jpg");
  auto trfMercuryScale = Transform::Make();
  trfMercuryScale->Scale(0.35f,0.35f,1.0f);
  auto mercuryGeom = Node::Make(shdTex, trfMercuryScale,{Color::Make(1.0f,1.0f,1.0f), mercuryTex},{Disk::Make(48)});
  auto mercuryTranslate = Node::Make(trfMercuryTranslate,{mercuryGeom});
  mercuryOrbit->AddNode(mercuryTranslate);
  sun->AddNode(mercuryOrbit);

  // Fundo estrelado (quad cobrindo viewport 0..10) - desenhar antes (add como primeiro filho da root)
  auto trfBg = Transform::Make();
  trfBg->Scale(10.0f,10.0f,1.0f); // grid 0..1 -> 0..10
  trfBg->Translate(0.0f,0.0f,-0.5f); // empurra para trás no eixo Z para não bloquear discos
  auto bgTex = Texture::Make("decal","textures/stars.jpg");
  auto bgQuad = Node::Make(shdTex, trfBg, {Color::Make(1.0f,1.0f,1.0f), bgTex}, {Quad::Make()});

  auto root = Node::Make(shdColor, std::initializer_list<NodePtr>{bgQuad,sun});
  auto sc = Scene::Make(root);
  return {sc, nodes};
}

static void initialize () {
  glClearColor(0.0f,0.0f,0.0f,1.0f);
  glEnable(GL_DEPTH_TEST);
  camera = Camera2D::Make(0,10,0,10);
  auto shdColor = CreateColorShader2D();
  auto shdTex = CreateTexShader2D();
  auto [sc, nodes] = BuildSolarSystem(shdColor, shdTex);
  scene = sc;
  // Anexa engine
  scene->AddEngine(std::make_shared<SolarEngine>(nodes.earthOrbit, nodes.earthSpin, nodes.moonOrbit, nodes.mercuryOrbit));
}

static void display (GLFWwindow* win)
{ 
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear window 
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
}

static void resize (GLFWwindow* win, int width, int height)
{
  glViewport(0,0,width,height);
}

static void update (float dt)
{
  scene->Update(dt);
}

int main ()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);       // required for mac os
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);  // option for mac os
#endif

    glfwSetErrorCallback(error);

  GLFWwindow* win = glfwCreateWindow(800, 800, "Sistema Solar", nullptr, nullptr);
    assert(win);
    glfwSetFramebufferSizeCallback(win, resize);  // resize callback
    glfwSetKeyCallback(win, keyboard);            // keyboard callback

    glfwMakeContextCurrent(win);
#ifdef _WIN32
  if (!gladLoadGL(glfwGetProcAddress)) {
    printf("Failed to initialize GLAD OpenGL context\n");
    exit(1);
  }
#elif !defined(__APPLE__)
  if (glewInit() != GLEW_OK) {
    printf("Failed to initialize GLEW\n");
    exit(1);
  }
#endif
    printf("OpenGL version: %s\n", glGetString(GL_VERSION));

  initialize();

  float t0 = float(glfwGetTime());
  while(!glfwWindowShouldClose(win)) {
    float t = float(glfwGetTime());
    update(t-t0);
    t0 = t;
    display(win);
    glfwSwapBuffers(win);
    glfwPollEvents();
  }
  glfwTerminate();
  return 0;
}

