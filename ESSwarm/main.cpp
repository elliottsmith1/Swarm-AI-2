//Include and link appropriate libraries and headers//
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment (lib, "dinput8.lib")
#pragma comment (lib, "dxguid.lib")

#include <windows.h>
#include <d3d11.h>
#include <directxmath.h>
#include <d3dcompiler.h>
#include <dinput.h>
#include <vector>

#include "Gameobject.h"

using namespace DirectX;

//Global Declarations - Interfaces//
IDXGISwapChain* SwapChain;
ID3D11Device* d3d11Device;
ID3D11DeviceContext* d3d11DevCon;
ID3D11RenderTargetView* renderTargetView;
ID3D11Buffer* triangleIndexBuffer;
ID3D11Buffer* triangleVertBuffer;
ID3D11VertexShader* VS;
ID3D11PixelShader* PS;
ID3D10Blob* VS_Buffer;
ID3D10Blob* PS_Buffer;
ID3D11InputLayout* vertLayout;
ID3D11Buffer* cbPerObjectBuffer;
IDirectInputDevice8* DIKeyboard;
IDirectInputDevice8* DIMouse;

ID3D11RasterizerState* RSCullNone;

//Global Declarations - Others//
LPCTSTR WndClassName = "firstwindow";
HWND hwnd = NULL;
HRESULT hr;

const int Width = 1920;
const int Height = 1080;

DIMOUSESTATE mouseLastState;
LPDIRECTINPUT8 DirectInput;

XMMATRIX WVP;
XMMATRIX World;
//XMMATRIX triangleWorld;
GameObject* triangle;
XMMATRIX camView;
XMMATRIX camProjection;

XMVECTOR camPosition;
XMVECTOR camTarget;
XMVECTOR camUp;

//XMMATRIX Rotation;
//XMMATRIX Scale;
//XMMATRIX Translation;
float rot = 0.01f;

double countsPerSecond = 0.0;
__int64 CounterStart = 0;

int frameCount = 0;
int fps = 0;

__int64 frameTimeOld = 0;
double frameTime;

//Function Prototypes//
bool InitializeDirect3d11App(HINSTANCE hInstance);
void CleanUp();
bool InitScene();
void UpdateScene();
void DrawScene();

void UpdateScene(double time);

//void RenderText(std::wstring text, int inInt);

void StartTimer();
double GetTime();
double GetFrameTime();

bool InitializeWindow(HINSTANCE hInstance,
	int ShowWnd,
	int width, int height,
	bool windowed);
int messageloop();

bool InitDirectInput(HINSTANCE hInstance);
void DetectInput(double time);

LRESULT CALLBACK WndProc(HWND hWnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam);

//Create effects constant buffer's structure//
struct cbPerObject
{
	XMMATRIX  WVP;
	XMMATRIX World;

	BOOL isInstance;
};

cbPerObject cbPerObj;

const int swarm_population = 100;
const int numLeavesPerTree = 1000;

struct cbPerScene
{
	XMMATRIX leafOnTree[numLeavesPerTree];
};

cbPerScene cbPerInst;
ID3D11Buffer* cbPerInstanceBuffer;
ID3D11InputLayout* leafVertLayout;

struct InstanceData
{
	XMFLOAT3 pos;
};

ID3D11Buffer* swarmInstanceBuff;
XMMATRIX swarmWorld;

//Vertex Structure and Vertex Layout (Input Layout)//
struct Vertex    //Overloaded Vertex Structure
{
	Vertex() {}
	Vertex(float x, float y, float z,
		float cr, float cg, float cb, float ca)
		: pos(x, y, z), color(cr, cg, cb, ca) {}

	XMFLOAT3 pos;
	XMFLOAT4 color;
};

D3D11_INPUT_ELEMENT_DESC layout[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },

	{ "INSTANCEPOS", 0, DXGI_FORMAT_R32G32B32_FLOAT,    1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 }
};
UINT numElements = ARRAYSIZE(layout);

