#include <d3d11_1.h>
#include <d3dcompiler.h>

//NOTE(moritz): Only used for snprintf atm
//#include <stdio.h>

global i64 GlobalPerfCountFrequency;

internal void *
Win32AllocateMemory(size_t Size, LPVOID BaseAddress = 0)
{
	void *Result = VirtualAlloc(BaseAddress, Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	return(Result);
}

internal void
Win32DeallocateMemory(void *Memory)
{
	VirtualFree(Memory, 0, MEM_RELEASE);
}

internal u32
SafeTruncateToUInt32(u64 Value)
{
	Assert(Value <= U64Max);
	u32 Result = (u32)Value;
	return Result;
}


internal read_file_result
ReadEntireFile(char *FileName)
{
	read_file_result Result = {};
	
	HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(FileHandle != INVALID_HANDLE_VALUE)
	{ 
		LARGE_INTEGER FileSize;
		if(GetFileSizeEx(FileHandle, &FileSize))
		{
			u32 FileSize32 = SafeTruncateToUInt32(FileSize.QuadPart);
			Result.Contents = Win32AllocateMemory(FileSize32);
			if(Result.Contents)
			{
				DWORD BytesRead;
				if(ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
				   (FileSize32 == BytesRead))
				{
					//NOTE(moritz): File read successfully
					Result.ContentsSize = FileSize32;
				}
				else
				{
					Win32DeallocateMemory(Result.Contents);
					Result = {};
				}
			}
		}
	}
	
	return(Result);
}

//Timing stuff for windows
internal LARGE_INTEGER
Win32GetWallClock(void)
{
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);
	return(Result);
}

internal real32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	real32 Result = ((real32)(End.QuadPart - Start.QuadPart) /
					 (real32)GlobalPerfCountFrequency);
	return(Result);
}

internal u64
LockedAddAndReturnPreviousValue(u64 volatile *Value, u64 Addend)
{
	u64 Result = InterlockedExchangeAdd64((volatile LONG64 *)Value, Addend);
	return(Result);
}

internal u64
LockedCompareExchangeAndReturnPreviousValue(u64 volatile *Value, u64 New, u64 Expected)
{
	u64 Result = InterlockedCompareExchange64((volatile LONG64 *)Value, New, Expected);
	return(Result);
}

internal DWORD WINAPI
WorkerThread(void *lpParameter)
{
	work_queue *Queue = (work_queue *)lpParameter;
	
	for(;;)
	{
		if(!RenderTile(Queue)) 
		{
			WaitForSingleObject(Queue->SemaphoreHandle, INFINITE);
		}
	}
}

internal u32
GetCPUCoreCount(void)
{
	SYSTEM_INFO Info;
	GetSystemInfo(&Info);
	u32 Result = Info.dwNumberOfProcessors;
	return(Result);
}

internal void
CreateWorkThread(void *Parameter)
{
	DWORD ThreadID;
	HANDLE ThreadHandle = CreateThread(0, 0, WorkerThread, Parameter, 0, &ThreadID );
	CloseHandle(ThreadHandle);
}

LRESULT CALLBACK 
WindowProc(HWND Window, UINT Msg, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    switch(Msg)
    {
        case WM_KEYDOWN:
		case WM_KEYUP:
        {
			bool IsDown = (Msg == WM_KEYDOWN);
            if(WParam == VK_ESCAPE)
                DestroyWindow(Window);
            else if(WParam == 'W')
				GlobalKeyIsDown[UserAction_MoveForward] = IsDown;
			else if(WParam == 'A')
				GlobalKeyIsDown[UserAction_MoveLeft]    = IsDown;
            else if(WParam == 'S')
				GlobalKeyIsDown[UserAction_MoveBack]    = IsDown;
            else if(WParam == 'D')
				GlobalKeyIsDown[UserAction_MoveRight]   = IsDown;
			else if(WParam == VK_SPACE)
				GlobalKeyIsDown[UserAction_MoveUp]      = IsDown;
			else if(WParam == 'C')
				GlobalKeyIsDown[UserAction_MoveDown]    = IsDown;
			else if(WParam == VK_LEFT)
				GlobalKeyIsDown[UserAction_TurnLeft]    = IsDown;
            else if(WParam == VK_RIGHT)
				GlobalKeyIsDown[UserAction_TurnRight]   = IsDown;
			else if(WParam == VK_UP)
				GlobalKeyIsDown[UserAction_TurnUp]      = IsDown;
			else if(WParam == VK_DOWN)
				GlobalKeyIsDown[UserAction_TurnDown]    = IsDown;
        } break;
		
        case WM_DESTROY:
        {
            PostQuitMessage(0);
        }break;
		
        default:
		Result = DefWindowProcW(Window, Msg, WParam, LParam);
    }
    return Result;
}

