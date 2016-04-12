#pragma once
#include "Basics.h"
#include "DirectX.h"
#ifndef RAY
#define RAY
class Ray {
public:
	D3DXVECTOR3 _origin;
	D3DXVECTOR3 _direction;
};
#endif