//***************************************************************************************
// d3dApp.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "DirectXCrash.h"
#include <WindowsX.h>
#include <sstream>
#include <assert.h>

namespace
{
	// This is just used to forward Windows messages from a global window
	// procedure to our member function window procedure because we cannot
	// assign a member function to WNDCLASS::lpfnWndProc.
	D3DApp* gd3dApp = 0;
}

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.
	return gd3dApp->MsgProc(hwnd, msg, wParam, lParam);
}

D3DApp::D3DApp(HINSTANCE hInstance)
:	mhAppInst(hInstance),
	mMainWndCaption(L"D3D11 Application"),
	mClientWidth(800),
	mClientHeight(600),
	mEnable4xMsaa(false),
	mhMainWnd(0),
	mAppPaused(false),
	mMinimized(false),
	mMaximized(false),
	mResizing(false),
	m4xMsaaQuality(0),
 
	md3dDevice(0),
	context(0),
	mSwapChain(0),
	mDepthStencilBuffer(0),
	mRenderTargetView(0),
	mDepthStencilView(0)
{
	ZeroMemory(&mScreenViewport, sizeof(D3D11_VIEWPORT));

	// Get a pointer to the application object so we can forward 
	// Windows messages to the object's window procedure through
	// the global window procedure.
	gd3dApp = this;
}

D3DApp::~D3DApp()
{
	ReleaseCOM(mRenderTargetView);
	ReleaseCOM(mDepthStencilView);
	ReleaseCOM(mSwapChain);
	ReleaseCOM(mDepthStencilBuffer);

	// Restore all default settings.
	if( context )
		context->ClearState();

	ReleaseCOM(context);
	ReleaseCOM(md3dDevice);
}

HINSTANCE D3DApp::AppInst()const
{
	return mhAppInst;
}

HWND D3DApp::MainWnd()const
{
	return mhMainWnd;
}

int D3DApp::Run()
{
	MSG msg = {0};
 
	while(msg.message != WM_QUIT)
	{
		// If there are Window messages then process them.
		if(PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
		{
            TranslateMessage( &msg );
            DispatchMessage( &msg );
		}
		// Otherwise, do animation/game stuff.
		else
        {
			// Vertex Buffer
			UINT stride = sizeof(VertexPositionTexture), offset = 0;
			context->IASetVertexBuffers(0, 1, &mVB, &stride, &offset);
			context->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			// Shader/DepthState 1
			context->OMSetDepthStencilState(mDS1, 0);
			context->VSSetShader(mShader1.mVS, nullptr, 0);
			context->PSSetShader(mShader1.mPS, nullptr, 0);
			context->IASetInputLayout(mShader1.mInput);
			context->VSSetConstantBuffers(0, 1, &mShader1.mVSBuffer);
			context->PSSetConstantBuffers(0, 1, &mShader1.mPSBuffer);
			context->Draw(4, 0);

			// Shader/DepthState 2
			context->OMSetDepthStencilState(mDS2, 0);
			context->VSSetShader(mShader2.mVS, nullptr, 0);
			context->PSSetShader(mShader2.mPS, nullptr, 0);
			context->IASetInputLayout(mShader2.mInput);
			context->VSSetConstantBuffers(0, 1, &mShader2.mVSBuffer);
			context->PSSetConstantBuffers(0, 1, &mShader2.mPSBuffer);
			context->Draw(4, 0);

			ThrowIfFailed(mSwapChain->Present(0, 0));
		}
    }

	return (int)msg.wParam;
}

bool D3DApp::Init()
{
	if(!InitMainWindow())
		return false;

	if(!InitDirect3D())
		return false;

	return true;
}
 
void D3DApp::OnResize()
{
	assert(context);
	assert(md3dDevice);
	assert(mSwapChain);

	// Release the old views, as they hold references to the buffers we
	// will be destroying.  Also release the old depth/stencil buffer.

	ReleaseCOM(mRenderTargetView);
	ReleaseCOM(mDepthStencilView);
	ReleaseCOM(mDepthStencilBuffer);


	// Resize the swap chain and recreate the render target view.

	ThrowIfFailed(mSwapChain->ResizeBuffers(1, mClientWidth, mClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
	ID3D11Texture2D* backBuffer;
	ThrowIfFailed(mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)));
	ThrowIfFailed(md3dDevice->CreateRenderTargetView(backBuffer, 0, &mRenderTargetView));
	ReleaseCOM(backBuffer);

	// Create the depth/stencil buffer and view.

	D3D11_TEXTURE2D_DESC depthStencilDesc;
	
	depthStencilDesc.Width     = mClientWidth;
	depthStencilDesc.Height    = mClientHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Use 4X MSAA? --must match swap chain MSAA values.
	if( mEnable4xMsaa )
	{
		depthStencilDesc.SampleDesc.Count   = 4;
		depthStencilDesc.SampleDesc.Quality = m4xMsaaQuality-1;
	}
	// No MSAA
	else
	{
		depthStencilDesc.SampleDesc.Count   = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
	}

	depthStencilDesc.Usage          = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags      = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0; 
	depthStencilDesc.MiscFlags      = 0;

	ThrowIfFailed(md3dDevice->CreateTexture2D(&depthStencilDesc, 0, &mDepthStencilBuffer));
	ThrowIfFailed(md3dDevice->CreateDepthStencilView(mDepthStencilBuffer, 0, &mDepthStencilView));

	// Bind the render target view and depth/stencil view to the pipeline.

	context->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);
	

	// Set the viewport transform.

	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width    = static_cast<float>(mClientWidth);
	mScreenViewport.Height   = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	context->RSSetViewports(1, &mScreenViewport);
}
 
LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch( msg )
	{
	// WM_ACTIVATE is sent when the window is activated or deactivated.  
	// We pause the game when the window is deactivated and unpause it 
	// when it becomes active.  
	case WM_ACTIVATE:
		if( LOWORD(wParam) == WA_INACTIVE )
		{
			mAppPaused = true;
		}
		else
		{
			mAppPaused = false;
		}
		return 0;

	// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
		mClientWidth  = LOWORD(lParam);
		mClientHeight = HIWORD(lParam);
		if( md3dDevice )
		{
			if( wParam == SIZE_MINIMIZED )
			{
				mAppPaused = true;
				mMinimized = true;
				mMaximized = false;
			}
			else if( wParam == SIZE_MAXIMIZED )
			{
				mAppPaused = false;
				mMinimized = false;
				mMaximized = true;
				OnResize();
			}
			else if( wParam == SIZE_RESTORED )
			{
				
				// Restoring from minimized state?
				if( mMinimized )
				{
					mAppPaused = false;
					mMinimized = false;
					OnResize();
				}

				// Restoring from maximized state?
				else if( mMaximized )
				{
					mAppPaused = false;
					mMaximized = false;
					OnResize();
				}
				else if( mResizing )
				{
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;

	// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		mAppPaused = true;
		mResizing  = true;
		return 0;

	// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
	// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing  = false;
		OnResize();
		return 0;
 
	// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	// The WM_MENUCHAR message is sent when a menu is active and the user presses 
	// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
        // Don't beep when we alt-enter.
        return MAKELRESULT(0, MNC_CLOSE);

	// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200; 
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}


bool D3DApp::InitMainWindow()
{
	WNDCLASS wc;
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = MainWndProc; 
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = mhAppInst;
	wc.hIcon         = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = L"D3DWndClassName";

	if( !RegisterClass(&wc) )
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 100, 100, mClientWidth, mClientHeight };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width  = R.right - R.left;
	int height = R.bottom - R.top;

	mhMainWnd = CreateWindow(L"D3DWndClassName", mMainWndCaption.c_str(), 
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0); 
	if( !mhMainWnd )
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);

	return true;
}

ID3D11Buffer* D3DApp::CreateConstantBuffer(int bufferSize) const
{
	D3D11_BUFFER_DESC desc;

	memset(&desc, 0, sizeof(desc));
	desc.ByteWidth = bufferSize;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	ID3D11Buffer* result;
	ThrowIfFailed(md3dDevice->CreateBuffer(&desc, nullptr, &result));

	return result;
}

Shader D3DApp::CreateShader(const std::wstring& filename, int bufferSize) const
{
	Shader result;

	UINT flags = D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY | D3DCOMPILE_SKIP_OPTIMIZATION;

	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	// Compile Vertex and Pixel shaders
	ThrowIfFailed(D3DCompileFromFile(filename.c_str(), NULL, NULL, "VS", "vs_4_0", flags, 0, &shaderBlob, &errorBlob));
	ThrowIfFailed(md3dDevice->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, &result.mVS));

	ID3DBlob* vertexBlob = shaderBlob;
	shaderBlob = errorBlob = nullptr;
	ThrowIfFailed(D3DCompileFromFile(filename.c_str(), NULL, NULL, "PS", "ps_4_0", flags, 0, &shaderBlob, &errorBlob));
	ThrowIfFailed(md3dDevice->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, &result.mPS));


	D3D11_INPUT_ELEMENT_DESC quadLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	ThrowIfFailed(md3dDevice->CreateInputLayout(quadLayout, 2, vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), &result.mInput));

	result.mVSBuffer = CreateConstantBuffer(bufferSize);
	result.mPSBuffer = CreateConstantBuffer(bufferSize);


	return result;
}

