#ifndef __cameraH__
#define __cameraH__

#include <d3dx9.h>

class Camera {
public:
	enum CameraType { LANDOBJECT, AIRCRAFT };
	// Camera constructor
	Camera();
	// Cmaera constructor with camera type
	Camera(CameraType cameraType);
	// Camera destructor
	~Camera();
	// Strafe the camera by units
	//param units: Units to move the camera left/right by
	void strafe(float units); // left/right
	// Fly the camera by units
	//param units: Units to move the camera up/down by
	void fly(float units);    // up/down
	// Walk the camera by units
	//param units: Units to move the camera forwards/backwards by
	void walk(float units);   // forward/backward
	// Pitch
	//param angle: Angle to pitch the camera by
	void pitch(float angle); // rotate on right vector
	// Yaw
	//param angle: Angle to yaw the camera by
	void yaw(float angle);   // rotate on up vector
	// Roll
	//param angle: Angle to roll the camera by
	void roll(float angle);  // rotate on look vector
	// Gets the current view matrix of the camera
	//param V: Pointer to the view to store the current camera position
	void getViewMatrix(D3DXMATRIX* V);
	// Sets the type of the camera
	//param cameraType: Type to change the camera to
	void setCameraType(CameraType cameraType);
	// Gets the current position of the camera
	//param pos: Pointer to the position to store the current camera position in
	void getPosition(D3DXVECTOR3* pos);
	// Sets the current position of the camer
	//param pos: Pointer to the new position object
	void setPosition(D3DXVECTOR3* pos);
	// Gets looking right
	//param right: Right vector
	void getRight(D3DXVECTOR3* right);
	// Gets looking up
	//param right: Up vector
	void getUp(D3DXVECTOR3* up);
	// Gets looking vector
	//param right: Look vector
	void getLook(D3DXVECTOR3* look);
private:
	// Type of the camera
	CameraType  _cameraType;
	// Vector to the right of the camera
	D3DXVECTOR3 _right;
	// Up vector of the camer
	D3DXVECTOR3 _up;
	// Look vector of the camera
	D3DXVECTOR3 _look;
	// Position of the camera
	D3DXVECTOR3 _pos;
};
#endif // __cameraH__