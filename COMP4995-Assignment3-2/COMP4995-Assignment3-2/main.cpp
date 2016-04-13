#include "d3dUtility.h"
#include "camera.h"
#include "Ray.h"
#include "BoundingSphere.h"

//
// Globals
//

IDirect3DDevice9* Device = 0;
D3DXMATRIX World;
IDirect3DVertexBuffer9* VB = 0;
IDirect3DTexture9* MirrorTex = 0;
D3DMATERIAL9 MirrorMtrl = d3d::WHITE_MTRL;
BoundingSphere bound1, bound2, bound3;
Ray ray;
bool hit = false;

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

Camera TheCamera(Camera::AIRCRAFT);

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

Ray d3d::CalcPickingRay(int x, int y)
{
	float px = 0.0f;
	float py = 0.0f;

	D3DVIEWPORT9 vp;
	Device->GetViewport(&vp);

	D3DXMATRIX proj;
	Device->GetTransform(D3DTS_PROJECTION, &proj);

	px = (((2.0f*x) / vp.Width) - 1.0f) / proj(0, 0);
	py = (((-2.0f*y) / vp.Height) + 1.0f) / proj(1, 1);

	Ray ray;
	ray._origin = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	ray._direction = D3DXVECTOR3(px, py, 1.0f);

	return ray;
}

void TransformRay(Ray* ray, D3DXMATRIX* T)
{
	// transform the ray's origin, w = 1.
	D3DXVec3TransformCoord(
		&ray->_origin,
		&ray->_origin,
		T);

	// transform the ray's direction, w = 0.
	D3DXVec3TransformNormal(
		&ray->_direction,
		&ray->_direction,
		T);

	// normalize the direction
	D3DXVec3Normalize(&ray->_direction, &ray->_direction);
}

bool RaySphereIntTest(Ray* ray, BoundingSphere* sphere)
{
	D3DXVECTOR3 v = ray->_origin - sphere->_center;

	float b = 2.0f * D3DXVec3Dot(&ray->_direction, &v);
	float c = D3DXVec3Dot(&v, &v) - (sphere->_radius * sphere->_radius);

	// find the discriminant
	float discriminant = (b * b) - (4.0f * c);

	// test for imaginary number
	if (discriminant < 0.0f)
		return false;

	discriminant = sqrtf(discriminant);

	float s0 = (-b + discriminant) / 2.0f;
	float s1 = (-b - discriminant) / 2.0f;

	// if a solution is >= 0, then we intersected the sphere
	if (s0 >= 0.0f || s1 >= 0.0f)
		return true;

	return false;
}

void setupBoundingSphere() {
	bound1._center = Meshes[0].pos; // plane
	bound1._radius = 1.25f;

	bound2._center = Meshes[1].pos; // monkey
	bound2._radius = 1.5f;
}

float distFromCamera(D3DXVECTOR3 vec) {
	float x = vec.x + ray._origin.x;
	float y = vec.y + ray._origin.y;
	float z = vec.z + ray._origin.z;
	return sqrt((x * x) + (y * y) + (z * z));
}

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
		ret[1] = 3.0f;
		ret[2] = -7.0f;
		break;
	case 1:
		ret[0] = -2.0f;
		ret[1] = 0.0f;
		ret[2] = -7.0f;
		break;
	case 2:
		ret[0] = -7.0f;
		ret[1] = 1.0f;
		ret[2] = -7.0f;
		break;
	}

	MeshStruct retstruct(Mesh, Mtrls, Textures, { ret[0],ret[1],ret[2] });
	Meshes.push_back(retstruct);
}

