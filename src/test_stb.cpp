#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <iostream>
#include <unistd.h>
int main(){
  char buf[4096];
  getcwd(buf,sizeof(buf));
  std::cout<<"CWD="<<buf<<"\n"; 
  int w,h,c; unsigned char* d = stbi_load("./textures/sun.jpg",&w,&h,&c,0);
  if(!d){ std::cerr<<"FAIL: sun.jpg\n"; return 1; }
  std::cout<<"Loaded sun.jpg "<<w<<"x"<<h<<" channels="<<c<<"\n"; stbi_image_free(d); return 0; }
