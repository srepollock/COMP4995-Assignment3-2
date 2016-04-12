#pragma once
#include "DirectX.h"
#ifndef BOUNDING_SPHERE
#define BOUNDING_SPHERE
class BoundingSphere{
public:
	BoundingSphere();
	~BoundingSphere();

	D3DXVECTOR3 _center;
	float       _radius;
};
#endif