D3D11_INPUT_ELEMENT_DESC leafLayout[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },

	{ "INSTANCEPOS", 0, DXGI_FORMAT_R32G32B32_FLOAT,    1, 0, D3D11_INPUT_PER_INSTANCE_DATA, numLeavesPerTree }
};
UINT numLeafElements = ARRAYSIZE(leafLayout);

int WINAPI WinMain(HINSTANCE hInstance,    //Main windows function
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nShowCmd)
{

	if (!InitializeWindow(hInstance, nShowCmd, Width, Height, true))
	{
		MessageBox(0, "Window Initialization - Failed",
			"Error", MB_OK);
		return 0;
	}

	if (!InitializeDirect3d11App(hInstance))    //Initialize Direct3D
	{
		MessageBox(0, "Direct3D Initialization - Failed",
			"Error", MB_OK);
		return 0;
	}

	if (!InitScene())    //Initialize our scene
	{
		MessageBox(0, "Scene Initialization - Failed",
			"Error", MB_OK);
		return 0;
	}

	if (!InitDirectInput(hInstance))
	{
		MessageBox(0, "Direct Input Initialization - Failed",
			"Error", MB_OK);
		return 0;
	}

	messageloop();

	CleanUp();

	return 0;
}

bool InitializeWindow(HINSTANCE hInstance,
	int ShowWnd,
	int width, int height,
	bool windowed)
{
	typedef struct _WNDCLASS {
		UINT cbSize;
		UINT style;
		WNDPROC lpfnWndProc;
		int cbClsExtra;
		int cbWndExtra;
		HANDLE hInstance;
		HICON hIcon;
		HCURSOR hCursor;
		HBRUSH hbrBackground;
		LPCTSTR lpszMenuName;
		LPCTSTR lpszClassName;
	} WNDCLASS;

	WNDCLASSEX wc;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = NULL;
	wc.cbWndExtra = NULL;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = WndClassName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, "Error registering class",
			"Error", MB_OK | MB_ICONERROR);
		return 1;
	}

	hwnd = CreateWindowEx(
		NULL,
		WndClassName,
		"Swarm AI - Elliott Smith",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		width, height,
		NULL,
		NULL,
		hInstance,
		NULL
		);

	if (!hwnd)
	{
		MessageBox(NULL, "Error creating window",
			"Error", MB_OK | MB_ICONERROR);
		return 1;
	}

	ShowWindow(hwnd, ShowWnd);
	UpdateWindow(hwnd);

	return true;
}

bool InitializeDirect3d11App(HINSTANCE hInstance)
{
	//Describe our Buffer
	DXGI_MODE_DESC bufferDesc;

	ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));

	bufferDesc.Width = Width;
	bufferDesc.Height = Height;
	bufferDesc.RefreshRate.Numerator = 60;
	bufferDesc.RefreshRate.Denominator = 1;
	bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	//Describe our SwapChain
	DXGI_SWAP_CHAIN_DESC swapChainDesc;

	ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));

	swapChainDesc.BufferDesc = bufferDesc;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.OutputWindow = hwnd;
	swapChainDesc.Windowed = false;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;


	//Create our SwapChain
	hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, NULL, NULL,
		D3D11_SDK_VERSION, &swapChainDesc, &SwapChain, &d3d11Device, NULL, &d3d11DevCon);

	//Create our BackBuffer
	ID3D11Texture2D* BackBuffer;
	hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);

	//Create our Render Target
	hr = d3d11Device->CreateRenderTargetView(BackBuffer, NULL, &renderTargetView);
	BackBuffer->Release();

	//Set our Render Target
	d3d11DevCon->OMSetRenderTargets(1, &renderTargetView, NULL);

	return true;
}

