#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>

Image::Image (const std::string& filename)
{
  //stbi_set_flip_vertically_on_load(1);
  std::string fname = filename;
  m_data = stbi_load(fname.c_str(),&m_width,&m_height,&m_nchannels,0);
  if (!m_data) {
    // Fallback: read whole file into memory and try stbi_load_from_memory (debugging WSL path oddities)
    std::ifstream fin(fname, std::ios::binary);
    if (fin.good()) {
      std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(fin)), {});
      if (!bytes.empty()) {
        int w=0,h=0,nc=0;
        unsigned char* tmp = stbi_load_from_memory(bytes.data(), (int)bytes.size(), &w,&h,&nc,0);
        if (tmp) {
          m_data = tmp; m_width=w; m_height=h; m_nchannels=nc;
        }
      }
    }
    // attempt fallback by prefixing Trabalho1/ if not already present
    if (fname.find("Trabalho1/") == std::string::npos) {
      std::string alt = std::string("Trabalho1/") + fname;
      m_data = stbi_load(alt.c_str(),&m_width,&m_height,&m_nchannels,0);
    }
  }
  if (!m_data) {
    std::cerr << "Could not load image: " << fname << std::endl;
    exit(1);
  }
}

ImagePtr Image::Make (const std::string& filename)
{
  return ImagePtr(new Image(filename));
}

Image::~Image ()
{
  stbi_image_free(m_data);
}

const unsigned char* Image::GetData () const
{
  return m_data;
}

int Image::GetWidth () const
{
  return m_width;
}

int Image::GetHeight () const
{
  return m_height;
}

int Image::GetNChannels () const
{
  return m_nchannels;
}

void Image::ExtractSubimage (int x, int y, int w, int h, unsigned char* img)
{
  for (int i=0; i<h; ++i) {
    memcpy(img+((h-i-1)*w)*m_nchannels,m_data+((y+i)*m_width+x)*m_nchannels,w*m_nchannels); 
  }
}