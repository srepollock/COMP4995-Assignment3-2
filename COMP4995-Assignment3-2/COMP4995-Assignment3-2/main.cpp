#include "d3dUtility.h"
#include "camera.h"

//
// Globals
//

IDirect3DDevice9* Device = 0;
D3DXMATRIX World;

struct Vertex {
	Vertex() {
	}

	Vertex(float x, float y, float z,
		float nx, float ny, float nz,
		float u, float v) {
		_x = x;
		_y = y;
		_z = z;
		_nx = nx;
		_ny = ny;
		_nz = nz;
		_u = u;
		_v = v;
	}

	float _x, _y, _z;
	float _nx, _ny, _nz;
	float _u, _v;

	static const DWORD FVF;
};
const DWORD Vertex::FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;

const int Width = SCREEN_WIDTH;
const int Height = SCREEN_HEIGHT;

POINT pos, cur;

int selected = -1; // Changes with picking

Camera TheCamera(Camera::LANDOBJECT);

struct MeshStruct {
	ID3DXMesh* mesh;
	std::vector<D3DMATERIAL9> mtrls;
	std::vector<IDirect3DTexture9*> tex;
	D3DXVECTOR3 pos;
	MeshStruct(ID3DXMesh* m, std::vector<D3DMATERIAL9> mtrl, std::vector<IDirect3DTexture9*> t, D3DXVECTOR3 p) {
		mesh = m;
		mtrls = mtrl;
		tex = t;
		pos = p;
	}
};
template<class T> void Release(T t)
{
	if (t)
	{
		t->Release();
		t = 0;
	}
}
template<class T> void Delete(T t)
{
	if (t)
	{
		delete t;
		t = 0;
	}
}

std::vector<MeshStruct> Meshes;

bool LoadMesh(char * filename, int pos) {
	//temporary meshes
	ID3DXMesh* Mesh = nullptr;
	std::vector<D3DMATERIAL9> Mtrls(0);
	std::vector<IDirect3DTexture9*> Textures(0);

	// loading mesh
	HRESULT hr = 0;

	//
	// Load the XFile data.
	//

	ID3DXBuffer* adjBuffer = 0;
	ID3DXBuffer* mtrlBuffer = 0;
	DWORD numMtrls = 0;

	hr = D3DXLoadMeshFromX(
		filename,
		D3DXMESH_MANAGED,
		Device,
		&adjBuffer,
		&mtrlBuffer,
		0,
		&numMtrls,
		&Mesh);

	if (FAILED(hr)) {
		::MessageBox(0, _T("D3DXLoadMeshFromX() - FAILED"), 0, 0);
		return false;
	}

	//
	// Extract the materials, and load textures.
	//

	if (mtrlBuffer != 0 && numMtrls != 0) {
		D3DXMATERIAL* mtrls = (D3DXMATERIAL*)mtrlBuffer->GetBufferPointer();

		for (int i = 0; i < numMtrls; i++) {
			// the MatD3D property doesn't have an ambient value set
			// when its loaded, so set it now:
			mtrls[i].MatD3D.Ambient = mtrls[i].MatD3D.Diffuse;

			// save the ith material
			Mtrls.push_back(mtrls[i].MatD3D);


			// check if the ith material has an associative texture
			if (mtrls[i].pTextureFilename != 0) {
				// yes, load the texture for the ith subset
				IDirect3DTexture9* tex = 0;
				D3DXCreateTextureFromFile(
					Device,
					mtrls[i].pTextureFilename,
					&tex);

				// save the loaded texture
				Textures.push_back(tex);
			}
			else {
				// no texture for the ith subset
				Textures.push_back(0);
			}
		}
	}
	Release<ID3DXBuffer*>(mtrlBuffer); // done w/ buffer

									   //
									   // Optimize the mesh.
									   //
	hr = Mesh->OptimizeInplace(
		D3DXMESHOPT_ATTRSORT |
		D3DXMESHOPT_COMPACT |
		D3DXMESHOPT_VERTEXCACHE,
		(DWORD*)adjBuffer->GetBufferPointer(),
		0, 0, 0);

	Release<ID3DXBuffer*>(adjBuffer); // done w/ buffer

	if (FAILED(hr)) {
		::MessageBox(0, _T("OptimizeInplace() - FAILED"), 0, 0);
		return false;
	}

	// Load more
	float ret[5];
	switch (pos) {
	case 0:
		ret[0] = 0.0f;
		ret[1] = 0.5f;
		ret[2] = -7.5f;
		break;
	case 1:
		ret[0] = 7.5f;
		ret[1] = 0.5f;
		ret[2] = 2.5f;
		break;
	case 2:
		ret[0] = 0.0f;
		ret[1] = 0.5f;
		ret[2] = 7.5f;
		break;
	case 3: // obj 4
		ret[0] = 0.0f;
		ret[1] = 0.0f;
		ret[2] = 0.0f;
		break;
	case 4: // obj 5
		ret[0] = 0.0f;
		ret[1] = 0.0f;
		ret[2] = 0.0f;
		break;
	}

	MeshStruct retstruct(Mesh, Mtrls, Textures, { ret[0],ret[1],ret[2] });
	Meshes.push_back(retstruct);
}

bool RenderMesh(MeshStruct mesh) {
	D3DXMATRIX world;
	D3DXMatrixTranslation(&world, mesh.pos.x, mesh.pos.y, mesh.pos.z);
	Device->SetTransform(D3DTS_WORLD, &world);
	for (int i = 0; i < mesh.mtrls.size(); i++) {
		Device->SetMaterial(&mesh.mtrls[i]);
		Device->SetTexture(0, mesh.tex[i]);
		mesh.mesh->DrawSubset(i);
	}
	return true;
}

//
// Framework functions
//
bool Setup()
{
	// Load the models

	LoadMesh("box.x", 0);
	LoadMesh("Harrier.x", 1);
	LoadMesh("Monkey.x", 2);

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

		for (auto mesh : Meshes) {
			RenderMesh(mesh);
		}

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