ID3D11Buffer* D3DApp::CreateVertexBuffer(const RECT& rectangle, const POINTF& texCoordTopLeft, const POINTF& texCoordBottomRight) const
{

	D3D11_BUFFER_DESC desc;

	memset(&desc, 0, sizeof(desc));
	desc.ByteWidth = 4 * sizeof(VertexPositionTexture);
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	ID3D11Buffer* result;
	ThrowIfFailed(md3dDevice->CreateBuffer(&desc, nullptr, &result));

	int left = rectangle.left;
	int top = rectangle.top;
	int bottom = rectangle.bottom;
	int right = rectangle.right;

	VertexPositionTexture data[4];
	data[0].Position[0] = left;
	data[0].Position[1] = top;
	data[0].Position[2] = 0;
	data[0].TexCoord[0] = texCoordTopLeft.x;
	data[0].TexCoord[1] = texCoordTopLeft.y;
	
	data[1].Position[0] = right;
	data[1].Position[1] = top;
	data[1].Position[2] = 0;
	data[1].TexCoord[0] = texCoordBottomRight.x;
	data[1].TexCoord[1] = texCoordTopLeft.y;

	data[2].Position[0] = left;
	data[2].Position[1] = bottom;
	data[2].Position[2] = 0;
	data[2].TexCoord[0] = texCoordTopLeft.x;
	data[2].TexCoord[1] = texCoordBottomRight.y;

	data[3].Position[0] = right;
	data[3].Position[1] = bottom;
	data[3].Position[2] = 0;
	data[3].TexCoord[0] = texCoordBottomRight.x;
	data[3].TexCoord[1] = texCoordBottomRight.y;

	D3D11_MAPPED_SUBRESOURCE dataBox;
	ThrowIfFailed(context->Map(result, 0, D3D11_MAP_WRITE_DISCARD, 0, &dataBox));
	memcpy(dataBox.pData, &data, 4 * sizeof(VertexPositionTexture));
	context->Unmap(result, 0);

	return result;
}