internal HWND
Win32CreateWindow(wchar_t *WindowTitle, s32 WindowWidth, s32 WindowHeight)
{
	HWND Window;
	HINSTANCE Instance = GetModuleHandle(0);
	{
		WNDCLASSEXW WindowClass = {};
		WindowClass.cbSize = sizeof(WNDCLASSEXW);
		WindowClass.style = CS_HREDRAW | CS_VREDRAW;
		WindowClass.lpfnWndProc = &WindowProc; 
		WindowClass.hInstance = Instance;
		WindowClass.hIcon = LoadIconW(0, IDI_APPLICATION);
		WindowClass.hCursor = LoadCursorW(0, IDC_ARROW);
		WindowClass.lpszClassName = L"TopGWindowClass";
		WindowClass.hIconSm = LoadIconW(0, IDI_APPLICATION);
		
		if(!RegisterClassExW(&WindowClass)) 
		{
			MessageBoxA(0, "RegisterClassEx failed", "Fatal Error", MB_OK);
			ExitProcess(GetLastError());
		}
		
		RECT initialRect = {0, 0, WindowWidth, WindowHeight};
		AdjustWindowRectEx(&initialRect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);
		LONG initialWidth = initialRect.right - initialRect.left;
		LONG initialHeight = initialRect.bottom - initialRect.top;
		
		Window = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW,
								 WindowClass.lpszClassName,
								 WindowTitle,
								 WS_OVERLAPPEDWINDOW | WS_VISIBLE,
								 CW_USEDEFAULT, CW_USEDEFAULT,
								 initialWidth, 
								 initialHeight,
								 0, 0, Instance, 0);
		
		if(!Window) 
		{
			MessageBoxA(0, "CreateWindowEx failed", "Fatal Error", MB_OK);
			ExitProcess(GetLastError());
		}
	}
	
	return(Window);
}

struct d3d11_state
{
	ID3D11DeviceContext1 *DeviceContext;
	ID3D11RenderTargetView *FrameBufferView;
	ID3D11InputLayout *InputLayout;
	ID3D11VertexShader *VertexShader;
	ID3D11PixelShader *PixelShader;
	ID3D11Texture2D *RenderResultTexture;
	ID3D11ShaderResourceView *RenderResultTextureView;
	ID3D11SamplerState *SamplerState;
	IDXGISwapChain1 *SwapChain;
	
	ID3D11Buffer *VertexBuffer;
	UINT VertexBufferStride;
	UINT VertexBufferOffset;
	UINT VertexBufferNumVerts;
};

internal void
InitSimpleD3D11(d3d11_state *D3D11, HWND Window,
				s32 RenderWidth, s32 RenderHeight)
{
	// Create D3D11 Device and Context
	ID3D11Device1* D3D11Device;
	ID3D11DeviceContext1* D3D11DeviceContext;
	{
		ID3D11Device* baseDevice;
		ID3D11DeviceContext* baseDeviceContext;
		D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
		UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef MINIRAY_DEBUG
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		
		HRESULT hResult = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 
											0, creationFlags, 
											featureLevels, ARRAYSIZE(featureLevels), 
											D3D11_SDK_VERSION, &baseDevice, 
											0, &baseDeviceContext);
		if(FAILED(hResult)){
			MessageBoxA(0, "D3D11CreateDevice() failed", "Fatal Error", MB_OK);
			ExitProcess(GetLastError());
		}
		
		// Get 1.1 interface of D3D11 Device and Context
		hResult = baseDevice->QueryInterface(__uuidof(ID3D11Device1), (void**)&D3D11Device);
		Assert(SUCCEEDED(hResult));
		baseDevice->Release();
		
		hResult = baseDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&D3D11DeviceContext);
		Assert(SUCCEEDED(hResult));
		baseDeviceContext->Release();
	}
	
#ifdef MINIRAY_DEBUG
	// Set up debug layer to break on D3D11 errors
	ID3D11Debug *d3dDebug = nullptr;
	D3D11Device->QueryInterface(__uuidof(ID3D11Debug), (void**)&d3dDebug);
	if (d3dDebug)
	{
		ID3D11InfoQueue *d3dInfoQueue = nullptr;
		if (SUCCEEDED(d3dDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&d3dInfoQueue)))
		{
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
			d3dInfoQueue->Release();
		}
		d3dDebug->Release();
	}
