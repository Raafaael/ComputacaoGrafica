#include "disk.h"
#include "error.h"

#include <cmath>

#ifdef _WIN32
#define GLAD_GL_IMPLEMENTATION // Necessary for headeronly version.
#include <glad/gl.h>
#elif __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

DiskPtr Disk::Make (int nslice)
{
  return DiskPtr(new Disk(nslice));
}

Disk::Disk (int nslice)
: m_vao(0), m_nslice(nslice < 3 ? 3 : nslice)
{
  // Create positions (x,y) for a unit disk centered at origin
  // plus texcoords (u,v) mapped polar to [0,1]
  const int nverts = m_nslice + 2; // center + ring (closing vertex)
  float* coord = new float[2 * nverts];
  float* tex   = new float[2 * nverts];

  // center vertex
  coord[0] = 0.0f; coord[1] = 0.0f;
  tex[0] = 0.5f;   tex[1] = 0.5f;

  const float twoPi = 6.28318530718f;
  for (int i = 0; i <= m_nslice; ++i) {
    float t = static_cast<float>(i) / static_cast<float>(m_nslice);
    float ang = t * twoPi;
    float x = std::cos(ang);
    float y = std::sin(ang);
    // position
    coord[2*(i+1) + 0] = x;
    coord[2*(i+1) + 1] = y;
    // texture (polar mapping)
    tex[2*(i+1) + 0] = 0.5f + 0.5f * x;
    tex[2*(i+1) + 1] = 0.5f + 0.5f * y;
  }

  // create VAO
  glGenVertexArrays(1,&m_vao);
  glBindVertexArray(m_vao);

  // create buffers: coord & texcoord
  GLuint id[2];
  glGenBuffers(2,id);

  glBindBuffer(GL_ARRAY_BUFFER,id[0]);
  glBufferData(GL_ARRAY_BUFFER,2*nverts*sizeof(float),coord,GL_STATIC_DRAW);
  glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,0);  // coord (location 0)
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER,id[1]);
  glBufferData(GL_ARRAY_BUFFER,2*nverts*sizeof(float),tex,GL_STATIC_DRAW);
  glVertexAttribPointer(3,2,GL_FLOAT,GL_FALSE,0,0);  // texcoord (location 3)
  glEnableVertexAttribArray(3);

  delete [] coord;
  delete [] tex;
}

Disk::~Disk ()
{
}

void Disk::Draw (StatePtr)
{
  glBindVertexArray(m_vao);
  // constant normal (0,0,1) and tangent (1,0,0) for 2D lighting/consistency
  glVertexAttrib3f(1,0.0f,0.0f,1.0f); // normal
  glVertexAttrib3f(2,1.0f,0.0f,0.0f); // tangent
  glDrawArrays(GL_TRIANGLE_FAN, 0, m_nslice + 2);
}
