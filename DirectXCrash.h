//***************************************************************************************
// d3dApp.h by Frank Luna (C) 2011 All Rights Reserved.
//
// Simple Direct3D demo application class.  
// Make sure you link: d3d11.lib d3dx11d.lib D3DCompiler.lib D3DX11EffectsD.lib 
//                     dxerr.lib dxgi.lib dxguid.lib.
// Link d3dx11.lib and D3DX11Effects.lib for release mode builds instead
//   of d3dx11d.lib and D3DX11EffectsD.lib.
//***************************************************************************************

#ifndef D3DAPP_H
#define D3DAPP_H

#include <d3d11.h>
#include <d3dcompiler.h>
#include <comdef.h>
#include <string>
#include <windows.h>
#include <wrl.h>

#define ReleaseCOM(x) { if(x){ x->Release(); x = 0; } }

class DxException
{
public:
	DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
		ErrorCode(hr),
		FunctionName(functionName),
		Filename(filename),
		LineNumber(lineNumber)
	{
	}

	std::wstring ToString()const
	{
		// Get the string description of the error code.
		_com_error err(ErrorCode);
		std::wstring msg = err.ErrorMessage();

		return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
	}

	HRESULT ErrorCode = S_OK;
	std::wstring FunctionName;
	std::wstring Filename;
	int LineNumber = -1;
};

inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif

struct VertexPositionTexture
{
	float Position[4];
	float TexCoord[2];
};

class Shader
{
public:
	ID3D11VertexShader* mVS;
	ID3D11PixelShader* mPS;
	ID3D11InputLayout* mInput;
	ID3D11Buffer* mVSBuffer;
	ID3D11Buffer* mPSBuffer;
};

class D3DApp
{
public:
	D3DApp(HINSTANCE hInstance);
	virtual ~D3DApp();
	
	HINSTANCE AppInst()const;
	HWND      MainWnd()const;
	
	int Run();
 
	// Framework methods.  Derived client class overrides these methods to 
	// implement specific application requirements.

	virtual bool Init();
	virtual void OnResize(); 
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// Convenience overrides for handling mouse input.
	virtual void OnMouseDown(WPARAM btnState, int x, int y){ }
	virtual void OnMouseUp(WPARAM btnState, int x, int y)  { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y){ }

	Shader CreateShader(const std::wstring& filename, int bufferSize) const;
	ID3D11Buffer* CreateConstantBuffer(int bufferSize) const;
	ID3D11Buffer* CreateVertexBuffer(const RECT &r, const POINTF &texCoordTopLeft, const POINTF &texCoordBottomRight) const;
protected:
	bool InitMainWindow();
	bool InitDirect3D();

protected:

	HINSTANCE mhAppInst;
	HWND      mhMainWnd;
	bool      mAppPaused;
	bool      mMinimized;
	bool      mMaximized;
	bool      mResizing;
	UINT      m4xMsaaQuality;

	ID3D11Device* md3dDevice;
	ID3D11DeviceContext* context;
	IDXGISwapChain* mSwapChain;
	ID3D11Texture2D* mDepthStencilBuffer;
	ID3D11RenderTargetView* mRenderTargetView;
	ID3D11DepthStencilView* mDepthStencilView;
	D3D11_VIEWPORT mScreenViewport;

	// Derived class should set these in derived constructor to customize starting values.
	std::wstring mMainWndCaption;
	int mClientWidth;
	int mClientHeight;
	bool mEnable4xMsaa;
	ID3D11Buffer* mVB;
	Shader mShader1, mShader2;
	ID3D11DepthStencilState* mDS1;
	ID3D11DepthStencilState* mDS2;
};

#endif // D3DAPP_H