#endif
	
	// Create Swap Chain
	IDXGISwapChain1* D3D11SwapChain;
	{ 
		// Get DXGI Factory (needed to create Swap Chain)
		IDXGIFactory2 *dxgiFactory;
		{
			IDXGIDevice1* dxgiDevice;
			HRESULT hResult = D3D11Device->QueryInterface(__uuidof(IDXGIDevice1), (void**)&dxgiDevice);
			Assert(SUCCEEDED(hResult));
			
			IDXGIAdapter* dxgiAdapter;
			hResult = dxgiDevice->GetAdapter(&dxgiAdapter);
			Assert(SUCCEEDED(hResult));
			dxgiDevice->Release();
			
			DXGI_ADAPTER_DESC adapterDesc;
			dxgiAdapter->GetDesc(&adapterDesc);
			
			OutputDebugStringA("Graphics Device: ");
			OutputDebugStringW(adapterDesc.Description);
			
			hResult = dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)&dxgiFactory);
			Assert(SUCCEEDED(hResult));
			dxgiAdapter->Release();
		}
		
		DXGI_SWAP_CHAIN_DESC1 d3d11SwapChainDesc = {};
		d3d11SwapChainDesc.Width = 0; // use window width
		d3d11SwapChainDesc.Height = 0; // use window height
		d3d11SwapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		d3d11SwapChainDesc.SampleDesc.Count = 1;
		d3d11SwapChainDesc.SampleDesc.Quality = 0;
		d3d11SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		d3d11SwapChainDesc.BufferCount = 2;
		d3d11SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
		d3d11SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		d3d11SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		d3d11SwapChainDesc.Flags = 0;
		
		HRESULT hResult = dxgiFactory->CreateSwapChainForHwnd(D3D11Device, Window, &d3d11SwapChainDesc, 0, 0, &D3D11SwapChain);
		Assert(SUCCEEDED(hResult));
		
		dxgiFactory->Release();
	}
	
	// Create Framebuffer Render Target
	ID3D11RenderTargetView *D3D11FrameBufferView;
	{
		ID3D11Texture2D* d3d11FrameBuffer;
		HRESULT hResult = D3D11SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&d3d11FrameBuffer);
		Assert(SUCCEEDED(hResult));
		
		hResult = D3D11Device->CreateRenderTargetView(d3d11FrameBuffer, 0, &D3D11FrameBufferView);
		Assert(SUCCEEDED(hResult));
		d3d11FrameBuffer->Release();
	}
	
	UINT Flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR | D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;
	
