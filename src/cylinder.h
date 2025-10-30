#include <memory>
class Cylinder;
using CylinderPtr = std::shared_ptr<Cylinder>;

#ifndef CYLINDER_H
#define CYLINDER_H

#include "shape.h"
#include "disk.h"

class Cylinder : public Shape {
  unsigned int m_vao;
  unsigned int m_nind; // number of indices
  // optional caps as separate Disk shapes (drawn with local transforms)
  DiskPtr m_topdisk;
  DiskPtr m_botdisk;
protected:
  Cylinder (int nstack, int nslice, bool caps);
public:
  static CylinderPtr Make (int nstack=64, int nslice=64, bool caps=false);
  virtual ~Cylinder ();
  virtual void Draw (StatePtr st);
};

#endif