void DrawBoxInit() {
	Device->CreateVertexBuffer(
		36 * sizeof(Vertex),
		0, // usage
		Vertex::FVF,
		D3DPOOL_MANAGED,
		&VB,
		0);

	Vertex* vert = 0;
	VB->Lock(0, 0, (void**)&vert, 0);

	// Mirror
	// Left Face
	vert[0] = Vertex( -2.5f, 0.0f, 5.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	vert[1] = Vertex( -2.5f, 5.0f, 5.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	vert[2] = Vertex( -2.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);

	vert[3] = Vertex( -2.5f, 0.0f, 5.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	vert[4] = Vertex( -2.5f, 5.0f, -0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	vert[5] = Vertex( -2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	//Front Face
	vert[6] = Vertex( -2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	vert[7] = Vertex( -2.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	vert[8] = Vertex(  2.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);

	vert[9] = Vertex( -2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	vert[10] = Vertex( 2.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	vert[11] = Vertex( 2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	//Right Face
	vert[12] = Vertex( 2.5f, 5.0f, 5.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	vert[13] = Vertex( 2.5f, 0.0f, 5.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	vert[14] = Vertex( 2.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);

	vert[15] = Vertex( 2.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	vert[16] = Vertex( 2.5f, 0.0f, 5.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	vert[17] = Vertex( 2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	//Back Face
	vert[18] = Vertex(-2.5f, 5.0f, 5.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	vert[19] = Vertex(-2.5f, 0.0f, 5.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	vert[20] = Vertex( 2.5f, 5.0f, 5.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);

	vert[21] = Vertex( 2.5f, 5.0f, 5.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	vert[22] = Vertex(-2.5f, 0.0f, 5.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	vert[23] = Vertex( 2.5f, 0.0f, 5.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	//Top Face
	vert[24] = Vertex(-2.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	vert[25] = Vertex(-2.5f, 5.0f, 5.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	vert[26] = Vertex( 2.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);

	vert[27] = Vertex(-2.5f, 5.0f, 5.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	vert[28] = Vertex( 2.5f, 5.0f, 5.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	vert[29] = Vertex( 2.5f, 5.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	//Bottem Face
	vert[30] = Vertex(-2.5f, 0.0f, 5.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	vert[31] = Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	vert[32] = Vertex( 2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);

	vert[33] = Vertex( 2.5f, 0.0f, 5.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	vert[34] = Vertex(-2.5f, 0.0f, 5.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	vert[35] = Vertex( 2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	VB->Unlock();
}

bool RenderBox() {
	D3DXMATRIX I;
	D3DXMatrixIdentity(&I);
	Device->SetTransform(D3DTS_WORLD, &I);

	Device->SetStreamSource(0, VB, 0, sizeof(Vertex));
	Device->SetFVF(Vertex::FVF);

	// draw the mirror
	Device->SetMaterial(&MirrorMtrl);
	Device->SetTexture(0, MirrorTex);
	Device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);
	Device->DrawPrimitive(D3DPT_TRIANGLELIST, 6, 2);
	Device->DrawPrimitive(D3DPT_TRIANGLELIST, 12, 2);
	Device->DrawPrimitive(D3DPT_TRIANGLELIST, 18, 2);
	Device->DrawPrimitive(D3DPT_TRIANGLELIST, 24, 2);
	Device->DrawPrimitive(D3DPT_TRIANGLELIST, 30, 2);
	return true;
}

bool RenderMirrorBox() {
	D3DXMATRIX I;
	D3DXMatrixIdentity(&I);
	Device->SetTransform(D3DTS_WORLD, &I);

	Device->SetStreamSource(0, VB, 0, sizeof(Vertex));
	Device->SetFVF(Vertex::FVF);

	// draw the mirror
	Device->SetMaterial(&MirrorMtrl);
	Device->SetTexture(0, MirrorTex);
	Device->SetRenderState(D3DRS_STENCILREF, 1);
	Device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);
	Device->SetRenderState(D3DRS_STENCILREF, 2);
	Device->DrawPrimitive(D3DPT_TRIANGLELIST, 6, 2);
	Device->SetRenderState(D3DRS_STENCILREF, 3);
	Device->DrawPrimitive(D3DPT_TRIANGLELIST, 12, 2);
	Device->SetRenderState(D3DRS_STENCILREF, 4);
	Device->DrawPrimitive(D3DPT_TRIANGLELIST, 18, 2);
	Device->SetRenderState(D3DRS_STENCILREF, 5);
	Device->DrawPrimitive(D3DPT_TRIANGLELIST, 24, 2);
	Device->SetRenderState(D3DRS_STENCILREF, 6);
	Device->DrawPrimitive(D3DPT_TRIANGLELIST, 30, 2);
	return true;
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

bool RenderMirrorMesh(MeshStruct mesh, D3DXMATRIX W) {
	Device->SetTransform(D3DTS_WORLD, &W);
	for (int i = 0; i < mesh.mtrls.size(); i++) {
		Device->SetMaterial(&mesh.mtrls[i]);
		Device->SetTexture(0, mesh.tex[i]);
		mesh.mesh->DrawSubset(i);
	}
	return true;
}

bool RenderWhiteMesh(MeshStruct mesh) {
	D3DXMATRIX world;
	D3DXMatrixTranslation(&world, mesh.pos.x, mesh.pos.y, mesh.pos.z);
	Device->SetTransform(D3DTS_WORLD, &world);
	for (int i = 0; i < mesh.mtrls.size(); i++) {
		Device->SetMaterial(&d3d::WHITE_MTRL);
		Device->SetTexture(0, mesh.tex[i]);
		mesh.mesh->DrawSubset(i);
	}
	return true;
}

bool RenderMirror() {
	D3DXPLANE planes[6] = {
		{ 1.0f, 0.0f, 0.0f, 2.5f },
		{ 0.0f, 1.0f, 0.0f, 0.0f },
		{ -1.0f, 0.0f, 0.0f, 2.5f },
		{ 0.0f, 0.0f, -1.0f, 5.0f },
		{ 0.0f, -1.0f, 0.0f, 5.0f },
		{ 0.0f, 1.0f, 0.0f, 0.0f }
	};

	D3DXMATRIX I;
	D3DXMatrixIdentity(&I);

	for (int i = 0; i < 6; i++) {
		// Clear stencil-buffer in-between drawing individual mirror-faces!
		Device->Clear(0, 0, D3DCLEAR_STENCIL, NULL, 1.0f, 0);
		Device->Clear(0, 0, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);

		Device->SetRenderState(D3DRS_STENCILENABLE, true);
		Device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
		Device->SetRenderState(D3DRS_STENCILREF, i + 1);  // 0x1
		Device->SetRenderState(D3DRS_STENCILMASK, 0xffffffff);
		Device->SetRenderState(D3DRS_STENCILWRITEMASK, 0xffffffff);
		Device->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
		Device->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
		Device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);

		// disable writes to the depth and back buffers
		Device->SetRenderState(D3DRS_ZWRITEENABLE, false);
		Device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
		Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
		Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
		// draw the mirror to the stencil buffer
		Device->SetStreamSource(0, VB, 0, sizeof(Vertex));
		Device->SetFVF(Vertex::FVF);
		Device->SetMaterial(&MirrorMtrl);
		Device->SetTexture(0, MirrorTex);
		Device->SetTransform(D3DTS_WORLD, &I);
		Device->DrawPrimitive(D3DPT_TRIANGLELIST, 0 + 6 * i, 2);		// Draw mirror primitive (pulls from vertex-buffer)
																		// re-enable depth writes
		Device->SetRenderState(D3DRS_ZWRITEENABLE, true);

		// Clear depth buffer to disable re-render of mirror primaries
		Device->Clear(0, 0, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);

		// Set device states for blending of mirrored image onto mirror surface
		Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR);
		Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);

		// Device->SetTransform(D3DTS_WORLD, &W);	
		Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW); // Moved up two lines

		Device->SetClipPlane(0, planes[i]);
		Device->SetRenderState(D3DRS_CLIPPLANEENABLE, D3DCLIPPLANE0);

		for (int p = 0; p < 6; p++) {
			// only draw reflected teapot to the pixels where the mirror
			// was drawn to.
			Device->SetRenderState(D3DRS_STENCILREF, p + 1);
			Device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
			Device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);

			// Plane
			// position reflection
			D3DXMATRIX W, T, R;
			D3DXMatrixReflect(&R, &planes[p]);

			D3DXMatrixTranslation(&T,
				Meshes[0].pos.x,
				Meshes[0].pos.y,
				Meshes[0].pos.z);

			W = T * R;

			// Finally, draw the reflected teapot
			Device->SetTransform(D3DTS_WORLD, &W);
			// Set Clipping planes
			Device->SetClipPlane(0, planes[p]);
			Device->SetRenderState(D3DRS_CLIPPLANEENABLE, D3DCLIPPLANE0);

			RenderMirrorMesh(Meshes[0], W);
			// End Plane

			// Monkey
			D3DXMATRIX W2, T2;
			D3DXMatrixReflect(&R, &planes[p]);

			D3DXMatrixTranslation(&T2,
				Meshes[1].pos.x,
				Meshes[1].pos.y,
				Meshes[1].pos.z);

			W2 = T2 * R;

			// Finally, draw the reflected teapot
			Device->SetTransform(D3DTS_WORLD, &W2);
			// Set Clipping planes
			Device->SetClipPlane(0, planes[p]);
			Device->SetRenderState(D3DRS_CLIPPLANEENABLE, D3DCLIPPLANE0);

			RenderMirrorMesh(Meshes[1], W2);
			// End Monkey

			Device->SetRenderState(D3DRS_CLIPPLANEENABLE, false);
		}
	}

	// Restore render states.
	Device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
	Device->SetRenderState(D3DRS_STENCILENABLE, false);
	Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

	return true;
}

//
// Framework functions
//
bool Setup()
{
	// Load the models
	DrawBoxInit(); // Only called here
	LoadMesh("Harrier.x", 0);
	LoadMesh("Monkey.x", 1);

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
		float dist[] = { distFromCamera(Meshes[0].pos),
						 distFromCamera(Meshes[1].pos) };
		float tempDist = 0;
		for (float d : dist)
			if (tempDist < d)
				tempDist = d;
		//
		// Update: Update the camera.
		//

		switch (selected) {
		case -1:
			if (::GetAsyncKeyState('W') & 0x8000f)
				TheCamera.walk(4.0f * 0.05);

			if (::GetAsyncKeyState('S') & 0x8000f)
				TheCamera.walk(-4.0f * 0.05);

			if (::GetAsyncKeyState('A') & 0x8000f)
				TheCamera.strafe(-4.0f * 0.05);

			if (::GetAsyncKeyState('D') & 0x8000f)
				TheCamera.strafe(4.0f * 0.05);

			if (::GetAsyncKeyState('R') & 0x8000f)
				TheCamera.fly(4.0f * 0.05);

			if (::GetAsyncKeyState('F') & 0x8000f)
				TheCamera.fly(-4.0f * 0.05);

			if (::GetAsyncKeyState('N') & 0x8000f)
				TheCamera.roll(1.0f * 0.05);

			if (::GetAsyncKeyState('M') & 0x8000f)
				TheCamera.roll(-1.0f * 0.05);
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
			break;
		case 0:
			if (::GetAsyncKeyState('R') & 0x8000f)
				Meshes[0].pos.y += 1.0f * 0.05;

			if (::GetAsyncKeyState('F') & 0x8000f)
				Meshes[0].pos.y -= 1.0f * 0.05;

			if (::GetAsyncKeyState('D') & 0x8000f)
				Meshes[0].pos.x -= 1.0f * 0.05;

			if (::GetAsyncKeyState('A') & 0x8000f)
				Meshes[0].pos.x += 1.0f * 0.05;

			if (::GetAsyncKeyState('W') & 0x8000f)
				Meshes[0].pos.z -= 1.0f * 0.05;

			if (::GetAsyncKeyState('S') & 0x8000f)
				Meshes[0].pos.z += 1.0f * 0.05;
			break;
		case 1:
			if (::GetAsyncKeyState('R') & 0x8000f)
				Meshes[1].pos.y += 1.0f * 0.05;

			if (::GetAsyncKeyState('F') & 0x8000f)
				Meshes[1].pos.y -= 1.0f * 0.05;

			if (::GetAsyncKeyState('D') & 0x8000f)
				Meshes[1].pos.x -= 1.0f * 0.05;

			if (::GetAsyncKeyState('A') & 0x8000f)
				Meshes[1].pos.x += 1.0f * 0.05;

			if (::GetAsyncKeyState('W') & 0x8000f)
				Meshes[1].pos.z -= 1.0f * 0.05;

			if (::GetAsyncKeyState('S') & 0x8000f)
				Meshes[1].pos.z += 1.0f * 0.05;
			break;
		}
		if (::GetAsyncKeyState(VK_LBUTTON) & 0x8000f) { // Left button click to select
			selected = -1;
			GetCursorPos(&pos);
			ray = d3d::CalcPickingRay(pos.x, pos.y);
			D3DXMATRIX temp;
			D3DXMATRIX V;
			TheCamera.getViewMatrix(&V);
			D3DXMatrixInverse(&temp, 0, &V);
			TransformRay(&ray, &temp);
			// check each
			if (RaySphereIntTest(&ray, &bound1) && dist[0] <= tempDist) {
				// Plane is closer
				selected = 0;
			}
			if (RaySphereIntTest(&ray, &bound2) && dist[1] <= tempDist) {
				// Monkey is closer
				selected = 1;
			}
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
		Device->BeginScene(); // BEGIN SCENE

		setupBoundingSphere();

		RenderBox();

		for (auto mesh : Meshes) {
			RenderMesh(mesh);
		}

		d3d::DrawBasicScene(Device, 1.0f);

		RenderMirror();

		Device->EndScene(); // END SCENE
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
