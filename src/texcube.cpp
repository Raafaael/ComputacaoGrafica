#include "texcube.h"
#include "image.h"
#include "state.h"

#ifdef _WIN32
//#define GLAD_GL_IMPLEMENTATION // Necessary for headeronly version.
#include <glad/gl.h>
#elif __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

#include <iostream>
#include <cstdlib>
#include <cstring>

TexCubePtr TexCube::Make (const std::string& varname, const std::string& filename)
{
  return TexCubePtr(new TexCube(varname,filename));
}

TexCube::TexCube (const std::string& varname, const std::string& filename)
{
  // store the sampler uniform name used in the shader (e.g., "envMap")
  m_varname = varname;
  ImagePtr img = Image::Make(filename);

  glGenTextures(1,&m_tex);
  glBindTexture(GL_TEXTURE_CUBE_MAP,m_tex);

  // subimages' dimension
  int w = img->GetWidth() / 4;
  int h = img->GetHeight() / 3;
  int x[] = {2*w,  0,  w,  w,  w,3*w};
  int y[] = {  h,  h,2*h,  0,  h,  h};
  // Swap top/bottom mapping to match atlas orientation (fix inverted floor/ceiling)
  GLenum face[] = {
    GL_TEXTURE_CUBE_MAP_POSITIVE_X,  // right
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X,  // left
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,  // bottom (was +Y)
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y,  // top    (was -Y)
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z,  // front
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,  // back
  };
  int nc = img->GetNChannels();
  unsigned char* subimg = new unsigned char[w*h*nc];
  const unsigned char* data = img->GetData();
  int fullW = img->GetWidth();
  // Extract without vertical flip to preserve original atlas orientation
  for (int i=0; i<6; ++i) {
    for (int row=0; row<h; ++row) {
      const unsigned char* src = data + ((y[i]+row)*fullW + x[i]) * nc;
      unsigned char* dst = subimg + (row*w) * nc;
      memcpy(dst, src, w*nc);
    }
    GLenum fmt = (nc==3)?GL_RGB:GL_RGBA;
    glTexImage2D(face[i],0,fmt,w,h,0,fmt,GL_UNSIGNED_BYTE,subimg);
  }
  delete [] subimg;
  glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);	
  glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);	
  glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);	
}

TexCube::~TexCube ()
{
}

unsigned int TexCube::GetTexId () const
{
  return m_tex;
}

void TexCube::Load (StatePtr st)
{
  ShaderPtr shd = st->GetShader();
  shd->ActiveTexture(m_varname.c_str());
  glBindTexture(GL_TEXTURE_CUBE_MAP,m_tex);
}

void TexCube::Unload (StatePtr st)
{
  ShaderPtr shd = st->GetShader();
  shd->DeactiveTexture();
}