#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <iostream>
#include <unistd.h>
int main(){char b[4096];getcwd(b,sizeof(b));std::cout<<"CWD="<<b<<"\n";int w,h,c;auto*d=stbi_load("textures/sun.jpg",&w,&h,&c,0);if(!d){std::cerr<<"FAIL"<<std::endl;return 1;}std::cout<<"OK "<<w<<"x"<<h<<" c="<<c<<"\n";stbi_image_free(d);}