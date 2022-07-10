#ifndef RENDERUTILS_H
#define RENDERUTILS_H
#include "mathlib/vector.h"

void DebugDrawBox(const Vector3D& vOrigin, const QAngle& vAngles, const Vector3D& vMins, const Vector3D& vMaxs, Color color, bool bZBuffer);
void DebugDrawCylinder(const Vector3D& vOrigin, const QAngle& vAngles, float flRadius, float flHeight, Color color, int nSides = 16, bool bZBuffer = false);
void DebugDrawSphere(const Vector3D& vOrigin, float flRadius, Color color, int nSegments = 16, bool bZBuffer = false);
void DebugDrawCircle(const Vector3D& vOrigin, const QAngle& vAngles, float flRadius, Color color, int nSegments = 16, bool bZBuffer = false);
void DebugDrawSquare(const Vector3D& vOrigin, const QAngle& vAngles, float flSquareSize, Color color, bool bZBuffer = false);
void DebugDrawTriangle(const Vector3D& vOrigin, const QAngle& vAngles, float flTriangleSize, Color color, bool bZBuffer = false);
void DebugDrawMark(const Vector3D& vOrigin, float flRadius, const vector<int>& vColor, bool bZBuffer = false);
void DrawStar(const Vector3D& vRrigin, float flRadius, bool bZBuffer = false);
void DebugDrawArrow(const Vector3D& vOrigin, const Vector3D& vEnd, float flArraySize, Color color, bool bZBuffer = false);
void DebugDrawAxis(const Vector3D& vOrigin, const QAngle& vAngles = { 0, 0, 0 }, float flScale = 50.f, bool bZBuffer = false);
void DebugDrawCapsule(const Vector3D& vStart, const Vector3D& vEnd, const float& flRadius, Color color, bool bZBuffer = false);
#endif // RENDERUTILS_H
