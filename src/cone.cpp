#include "cone.h"
#include "grid.h"

#include <cmath>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>

#ifdef _WIN32
#include <glad/gl.h>
#elif __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

#define PI 3.14159265f

ConePtr Cone::Make (int nstack, int nslice, bool cap)
{
  return ConePtr(new Cone(nstack,nslice,cap));
}

Cone::Cone (int nstack, int nslice, bool cap)
{
  GridPtr grid = Grid::Make(nstack,nslice);
  int vcount = grid->VertexCount();

  std::vector<float> coord(3*vcount);
  std::vector<float> normal(3*vcount);

  const float* texcoord = grid->GetCoords();
  for (int vi=0; vi<vcount; ++vi) {
    float u = texcoord[2*vi+0];
    float v = texcoord[2*vi+1];
    float theta = u * 2.0f * PI;
    float y = v - 0.5f;     // from -0.5 (base) to +0.5 (apex)
    float r = 1.0f - v;     // radius decreases linearly to apex
    float s = sinf(theta);
    float c = cosf(theta);
    coord[3*vi+0] = r * s;  // x
    coord[3*vi+1] = y;      // y
    coord[3*vi+2] = r * c;  // z
    // side normal for a right cone (H=1, R=1): n ~ (s,1,c)
    float nx = s;
    float ny = 1.0f;
    float nz = c;
    float invlen = 1.0f / sqrtf(nx*nx + ny*ny + nz*nz);
    normal[3*vi+0] = nx * invlen;
    normal[3*vi+1] = ny * invlen;
    normal[3*vi+2] = nz * invlen;
  }

  m_nind = (unsigned int)grid->IndexCount();

  // create VAO
  glGenVertexArrays(1,&m_vao);
  glBindVertexArray(m_vao);

  // create buffers: coord, normal, texcoord
  GLuint id[3];
  glGenBuffers(3,id);

  glBindBuffer(GL_ARRAY_BUFFER,id[0]);
  glBufferData(GL_ARRAY_BUFFER,GLsizeiptr(coord.size()*sizeof(float)),coord.data(),GL_STATIC_DRAW);
  glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER,id[1]);
  glBufferData(GL_ARRAY_BUFFER,GLsizeiptr(normal.size()*sizeof(float)),normal.data(),GL_STATIC_DRAW);
  glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,0,0);
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER,id[2]);
  glBufferData(GL_ARRAY_BUFFER,GLsizeiptr(2*vcount*sizeof(float)),texcoord,GL_STATIC_DRAW);
  glVertexAttribPointer(3,2,GL_FLOAT,GL_FALSE,0,0);
  glEnableVertexAttribArray(3);

  // create index buffer
  GLuint index;
  glGenBuffers(1,&index);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,index);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,GLsizeiptr(m_nind*sizeof(unsigned int)),grid->GetIndices(),GL_STATIC_DRAW);

  m_basedisk = cap ? Disk::Make(nstack) : nullptr;
}

Cone::~Cone ()
{
}

void Cone::Draw (StatePtr st)
{
  glBindVertexArray(m_vao);
  glDrawElements(GL_TRIANGLES,m_nind,GL_UNSIGNED_INT,0);
  if (m_basedisk) {
    // base cap at y=-0.5 (normal -Y)
    st->PushMatrix();
    glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f,-0.5f,0.0f))
                * glm::rotate(glm::mat4(1.0f),  PI*0.5f, glm::vec3(1.0f,0.0f,0.0f));
    st->MultMatrix(M);
    st->LoadMatrices();
    m_basedisk->Draw(st);
    st->PopMatrix();
  }
}