bool InitDirectInput(HINSTANCE hInstance)
{
	hr = DirectInput8Create(hInstance,
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		(void**)&DirectInput,
		NULL);

	hr = DirectInput->CreateDevice(GUID_SysKeyboard,
		&DIKeyboard,
		NULL);

	hr = DirectInput->CreateDevice(GUID_SysMouse,
		&DIMouse,
		NULL);

	hr = DIKeyboard->SetDataFormat(&c_dfDIKeyboard);
	hr = DIKeyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

	hr = DIMouse->SetDataFormat(&c_dfDIMouse);
	hr = DIMouse->SetCooperativeLevel(hwnd, DISCL_EXCLUSIVE | DISCL_NOWINKEY | DISCL_FOREGROUND);

	return true;
}

void DetectInput(double time)
{
	DIMOUSESTATE mouseCurrState;

	BYTE keyboardState[256];

	DIKeyboard->Acquire();
	DIMouse->Acquire();

	DIMouse->GetDeviceState(sizeof(DIMOUSESTATE), &mouseCurrState);

	DIKeyboard->GetDeviceState(sizeof(keyboardState), (LPVOID)&keyboardState);

	if (keyboardState[DIK_ESCAPE] & 0x80)
		PostMessage(hwnd, WM_DESTROY, 0, 0);

	XMFLOAT4 camPos;

	XMStoreFloat4(&camPos, camPosition);

	if (keyboardState[DIK_LEFT] & 0x80)
	{
		camPos.x -= 100.0f;

		XMVectorSetIntX(camPosition, camPos.x);
	}
	if (keyboardState[DIK_RIGHT] & 0x80)
	{
		camPos.x += 100.0f;

		XMVectorSetIntX(camPosition, camPos.x);
	}

	if (keyboardState[DIK_UP] & 0x80)
	{
		camPos.y += 100.0f;

		XMVectorSetIntY(camPosition, camPos.y);
	}
	if (keyboardState[DIK_DOWN] & 0x80)
	{
		camPos.y -= 100.0f;

		XMVectorSetIntY(camPosition, camPos.y);
	}

	if (mouseCurrState.lX != mouseLastState.lX)
	{
		//scaleX -= (mouseCurrState.lX * 0.001f);
	}

	if (mouseCurrState.lY != mouseLastState.lY)
	{
		//scaleY -= (mouseCurrState.lY * 0.001f);
	}

	/*if (rotx > 6.28)
		rotx -= 6.28;
	else if (rotx < 0)
		rotx = 6.28 + rotx;

	if (rotz > 6.28)
		rotz -= 6.28;
	else if (rotz < 0)
		rotz = 6.28 + rotz;*/

	mouseLastState = mouseCurrState;

	return;
}

void CleanUp()
{
	SwapChain->SetFullscreenState(false, NULL);
	PostMessage(hwnd, WM_DESTROY, 0, 0);

	//Release the COM Objects we created
	SwapChain->Release();
	d3d11Device->Release();
	d3d11DevCon->Release();
	renderTargetView->Release();
	triangleVertBuffer->Release();
	triangleIndexBuffer->Release();
	VS->Release();
	PS->Release();
	VS_Buffer->Release();
	PS_Buffer->Release();
	vertLayout->Release();
	cbPerObjectBuffer->Release();
	DIKeyboard->Unacquire();
	DIMouse->Unacquire();
	DirectInput->Release();

	triangle = nullptr;
	delete triangle;

}

