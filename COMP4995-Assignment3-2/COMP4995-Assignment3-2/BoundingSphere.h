#pragma once
#include "DirectX.h"
#ifndef BOUNDING_SPHERE
#define BOUNDING_SPHERE
// Class for the bounding sphere object
class BoundingSphere{
public:
	// Constructor
	BoundingSphere();
	// Destructor
	~BoundingSphere();
	// Center of the bounding sphere
	D3DXVECTOR3 _center;
	// Radius of the bounding sphere
	float       _radius;
};
#endif