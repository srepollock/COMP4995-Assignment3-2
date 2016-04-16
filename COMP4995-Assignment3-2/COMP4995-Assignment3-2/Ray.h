#pragma once
#include "Basics.h"
#include "DirectX.h"
#ifndef RAY
#define RAY
// Ray class for picking
class Ray {
public:
	// Origin of the ray
	D3DXVECTOR3 _origin;
	// Direction of the ray
	D3DXVECTOR3 _direction;
};
#endif