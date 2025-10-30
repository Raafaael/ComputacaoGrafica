#include "cylinder.h"
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

CylinderPtr Cylinder::Make (int nstack, int nslice, bool caps)
{
  return CylinderPtr(new Cylinder(nstack,nslice,caps));
}

Cylinder::Cylinder (int nstack, int nslice, bool caps)
{
  (void)caps; 
  GridPtr grid = Grid::Make(nstack,nslice);
  int vcount_side = grid->VertexCount();

  std::vector<float> coord(3*vcount_side);
  std::vector<float> normal(3*vcount_side);

  const float* texcoord_side = grid->GetCoords();
  for (int vi=0; vi<vcount_side; ++vi) {
    float u = texcoord_side[2*vi+0];
    float v = texcoord_side[2*vi+1];
    float theta = u * 2.0f * PI;
    float y = v - 0.5f;
    float s = sinf(theta);
    float c = cosf(theta);
    coord[3*vi+0] = s; // x
    coord[3*vi+1] = y; // y
    coord[3*vi+2] = c; // z
    normal[3*vi+0] = s;
    normal[3*vi+1] = 0.0f;
    normal[3*vi+2] = c;
  }

  // Use grid indices directly
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
  // use texcoords directly from Grid (matches friend's example style)
  glBufferData(GL_ARRAY_BUFFER,GLsizeiptr(2*vcount_side*sizeof(float)),texcoord_side,GL_STATIC_DRAW);
  glVertexAttribPointer(3,2,GL_FLOAT,GL_FALSE,0,0);
  glEnableVertexAttribArray(3);

  // create index buffer
  GLuint index;
  glGenBuffers(1,&index);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,index);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,GLsizeiptr(m_nind*sizeof(unsigned int)),grid->GetIndices(),GL_STATIC_DRAW);

  m_topdisk = Disk::Make(nstack);
  m_botdisk = Disk::Make(nstack);
}

Cylinder::~Cylinder ()
{
}

void Cylinder::Draw (StatePtr st)
{
  glBindVertexArray(m_vao);
  glDrawElements(GL_TRIANGLES,m_nind,GL_UNSIGNED_INT,0);
  // top cap (+Y)
  st->PushMatrix();
  glm::mat4 Mtop = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.0f)) * glm::rotate(glm::mat4(1.0f), -PI*0.5f, glm::vec3(1.0f,0.0f,0.0f));
  st->MultMatrix(Mtop);
  st->LoadMatrices();
  m_topdisk->Draw(st);
  st->PopMatrix();
  // bottom cap (-Y)
  st->PushMatrix();
  glm::mat4 Mbot = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f,-0.5f, 0.0f)) * glm::rotate(glm::mat4(1.0f),  PI*0.5f, glm::vec3(1.0f,0.0f,0.0f));
  st->MultMatrix(Mbot);
  st->LoadMatrices();
  m_botdisk->Draw(st);
  st->PopMatrix();
}
