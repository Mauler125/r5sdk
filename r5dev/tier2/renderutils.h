#ifndef RENDERUTILS_H
#define RENDERUTILS_H
#include "mathlib/vector.h"

Vector3D* GetBoxCorners(Vector3D org, QAngle ang, Vector3D mins, Vector3D maxs);
void DrawAngledBox(Vector3D org, QAngle ang, Vector3D mins, Vector3D maxs, int r, int g, int b, int a, bool throughSolid);

void RenderCapsule(const Vector3D& vStart, const Vector3D& vEnd, const float& flRadius, Color c);

#endif // RENDERUTILS_H