#ifdef MINIRAY_DEBUG
	Flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	Flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
	
	// Create Vertex Shader
	ID3DBlob* vsBlob;
	ID3D11VertexShader* VertexShader;
	{
		ID3DBlob* Error;
		HRESULT hResult = D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "vs_main", "vs_5_0", Flags, 0, &vsBlob, &Error);
		if(FAILED(hResult))
		{
			const char* message = (const char *)Error->GetBufferPointer();
			OutputDebugStringA(message);
			Assert(!"Failed to compile vertex shader!");
		}
		
		hResult = D3D11Device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &VertexShader);
		Assert(SUCCEEDED(hResult));
	}
	
	// Create Pixel Shader
	ID3D11PixelShader* PixelShader;
	{
		ID3DBlob *psBlob;
		ID3DBlob *Error;
		HRESULT hResult = D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "ps_main", "ps_5_0", Flags, 0, &psBlob, &Error);
		if(FAILED(hResult))
		{
			const char* message = (const char *)Error->GetBufferPointer();
			OutputDebugStringA(message);
			Assert(!"Failed to compile pixel shader!");
		}
		
		hResult = D3D11Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &PixelShader);
		Assert(SUCCEEDED(hResult));
		psBlob->Release();
	}
	
	// Create Input Layout
	ID3D11InputLayout* inputLayout;
	{
		D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
		{
			{ "POS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};
		
		HRESULT hResult = D3D11Device->CreateInputLayout(inputElementDesc, ARRAYSIZE(inputElementDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);
		Assert(SUCCEEDED(hResult));
		vsBlob->Release();
	}
	
	// Create Vertex Buffer
	ID3D11Buffer* VertexBuffer;
	UINT numVerts;
	UINT stride;
	UINT offset;
	{
		float vertexData[] = 
		{ // x, y, u, v
			-1.0f,  1.0f, 0.f, 1.f,
			
			1.0f, -1.0f, 1.f, 0.f,
			
			-1.0f, -1.0f, 0.f, 0.f,
			
			-1.0f,  1.0f, 0.f, 1.f,
			
			1.0f,  1.0f, 1.f, 1.f,
			
			1.0f, -1.0f, 1.f, 0.f
		};
		
		stride = 4 * sizeof(float);
		numVerts = sizeof(vertexData) / stride;
		offset = 0;
		
		D3D11_BUFFER_DESC vertexBufferDesc = {};
		vertexBufferDesc.ByteWidth = sizeof(vertexData);
		vertexBufferDesc.Usage     = D3D11_USAGE_IMMUTABLE;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		
		D3D11_SUBRESOURCE_DATA vertexSubresourceData = { vertexData };
		
		HRESULT hResult = D3D11Device->CreateBuffer(&vertexBufferDesc, &vertexSubresourceData, &VertexBuffer);
		Assert(SUCCEEDED(hResult));
	}
	
	// Create Sampler State
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU       = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressW       = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.BorderColor[0] = 1.0f;
	samplerDesc.BorderColor[1] = 1.0f;
	samplerDesc.BorderColor[2] = 1.0f;
	samplerDesc.BorderColor[3] = 1.0f;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	
	ID3D11SamplerState* samplerState;
	D3D11Device->CreateSamplerState(&samplerDesc, &samplerState);
	
	//NOTE(moritz): Texture that we copy the rendering results from the CPU into.
	//And then send it to the GPU for blitting.
	ID3D11ShaderResourceView *TextureView;
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width              = RenderWidth;
	textureDesc.Height             = RenderHeight;
	textureDesc.MipLevels          = 1;
	textureDesc.ArraySize          = 1;
	textureDesc.Format             = DXGI_FORMAT_R32G32B32_FLOAT;
	textureDesc.SampleDesc.Count   = 1;
	textureDesc.Usage              = D3D11_USAGE_DYNAMIC;
	textureDesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags     = D3D11_CPU_ACCESS_WRITE;
	
	ID3D11Texture2D *Texture;
	D3D11Device->CreateTexture2D(&textureDesc, 0, &Texture);
	
	D3D11Device->CreateShaderResourceView(Texture, nullptr, &TextureView);
	
	D3D11->DeviceContext           = D3D11DeviceContext;
	D3D11->FrameBufferView         = D3D11FrameBufferView;
	D3D11->InputLayout             = inputLayout;
	D3D11->VertexShader            = VertexShader;
	D3D11->PixelShader             = PixelShader;
	D3D11->RenderResultTexture     = Texture;
	D3D11->RenderResultTextureView = TextureView;
	D3D11->SamplerState            = samplerState;
	D3D11->SwapChain               = D3D11SwapChain;
	
	D3D11->VertexBuffer            = VertexBuffer;
	D3D11->VertexBufferStride      = stride;
	D3D11->VertexBufferOffset      = offset;
	D3D11->VertexBufferNumVerts    = numVerts;
}

extern "C" void __cdecl WinMainCRTStartup(void)
{
	//NOTE(moritz): Setting math constants
	
	math_constants MathConstants = {};
	
	MathConstants.SignMaskU32 = (u32)0x80000000;
	MathConstants.InvSignMask = (u32)~0x80000000;
	
	MathConstants._ps256_sign_mask     = _mm256_set1_ps(*(float *)&MathConstants.SignMaskU32);
	MathConstants._ps256_inv_sign_mask = _mm256_set1_ps(*(float *)&MathConstants.InvSignMask);
	MathConstants._ps256_cephes_FOPI = _mm256_set1_ps(1.27323954473516);
	MathConstants._pi32_256_1 = _mm256_set1_epi32(1);
	MathConstants._pi32_256_inv1 = _mm256_set1_epi32(~1);
	MathConstants._pi32_256_0 = _mm256_set1_epi32(0);
	MathConstants._pi32_256_2 = _mm256_set1_epi32(2);
	MathConstants._pi32_256_4 = _mm256_set1_epi32(4);
	MathConstants._ps256_minus_cephes_DP1 = _mm256_set1_ps(-0.78515625);
	MathConstants._ps256_minus_cephes_DP2 = _mm256_set1_ps(-2.4187564849853515625e-4);
	MathConstants._ps256_minus_cephes_DP3 = _mm256_set1_ps(-3.77489497744594108e-8);
	MathConstants._ps256_coscof_p0  = _mm256_set1_ps(2.443315711809948E-005);
	MathConstants._ps256_coscof_p1  = _mm256_set1_ps(-1.388731625493765E-003);
	MathConstants._ps256_coscof_p2  = _mm256_set1_ps(4.166664568298827E-002);
	MathConstants._ps256_sincof_p0  = _mm256_set1_ps(-1.9515295891E-4);
	MathConstants._ps256_sincof_p1  = _mm256_set1_ps(8.3321608736E-3);
	MathConstants._ps256_sincof_p2  = _mm256_set1_ps(-1.6666654611E-1);
	
	MathConstants._ps256_1 = _mm256_set1_ps(1.0f);
	MathConstants._ps256_0p5 = _mm256_set1_ps(0.5f);
	
	///
	
	HINSTANCE Instance = GetModuleHandle(0);
	
	//NOTE(moritz): Timer setup
	LARGE_INTEGER PerfCountFrequencyResult;
	QueryPerformanceFrequency(&PerfCountFrequencyResult);
	GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;
	
	read_file_result MeshFile = ReadEntireFile("Lady.fobj");
	
	s32 RenderWidth  = 640;
	s32 RenderHeight = 480;
	
	s32 WindowWidth = 1280;
	s32 WindowHeight = 720;
	
	HWND Window = Win32CreateWindow(L"RayTracer", WindowWidth, WindowHeight);
	
	d3d11_state D3D11 = {};
	InitSimpleD3D11(&D3D11, Window, RenderWidth, RenderHeight);
	
	//Multi-threading setup
	
	/*
NOTE(moritz): This follows the Single Server, Multiple Consumer principle.
The main thread fills out the FIFO work queue. 
The worker threads are spun up at startup and grab a job (a tile to render) as soon as possible.
Grabbing work from the queue is managed via atomic operations.

Once the main thread is finished filling out the work queue and there is still work to do,
the main thread will join the worker threads in consuming the work queue.
*/
	u32 CoreCount  = GetCPUCoreCount();
	u32 TileWidth  = (RenderWidth + CoreCount - 1) / CoreCount;
	u32 TileHeight = TileWidth;
	TileWidth = TileHeight = 32;
	u32 TileCountX = (RenderWidth  + TileWidth  - 1) / TileWidth;
	u32 TileCountY = (RenderHeight + TileHeight - 1) / TileHeight;
	u32 TotalTileCount = TileCountX*TileCountY;
	
	work_queue Queue = {};
	u32 InitialCount = 0;
	Queue.MaxWorkOrderCount = TotalTileCount;
	Queue.SemaphoreHandle = CreateSemaphoreExA(0,
											   InitialCount,
											   CoreCount,
											   0,
											   0,
											   SEMAPHORE_ALL_ACCESS);
	
	
	for(u32 CoreIndex = 1;
		CoreIndex < CoreCount;
		++CoreIndex)
	{
		CreateWorkThread(&Queue);
	};
	
	
	frame_buffer FrameBuffer = {};
	FrameBuffer.Width  = RenderWidth;
	FrameBuffer.Height = RenderHeight;
	UINT FrameBufferPitch = FrameBuffer.Width*sizeof(f32)*3;
	
	umm FrameBufferSize = sizeof(v3)*FrameBuffer.Width*FrameBuffer.Height;
	
	fobj_header *Header = (fobj_header *)MeshFile.Contents;
	Assert(Header->MagicValue == FOBJ_MAGIC_VALUE);
	
	u32 TriangleCount = Header->IndexCount/3;
	umm TransformedVerticesSize = sizeof(v3)*Header->VertexCount;
	umm TransformedNormalsSize  = sizeof(v3)*Header->NormalCount;
	umm TransformedIndicesSize  = sizeof(v3i)*Header->IndexCount;
	
	umm ScratchBufferSize = Megabytes(200);
	
	umm TotalSize = FrameBufferSize + ScratchBufferSize + TransformedVerticesSize + TransformedNormalsSize + TransformedIndicesSize;
	void *Memory = Win32AllocateMemory(TotalSize, (LPVOID)Terabytes((u64)2));
	
	FrameBuffer.Memory = Memory;
	
	memory_arena *ScratchArena = InitialiseArena(ScratchBufferSize, (u8 *)FrameBuffer.Memory + FrameBufferSize);
	
	Queue.WorkOrders = PushArray(ScratchArena, Queue.MaxWorkOrderCount, work_order);
	
	world *World = 0;
	InitialiseWorld(ScratchArena, ScratchBufferSize, MeshFile, TriangleCount,
					TransformedVerticesSize, TransformedNormalsSize, TransformedIndicesSize, &MathConstants, &World);
	
	LARGE_INTEGER FrameStart = Win32GetWallClock();
	// Main Loop
	bool isRunning = true;
	while(isRunning)
	{
		f32 dt;
		{
			LARGE_INTEGER FrameEnd = FrameStart;
			
			FrameStart = Win32GetWallClock();
			dt = Win32GetSecondsElapsed(FrameEnd, FrameStart);
			
#if 0
			//NOTE(moritz): This is only available
			//when compiling with the CRT
			f32 FPS = 1.0f/dt;
			char FPSBuffer[512];
			_snprintf_s(FPSBuffer, sizeof(FPSBuffer),
						"FPS: %f\n", FPS);
			OutputDebugStringA(FPSBuffer);
#endif
		}
		
		MSG msg = {};
		while(PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
		{
			if(msg.message == WM_QUIT)
				isRunning = false;
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		
		UpdateAndRender(World, &MathConstants, FrameBuffer, FrameBufferSize,
						&Queue, TileCountX, TileCountY,
						TileWidth, TileHeight, dt);
		
		FLOAT backgroundColor[4] = { 0.1f, 0.2f, 0.6f, 1.0f };
		D3D11.DeviceContext->ClearRenderTargetView(D3D11.FrameBufferView, backgroundColor);
		
		RECT winRect;
		GetClientRect(Window, &winRect);
		D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (FLOAT)(winRect.right - winRect.left), (FLOAT)(winRect.bottom - winRect.top), 0.0f, 1.0f };
		D3D11.DeviceContext->RSSetViewports(1, &viewport);
		
		D3D11.DeviceContext->OMSetRenderTargets(1, &D3D11.FrameBufferView, nullptr);
		
		D3D11.DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		D3D11.DeviceContext->IASetInputLayout(D3D11.InputLayout);
		
		D3D11.DeviceContext->VSSetShader(D3D11.VertexShader, nullptr, 0);
		D3D11.DeviceContext->PSSetShader(D3D11.PixelShader, nullptr, 0);
		
		
		//NOTE(moritz): It's unclear wether the Map/Unmap method is the correct thing to do here...
		u8 *FrameBufferAt = (u8 *)FrameBuffer.Memory;
		D3D11_MAPPED_SUBRESOURCE MappedSubresource;
		D3D11.DeviceContext->Map(D3D11.RenderResultTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource);
		
		u8 *MappedData = (u8 *)MappedSubresource.pData;
		for(u32 RowIndex = 0;
			RowIndex < FrameBuffer.Height;
			++RowIndex)
		{
			Copy(FrameBufferAt, MappedData, FrameBufferPitch);
			MappedData += MappedSubresource.RowPitch;
			FrameBufferAt += FrameBufferPitch;
		}
		
		D3D11.DeviceContext->Unmap(D3D11.RenderResultTexture, 0);
		
		D3D11.DeviceContext->PSSetShaderResources(0, 1, &D3D11.RenderResultTextureView);
		D3D11.DeviceContext->PSSetSamplers(0, 1, &D3D11.SamplerState);
		
		D3D11.DeviceContext->IASetVertexBuffers(0, 1, &D3D11.VertexBuffer, &D3D11.VertexBufferStride, &D3D11.VertexBufferOffset);
		
		D3D11.DeviceContext->Draw(D3D11.VertexBufferNumVerts, 0);
		
		D3D11.SwapChain->Present(1, 0);
	}
	
	ExitProcess(0);
}

#if 0
//NOTE(moritz): Exporting this data using the microsoft specific extended attribute syntax
//forces the computer to use the discrete GPU in case an integrated GPU is present on the system..
extern "C" 
{
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif


extern "C"
{
	//NOTE(moritz): This value and memset have to specified when compiling without the CRT
	
	int _fltused = 0x9875;
	
#pragma function(memset)
	void * __cdecl memset (
						   void *dst,
						   int val,
						   size_t count
						   )
	{
		void *start = dst;
		while (count--) {
			*(char *)dst = (char)val;
			dst = (char *)dst + 1;
		}
		
		return(start);
	}
}
