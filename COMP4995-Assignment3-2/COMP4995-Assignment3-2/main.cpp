#include "d3dUtility.h"
#include "camera.h"

//
// Globals
//

IDirect3DDevice9* Device = 0;

const int Width = 640;
const int Height = 480;

POINT pos, cur;

int selected = -1; // Changes with picking

Camera TheCamera(Camera::LANDOBJECT);

//
// Framework functions
//
bool Setup()
{
	//
	// Setup a basic scene.  The scene will be created the
	// first time this function is called.
	//

	d3d::DrawBasicScene(Device, 0.0f);

	//
	// Set projection matrix.
	//

	D3DXMATRIX proj;
	D3DXMatrixPerspectiveFovLH(
		&proj,
		D3DX_PI * 0.25f, // 45 - degree
		(float)Width / (float)Height,
		1.0f,
		1000.0f);
	Device->SetTransform(D3DTS_PROJECTION, &proj);

	return true;
}

void Cleanup()
{
	// pass 0 for the first parameter to instruct cleanup.
	d3d::DrawBasicScene(0, 0.0f);
}

bool Display(float timeDelta)
{
	if (Device)
	{
		//
		// Update: Update the camera.
		//

		if (::GetAsyncKeyState('W') & 0x8000f)
			TheCamera.walk(4.0f * timeDelta);

		if (::GetAsyncKeyState('S') & 0x8000f)
			TheCamera.walk(-4.0f * timeDelta);

		if (::GetAsyncKeyState('A') & 0x8000f)
			TheCamera.strafe(-4.0f * timeDelta);

		if (::GetAsyncKeyState('D') & 0x8000f)
			TheCamera.strafe(4.0f * timeDelta);

		if (::GetAsyncKeyState('R') & 0x8000f)
			TheCamera.fly(4.0f * timeDelta);

		if (::GetAsyncKeyState('F') & 0x8000f)
			TheCamera.fly(-4.0f * timeDelta);

		if (::GetAsyncKeyState('N') & 0x8000f)
			TheCamera.roll(1.0f * timeDelta);

		if (::GetAsyncKeyState('M') & 0x8000f)
			TheCamera.roll(-1.0f * timeDelta);
		if (selected == -1) {
			GetCursorPos(&pos);
			if (selected == -1) {
				if (pos.x != cur.x || pos.y != cur.y) {
					TheCamera.yaw(((float)(pos.x - cur.x) / 500));
					TheCamera.pitch(((float)(pos.y - cur.y) / 500));
				}
			}
			SetCursorPos(Width / 2, Height / 2);
			GetCursorPos(&cur);
		}

		// Update the view matrix representing the cameras 
		// new position/orientation.
		D3DXMATRIX V;
		TheCamera.getViewMatrix(&V);
		Device->SetTransform(D3DTS_VIEW, &V);

		//
		// Render
		//

		Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0);
		Device->BeginScene();

		d3d::DrawBasicScene(Device, 1.0f);

		Device->EndScene();
		Device->Present(0, 0, 0, 0);
	}
	return true;
}

//
// WndProc
//
LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		::PostQuitMessage(0);
		break;

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			::DestroyWindow(hwnd);

		break;
	}
	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

//
// WinMain
//
int WINAPI WinMain(HINSTANCE hinstance,
	HINSTANCE prevInstance,
	PSTR cmdLine,
	int showCmd)
{
	if (!d3d::InitD3D(hinstance,
		Width, Height, TRUE, D3DDEVTYPE_HAL, &Device))
	{
		::MessageBox(0, _T("InitD3D() - FAILED"), 0, 0);
		return 0;
	}

	if (!Setup())
	{
		::MessageBox(0, _T("Setup() - FAILED"), 0, 0);
		return 0;
	}

	GetCursorPos(&pos);

	d3d::EnterMsgLoop(Display);

	Cleanup();

	Device->Release();

	return 0;
}