bool InitScene()
{

	// Set up the swarm positions then instance buffer
	std::vector<InstanceData> inst(swarm_population);
	XMVECTOR tempPos;
	srand(100);
	// We are just creating random positions for the swarm, between the positions of (-100, 0, -100) to (100, 0, 100)
	// then storing the position in our instanceData array
	for (int i = 0; i < swarm_population; i++)
	{
		float randX = ((float)(rand() % 2000) / 10) - 100;
		float randZ = ((float)(rand() % 2000) / 10) - 100;
		tempPos = XMVectorSet(randX, 0.0f, randZ, 0.0f);

		XMStoreFloat3(&inst[i].pos, tempPos);
	}

	// Create our swarm instance buffer
	// Pretty much the same thing as a regular vertex buffer, except that this buffers data
	// will be used per "instance" instead of per "vertex". Each instance of the geometry
	// gets it's own instanceData data, similar to how each vertex of the geometry gets its own
	// Vertex data
	D3D11_BUFFER_DESC instBuffDesc;
	ZeroMemory(&instBuffDesc, sizeof(instBuffDesc));

	instBuffDesc.Usage = D3D11_USAGE_DEFAULT;
	instBuffDesc.ByteWidth = sizeof(InstanceData) * swarm_population;
	instBuffDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	instBuffDesc.CPUAccessFlags = 0;
	instBuffDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA instData;
	ZeroMemory(&instData, sizeof(instData));

	instData.pSysMem = &inst[0];
	hr = d3d11Device->CreateBuffer(&instBuffDesc, &instData, &swarmInstanceBuff);

	// The swarm's world matrix (We will keep it an identity matrix, but we could change their positions without
	// unrealistic effects, since remember that all transformations are done around the point (0,0,0), and we will
	// be applying this world matrix to our swarm AFTER they have been individually positioned depending on the
	// instance buffer, which means they will not be centered at the point (0,0,0))
	swarmWorld = XMMatrixIdentity();

	//Compile Shaders from shader file
	hr = D3DCompileFromFile(L"Effects.fx", 0, 0, "VS", "vs_4_0", 0, 0, &VS_Buffer, 0);
	hr = D3DCompileFromFile(L"Effects.fx", 0, 0, "PS", "ps_4_0", 0, 0, &PS_Buffer, 0);

	//Create the Shader Objects
	hr = d3d11Device->CreateVertexShader(VS_Buffer->GetBufferPointer(), VS_Buffer->GetBufferSize(), NULL, &VS);
	hr = d3d11Device->CreatePixelShader(PS_Buffer->GetBufferPointer(), PS_Buffer->GetBufferSize(), NULL, &PS);

	//Set Vertex and Pixel Shaders
	d3d11DevCon->VSSetShader(VS, 0, 0);
	d3d11DevCon->PSSetShader(PS, 0, 0);

	triangle = new GameObject;

	//Create the vertex buffer
	Vertex v[] =
	{
		Vertex(0.0f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f),
		Vertex(0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f),
		Vertex(-0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f),
	};

	DWORD indices[] = {
		0, 1, 2,
	};

	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));

	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(DWORD) * 1 * 3;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iinitData;

	iinitData.pSysMem = indices;
	d3d11Device->CreateBuffer(&indexBufferDesc, &iinitData, &triangleIndexBuffer);

	d3d11DevCon->IASetIndexBuffer(triangleIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(Vertex) * 3;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vertexBufferData;

	ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
	vertexBufferData.pSysMem = v;
	hr = d3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &triangleVertBuffer);

	srand(100);
	XMFLOAT3 fTPos;
	XMMATRIX rotationMatrix;
	XMMATRIX tempMatrix;
	for (int i = 0; i < swarm_population; i++)
	{
		float rotX = (rand() % 2000) / 500.0f; // Value between 0 and 4 PI (two circles, makes it slightly more mixed)
		float rotY = (rand() % 2000) / 500.0f;
		float rotZ = (rand() % 2000) / 500.0f;

		float distFromCenter = 6.0f - ((rand() % 1000) / 250.0f);

		if (distFromCenter > 4.0f)
			distFromCenter = 4.0f;

		tempPos = XMVectorSet(distFromCenter, 0.0f, 0.0f, 0.0f);
		rotationMatrix = XMMatrixRotationRollPitchYaw(rotX, rotY, rotZ);
		tempPos = XMVector3TransformCoord(tempPos, rotationMatrix);

		if (XMVectorGetY(tempPos) < -1.0f)
			tempPos = XMVectorSetY(tempPos, -XMVectorGetY(tempPos));

		XMStoreFloat3(&fTPos, tempPos);

		XMMATRIX Scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
		XMMATRIX Translation = XMMatrixTranslation(fTPos.x, fTPos.y + 8.0f, fTPos.z);
		tempMatrix = Scale * rotationMatrix * Translation;

		// To make things simple, we just store the matrix directly into our cbPerInst structure
		cbPerInst.leafOnTree[i] = XMMatrixTranspose(tempMatrix);
	}

	//Set the vertex buffer
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	d3d11DevCon->IASetVertexBuffers(0, 1, &triangleVertBuffer, &stride, &offset);

	//Create the Input Layout
	hr = d3d11Device->CreateInputLayout(layout, numElements, VS_Buffer->GetBufferPointer(),
		VS_Buffer->GetBufferSize(), &vertLayout);

	hr = d3d11Device->CreateInputLayout(leafLayout, numLeafElements, VS_Buffer->GetBufferPointer(),
		VS_Buffer->GetBufferSize(), &leafVertLayout);

	//Set the Input Layout
	d3d11DevCon->IASetInputLayout(vertLayout);

	//Set Primitive Topology
	d3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//Create the Viewport
	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = Width;
	viewport.Height = Height;

	//Set the Viewport
	d3d11DevCon->RSSetViewports(1, &viewport);

	//Create the buffer to send to the cbuffer in effect file
	D3D11_BUFFER_DESC cbbd;
	ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));

	cbbd.Usage = D3D11_USAGE_DEFAULT;
	cbbd.ByteWidth = sizeof(cbPerObject);
	cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbbd.CPUAccessFlags = 0;
	cbbd.MiscFlags = 0;

	hr = d3d11Device->CreateBuffer(&cbbd, NULL, &cbPerObjectBuffer);

	//Create the buffer to send to the cbuffer per instance in effect file
	ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));

	cbbd.Usage = D3D11_USAGE_DEFAULT;
	// We have already defined how many elements are in our leaf matrix array inside the cbPerScene structure
	cbbd.ByteWidth = sizeof(cbPerScene);
	cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbbd.CPUAccessFlags = 0;
	cbbd.MiscFlags = 0;

	hr = d3d11Device->CreateBuffer(&cbbd, NULL, &cbPerInstanceBuffer);

	// Now we set the constant buffer per instance 
	// We are sending this buffer to the GPU now, because it will not be updated throughout the scene,
	// so it would be a waste of time to be sending this to the GPU every frame, when we only have to
	// send it once per scene. This is why constant buffers should be separated depending on how often
	// they are updated, so that you do not send data to the GPU more often than you have to. It's a
	// performance thing 
	d3d11DevCon->UpdateSubresource(cbPerInstanceBuffer, 0, NULL, &cbPerInst, 0, 0);

	//Camera information
	camPosition = XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f);
	camTarget = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	camUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	//Set the View matrix
	camView = XMMatrixLookAtLH(camPosition, camTarget, camUp);

	//Set the Projection matrix
	camProjection = XMMatrixPerspectiveFovLH(0.4f*3.14f, (float)Width / Height, 1.0f, 1000.0f);

	return true;
}

