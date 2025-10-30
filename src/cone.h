#include <memory>
class Cone;
using ConePtr = std::shared_ptr<Cone>;

#ifndef CONE_H
#define CONE_H

#include "shape.h"
#include "disk.h"

class Cone : public Shape {
  unsigned int m_vao;
  unsigned int m_nind; // number of indices (side)
  DiskPtr m_basedisk;  // optional base cap
protected:
  Cone (int nstack, int nslice, bool cap);
public:
  static ConePtr Make (int nstack=64, int nslice=64, bool cap=true);
  virtual ~Cone ();
  virtual void Draw (StatePtr st);
};

#endif