bool D3DApp::InitDirect3D()
{
	// Create the device and device context.

	D3D_FEATURE_LEVEL featureLevel;
	HRESULT hr = D3D11CreateDevice(
			0,                 // default adapter
			D3D_DRIVER_TYPE_HARDWARE,
			0,                 // no software device
			D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
			0, 0,              // default feature level array
			D3D11_SDK_VERSION,
			&md3dDevice,
			&featureLevel,
			&context);

	if( FAILED(hr) )
	{
		MessageBox(0, L"D3D11CreateDevice Failed.", 0, 0);
		return false;
	}

	if( featureLevel != D3D_FEATURE_LEVEL_11_0 )
	{
		MessageBox(0, L"Direct3D Feature Level 11 unsupported.", 0, 0);
		return false;
	}

	// Check 4X MSAA quality support for our back buffer format.
	// All Direct3D 11 capable devices support 4X MSAA for all render 
	// target formats, so we only need to check quality support.

	ThrowIfFailed(md3dDevice->CheckMultisampleQualityLevels(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, &m4xMsaaQuality));
	assert( m4xMsaaQuality > 0 );

	// Fill out a DXGI_SWAP_CHAIN_DESC to describe our swap chain.

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width  = mClientWidth;
	sd.BufferDesc.Height = mClientHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Use 4X MSAA? 
	if( mEnable4xMsaa )
	{
		sd.SampleDesc.Count   = 4;
		sd.SampleDesc.Quality = m4xMsaaQuality-1;
	}
	// No MSAA
	else
	{
		sd.SampleDesc.Count   = 1;
		sd.SampleDesc.Quality = 0;
	}

	sd.BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount  = 1;
	sd.OutputWindow = mhMainWnd;
	sd.Windowed     = true;
	sd.SwapEffect   = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags        = 0;

	// To correctly create the swap chain, we must use the IDXGIFactory that was
	// used to create the device.  If we tried to use a different IDXGIFactory instance
	// (by calling CreateDXGIFactory), we get an error: "IDXGIFactory::CreateSwapChain: 
	// This function is being called with a device from a different IDXGIFactory."

	IDXGIDevice* dxgiDevice = 0;
	ThrowIfFailed(md3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice));
	      
	IDXGIAdapter* dxgiAdapter = 0;
	ThrowIfFailed(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter));

	IDXGIFactory* dxgiFactory = 0;
	ThrowIfFailed(dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory));

	ThrowIfFailed(dxgiFactory->CreateSwapChain(md3dDevice, &sd, &mSwapChain));
	
	ReleaseCOM(dxgiDevice);
	ReleaseCOM(dxgiAdapter);
	ReleaseCOM(dxgiFactory);

	// The remaining steps that need to be carried out for d3d creation
	// also need to be executed every time the window is resized.  So
	// just call the OnResize method here to avoid code duplication.
	
	OnResize();

	mShader1 = CreateShader(L"RebuildZBuffer.fx", 112);
	mShader2 = CreateShader(L"CameraMotionBlur.fx", 224);

	RECT r;
	r.left = 0;
	r.top = 0;
	r.right = 1600;
	r.bottom = 900;

	POINTF topLeft;
	topLeft.x = 0;
	topLeft.y = 0;

	POINTF bottomRight;
	bottomRight.x = 1;
	bottomRight.y = 1;
	mVB = CreateVertexBuffer(r, topLeft, bottomRight);

	CD3D11_DEPTH_STENCIL_DESC desc;
	memset(&desc, 0, sizeof(desc));
	desc.DepthEnable = 1;
	desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	desc.StencilEnable = 0;
	desc.StencilWriteMask = 255;
	desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;

	md3dDevice->CreateDepthStencilState(&desc, &mDS1);

	desc.DepthEnable = 0;
	desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

	md3dDevice->CreateDepthStencilState(&desc, &mDS2);

	return true;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		D3DApp theApp(hInstance);

		if (!theApp.Init())
			return 0;

		return theApp.Run();
	}
	catch (const DxException& ex)
	{
		std::wstring w = ex.ToString();
		MessageBoxW(nullptr, w.c_str(), L"Error", MB_OK);
	}
}