void StartTimer()
{
	LARGE_INTEGER frequencyCount;
	QueryPerformanceFrequency(&frequencyCount);

	countsPerSecond = double(frequencyCount.QuadPart);

	QueryPerformanceCounter(&frequencyCount);
	CounterStart = frequencyCount.QuadPart;
}

double GetTime()
{
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);
	return double(currentTime.QuadPart - CounterStart) / countsPerSecond;
}

double GetFrameTime()
{
	LARGE_INTEGER currentTime;
	__int64 tickCount;
	QueryPerformanceCounter(&currentTime);

	tickCount = currentTime.QuadPart - frameTimeOld;
	frameTimeOld = currentTime.QuadPart;

	if (tickCount < 0.0f)
		tickCount = 0.0f;

	return float(tickCount) / countsPerSecond;
}

void UpdateScene(double time)
{
	XMFLOAT3 temp_pos = triangle->GetPos();
	temp_pos.x += 0.00005f;
	temp_pos.y += 0.00005f;

	triangle->SetPos(temp_pos);

	float temp_roll = triangle->GetRoll();
	temp_roll += 0.0001f;
	triangle->SetRoll(temp_roll);

	triangle->Tick(frameTime);
}

void DrawScene()
{
	//Clear our backbuffer
	float bgColor[4] = { (0.0f, 0.0f, 0.0f, 0.0f) };
	d3d11DevCon->ClearRenderTargetView(renderTargetView, bgColor);

	/*WVP = triangle->GetWorldMat() * camView * camProjection;
	cbPerObj.WVP = XMMatrixTranspose(WVP);
	d3d11DevCon->UpdateSubresource(cbPerObjectBuffer, 0, NULL, &cbPerObj, 0, 0);
	d3d11DevCon->VSSetConstantBuffers(0, 1, &cbPerObjectBuffer);*/

	//Draw the triangle
	//d3d11DevCon->DrawIndexed(3, 0, 0);

	UINT strides[2] = { sizeof(Vertex), sizeof(InstanceData) };
	UINT offsets[2] = { 0, 0 };

	// Store the vertex and instance buffers into an array
	ID3D11Buffer* vertInstBuffers[2] = { triangleVertBuffer, swarmInstanceBuff };

	d3d11DevCon->IASetInputLayout(leafVertLayout);

	d3d11DevCon->IASetIndexBuffer(triangleIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	d3d11DevCon->IASetVertexBuffers(0, 2, vertInstBuffers, strides, offsets);

	//Set the WVP matrix and send it to the constant buffer in effect file
	WVP = triangle->GetWorldMat() * camView * camProjection;
	cbPerObj.WVP = XMMatrixTranspose(WVP);
	cbPerObj.World = XMMatrixTranspose(swarmWorld);
	cbPerObj.isInstance = true;        // Tell shaders if this is instanced data so it will know to use instance data or not
	d3d11DevCon->UpdateSubresource(cbPerObjectBuffer, 0, NULL, &cbPerObj, 0, 0);

	// We are sending two constant buffers to the vertex shader now, wo we will create an array of them
	ID3D11Buffer* vsConstBuffers[2] = { cbPerObjectBuffer, cbPerInstanceBuffer };
	d3d11DevCon->VSSetConstantBuffers(0, 2, vsConstBuffers);
	d3d11DevCon->PSSetConstantBuffers(1, 1, &cbPerObjectBuffer);	

	d3d11DevCon->RSSetState(RSCullNone);
	d3d11DevCon->DrawIndexedInstanced(3, swarm_population, 0, 0, 0);

	// Reset the default Input Layout
	d3d11DevCon->IASetInputLayout(vertLayout);

	//Set Vertex and Pixel Shaders
	d3d11DevCon->VSSetShader(VS, 0, 0);
	d3d11DevCon->PSSetShader(PS, 0, 0);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	// Reset the default Input Layout
	d3d11DevCon->IASetInputLayout(vertLayout);
	
	//Present the backbuffer to the screen
	SwapChain->Present(0, 0);
}

int messageloop() {
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));
	while (true)
	{
		BOOL PeekMessageL(
			LPMSG lpMsg,
			HWND hWnd,
			UINT wMsgFilterMin,
			UINT wMsgFilterMax,
			UINT wRemoveMsg
			);

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			// run game code         
			frameCount++;
			if (GetTime() > 1.0f)
			{
				fps = frameCount;
				frameCount = 0;
				StartTimer();
			}

			frameTime = GetFrameTime();

			DetectInput(frameTime);

			UpdateScene(frameTime);

			frameTime = GetFrameTime();

			UpdateScene(frameTime);
			DrawScene();
		}
	}
	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) {
			DestroyWindow(hwnd);
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd,
		msg,
		wParam,
		lParam);
}
