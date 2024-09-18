#include "DirectX11GraphicsDevice.h"

#include <wv/Texture/Texture.h>
#include <wv/Material/Material.h>

#include <wv/Debug/Print.h>
#include <wv/Debug/Trace.h>

#include <wv/Decl.h>

#include <wv/Memory/FileSystem.h>

#include <wv/Math/Transform.h>

#include <wv/Primitive/Mesh.h>
#include <wv/Primitive/Primitive.h>
#include <wv/RenderTarget/RenderTarget.h>

#include <wv/Device/DeviceContext.h>

#ifdef WV_SUPPORT_D3D11
#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "DXGI.lib" )
#pragma comment( lib, "d3dcompiler.lib" )
#include <d3dcompiler.h>
#endif

#ifdef WV_SUPPORT_GLFW
#include <wv/Device/DeviceContext/GLFW/GLFWDeviceContext.h>
#include <GLFW/glfw3native.h>
#endif

#ifdef WV_SUPPORT_SDL2
#include <wv/Device/DeviceContext/SDL/SDLDeviceContext.h>
#include <SDL2/SDL_syswm.h>
#endif

#include <stdio.h>
#include <sstream>
#include <fstream>
#include <vector>


#define WV_HARD_ASSERT 0

#ifdef WV_DEBUG
	#define WV_VALIDATE_GL( _func ) if( _func == nullptr ) { Debug::Print( Debug::WV_PRINT_FATAL, "Missing function '%s'\n", #_func ); }

	#if WV_HARD_ASSERT
		#define WV_ASSERT_ERR( _msg ) if( !assertGLError( _msg ) ) throw std::runtime_error( _msg )
	#else
		#define WV_ASSERT_ERR( _msg ) assertGLError( _msg ) 
	#endif
#else
	#define WV_VALIDATE_GL( _func )
	#define WV_ASSERT_ERR( _msg ) 
#endif


#ifdef WV_SUPPORT_D3D11
std::string getErrorMessage( HRESULT _hr )
{
	LPTSTR error_text = NULL;
	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM |
				   FORMAT_MESSAGE_ALLOCATE_BUFFER |
				   FORMAT_MESSAGE_IGNORE_INSERTS,
				   NULL,
				   _hr,
				   MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
				   (LPTSTR)&error_text, 0,
				   NULL );

	return std::string(error_text);
	/*
	bool severity = ( _hr & 0b1 ) != 0;
	bool customer = ( _hr & 0b100 ) != 0;
	bool ntstatus = ( _hr & 0b1000 ) != 0;
	int facility = ( _hr & 0b1111111111100000 ) >> 5;

	std::string result = "";
	result += severity ? "(S) " : "";
	result += customer ? "(C) " : "";
	result += ntstatus ? "(N) " : "";
	switch ( facility )
	{
	case FACILITY_NULL: result += "(FACILITY_NULL) The default facility code."; break;
	case FACILITY_RPC: result += "(FACILITY_RPC) The source of the error code is an RPC subsystem."; break;
	case FACILITY_DISPATCH: result += "(FACILITY_DISPATCH) The source of the error code is a COM Dispatch."; break;
	case FACILITY_STORAGE: result += "(FACILITY_STORAGE) The source of the error code is OLE Storage."; break;
	case FACILITY_ITF: result += "(FACILITY_ITF) The source of the error code is COM/OLE Interface management."; break;
	case FACILITY_WIN32: result += "(FACILITY_WIN32) This region is reserved to map undecorated error codes into HRESULTs."; break;
	case FACILITY_WINDOWS: result += "(FACILITY_WINDOWS) The source of the error code is the Windows subsystem."; break;
	case FACILITY_SECURITY: result += "(FACILITY_SECURITY) The source of the error code is the Security API layer."; break;
	// case FACILITY_SSPI: result += "(FACILITY_SSPI) The source of the error code is the Security API layer."; break;
	case FACILITY_CONTROL: result += "(FACILITY_CONTROL) The source of the error code is the control mechanism."; break;
	case FACILITY_CERT: result += "(FACILITY_CERT) The source of the error code is a certificate client or server? "; break;
	case FACILITY_INTERNET: result += "(FACILITY_INTERNET) The source of the error code is Wininet related."; break;
	case FACILITY_MEDIASERVER: result += "(FACILITY_MEDIASERVER) The source of the error code is the Windows Media Server."; break;
	case FACILITY_MSMQ: result += "(FACILITY_MSMQ) The source of the error code is the Microsoft Message Queue."; break;
	case FACILITY_SETUPAPI: result += "(FACILITY_SETUPAPI) The source of the error code is the Setup API."; break;
	case FACILITY_SCARD: result += "(FACILITY_SCARD) The source of the error code is the Smart-card subsystem."; break;
	case FACILITY_COMPLUS: result += "(FACILITY_COMPLUS) The source of the error code is COM+."; break;
	case FACILITY_AAF: result += "(FACILITY_AAF) The source of the error code is the Microsoft agent."; break;
	case FACILITY_URT: result += "(FACILITY_URT) The source of the error code is .NET CLR."; break;
	case FACILITY_ACS: result += "(FACILITY_ACS) The source of the error code is the audit collection service."; break;
	case FACILITY_DPLAY: result += "(FACILITY_DPLAY) The source of the error code is Direct Play."; break;
	case FACILITY_UMI: result += "(FACILITY_UMI) The source of the error code is the ubiquitous memoryintrospection service."; break;
	case FACILITY_SXS: result += "(FACILITY_SXS) The source of the error code is Side-by-side servicing."; break;
	case FACILITY_WINDOWS_CE: result += "(FACILITY_WINDOWS_CE) The error code is specific to Windows CE."; break;
	case FACILITY_HTTP: result += "(FACILITY_HTTP) The source of the error code is HTTP support."; break;
	case FACILITY_USERMODE_COMMONLOG: result += "(FACILITY_USERMODE_COMMONLOG) The source of the error code is common Logging support."; break;
	case FACILITY_USERMODE_FILTER_MANAGER: result += "(FACILITY_USERMODE_FILTER_MANAGER) The source of the error code is the user mode filter manager."; break;
	case FACILITY_BACKGROUNDCOPY: result += "(FACILITY_BACKGROUNDCOPY) The source of the error code is background copy control"; break;
	case FACILITY_CONFIGURATION: result += "(FACILITY_CONFIGURATION) The source of the error code is configuration services."; break;
	case FACILITY_STATE_MANAGEMENT: result += "(FACILITY_STATE_MANAGEMENT) The source of the error code is state management services."; break;
	case FACILITY_METADIRECTORY: result += "(FACILITY_METADIRECTORY) The source of the error code is the Microsoft Identity Server."; break;
	case FACILITY_WINDOWSUPDATE: result += "(FACILITY_WINDOWSUPDATE) The source of the error code is a Windows update."; break;
	case FACILITY_DIRECTORYSERVICE: result += "(FACILITY_DIRECTORYSERVICE) The source of the error code is Active Directory."; break;
	case FACILITY_GRAPHICS: result += "(FACILITY_GRAPHICS) The source of the error code is the graphics drivers."; break;
	case FACILITY_SHELL: result += "(FACILITY_SHELL) The source of the error code is the user Shell."; break;
	case FACILITY_TPM_SERVICES: result += "(FACILITY_TPM_SERVICES) The source of the error code is the Trusted Platform Module services."; break;
	case FACILITY_TPM_SOFTWARE: result += "(FACILITY_TPM_SOFTWARE) The source of the error code is the Trusted Platform Module applications."; break;
	case FACILITY_PLA: result += "(FACILITY_PLA) The source of the error code is Performance Logs and Alerts"; break;
	case FACILITY_FVE: result += "(FACILITY_FVE) The source of the error code is Full volume encryption."; break;
	case FACILITY_FWP: result += "(FACILITY_FWP) he source of the error code is the Firewall Platform."; break;
	case FACILITY_WINRM: result += "(FACILITY_WINRM) The source of the error code is the Windows Resource Manager."; break;
	case FACILITY_NDIS: result += "(FACILITY_NDIS) The source of the error code is the Network Driver Interface."; break;
	case FACILITY_USERMODE_HYPERVISOR: result += "(FACILITY_USERMODE_HYPERVISOR) The source of the error code is the Usermode Hypervisor components."; break;
	case FACILITY_CMI: result += "(FACILITY_CMI) The source of the error code is the Configuration Management Infrastructure."; break;
	case FACILITY_USERMODE_VIRTUALIZATION: result += "(FACILITY_USERMODE_VIRTUALIZATION) The source of the error code is the user mode virtualization subsystem."; break;
	case FACILITY_USERMODE_VOLMGR: result += "(FACILITY_USERMODE_VOLMGR) The source of the error code is  the user mode volume manager"; break;
	case FACILITY_BCD: result += "(FACILITY_BCD) The source of the error code is the Boot Configuration Database."; break;
	case FACILITY_USERMODE_VHD: result += "(FACILITY_USERMODE_VHD) The source of the error code is user mode virtual hard disk support."; break;
	case FACILITY_SDIAG: result += "(FACILITY_SDIAG) The source of the error code is System Diagnostics."; break;
	case FACILITY_WEBSERVICES: result += "(FACILITY_WEBSERVICES) The source of the error code is the Web Services."; break;
	case FACILITY_WINDOWS_DEFENDER: result += "(FACILITY_WINDOWS_DEFENDER) The source of the error code is a Windows Defender component."; break;
	case FACILITY_OPC: result += "(FACILITY_OPC) The source of the error code is the open connectivity service."; break;
	default: result += "(UNKNOWN) Unknown facility " + std::to_string(facility); break;
	}

	return result;
	*/
}
#endif

///////////////////////////////////////////////////////////////////////////////////////

wv::cDirectX11GraphicsDevice::cDirectX11GraphicsDevice()
{
	WV_TRACE();

	IDXGIFactory* pFactory = nullptr;

	// Create a DXGIFactory object.
	HRESULT hr = CreateDXGIFactory( __uuidof( IDXGIFactory ), reinterpret_cast<void**>( &pFactory ) );
	if ( FAILED( hr ) )
	{
		Debug::Print( Debug::WV_PRINT_ERROR, "Failed to create DXGIFactory for enumerating adapters. (%d)\n", hr );
		exit( -1 );
	}

	IDXGIAdapter* pAdapter;
	UINT index = 0;
	while ( SUCCEEDED( pFactory->EnumAdapters( index, &pAdapter ) ) )
	{
		m_d3d11adapters.push_back( sD3D11AdapterData{ pAdapter } );
		index += 1;
	}

	pFactory->Release();
}

///////////////////////////////////////////////////////////////////////////////////////

HWND getHwndFromDeviceDesc( wv::iDeviceContext* _context )
{
	switch ( _context->getContextAPI() )
	{
#ifdef WV_SUPPORT_GLFW
	case wv::WV_DEVICE_CONTEXT_API_GLFW:
	{
		auto* pointer = dynamic_cast<wv::GLFWDeviceContext*>( _context );
		if ( pointer == nullptr )
			return HWND( 0 );

		return glfwGetWin32Window( pointer->m_windowContext );
	}
#endif
#ifdef WV_SUPPORT_SDL2
	case wv::WV_DEVICE_CONTEXT_API_SDL:
	{
		auto* pointer = dynamic_cast<wv::SDLDeviceContext*>( _context );
		if ( pointer == nullptr )
			return HWND( 0 );

		SDL_SysWMinfo wmInfo;
		SDL_VERSION( &wmInfo.version );
		SDL_GetWindowWMInfo( pointer->m_windowContext, &wmInfo );
		return wmInfo.info.win.window;
	}
#endif
	default:
		return HWND( 0 );
	}
}

///////////////////////////////////////////////////////////////////////////////////////

bool wv::cDirectX11GraphicsDevice::initialize( GraphicsDeviceDesc* _desc )
{
	WV_TRACE();

#ifdef WV_SUPPORT_D3D11
	m_graphicsApi = _desc->pContext->getGraphicsAPI();
	
	Debug::Print( Debug::WV_PRINT_DEBUG, "Initializing Graphics Device...\n" );

	int initRes = 0;
	switch ( m_graphicsApi )
	{
	case WV_GRAPHICS_API_D3D11: initRes = 1; break;
	}
	if ( !initRes )
	{
		Debug::Print( Debug::WV_PRINT_FATAL, "Failed to initialize Graphics Device\n" );
		return false;
	}

	Debug::Print( Debug::WV_PRINT_INFO, "Intialized Graphics Device\n" );
	// Debug::Print( Debug::WV_PRINT_INFO, "  %s\n", glGetString( GL_VERSION ) );

	m_graphicsApiVersion.major = 11;
	m_graphicsApiVersion.minor = 0;
	
	int width = _desc->pContext->getWidth();
	int height = _desc->pContext->getHeight();
	int fps = 60;

	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory( &scd, sizeof( DXGI_SWAP_CHAIN_DESC ) );

	scd.BufferDesc.Width = width;
	scd.BufferDesc.Height = height;
	scd.BufferDesc.RefreshRate.Numerator = fps;
	scd.BufferDesc.RefreshRate.Denominator = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	scd.SampleDesc.Count = 1;
	scd.SampleDesc.Quality = 0;

	HWND window = getHwndFromDeviceDesc( _desc->pContext );
	if (window == 0)
	{
		return false;
	}

	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.BufferCount = 1;
	scd.OutputWindow = window;
	scd.Windowed = TRUE;
	scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	HRESULT hr;
	hr = D3D11CreateDeviceAndSwapChain( m_d3d11adapters[ 0 ].pAdapter, //IDXGI Adapter
										D3D_DRIVER_TYPE_UNKNOWN,
										NULL, //FOR SOFTWARE DRIVER TYPE
										D3D11_CREATE_DEVICE_DEBUG, //FLAGS FOR RUNTIME LAYERS
										NULL, //FEATURE LEVELS ARRAY
										0,    //# OF FEATURE LEVELS IN ARRAY
										D3D11_SDK_VERSION,
										&scd,               //Swapchain description
										&m_swapChain,       //Swapchain Address
										&m_device,          //Device Address
										NULL,               //Supported feature level
										&m_deviceContext ); //Device Context Address

	if ( FAILED( hr ) )
	{
		Debug::Print( Debug::WV_PRINT_FATAL, "Failed to create device and swapchain. (%d)\n", hr );
		return false;
	}

	hr = m_swapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), reinterpret_cast<void**>( &m_backBuffer ) );
	if ( FAILED( hr ) ) //If error occurred
	{
		( Debug::WV_PRINT_FATAL, "GetBuffer Failed. (%d)\n", hr );
		return false;
	}

	hr = m_device->CreateRenderTargetView( m_backBuffer, NULL, &m_renderTargetView );
	if ( FAILED( hr ) ) //If error occurred
	{
		Debug::Print( Debug::WV_PRINT_FATAL, "Failed to create render target view. (%d)\n", hr );
		return false;
	}

	m_deviceContext->OMSetRenderTargets( 1, &m_renderTargetView, NULL );

	
	ID3D11Debug* d3dDebug = nullptr;
	m_device->QueryInterface( __uuidof( ID3D11Debug ), (void**)&d3dDebug );
	if ( d3dDebug )
	{
		ID3D11InfoQueue* d3dInfoQueue = nullptr;
		if ( SUCCEEDED( d3dDebug->QueryInterface( __uuidof( ID3D11InfoQueue ), (void**)&d3dInfoQueue ) ) )
		{
			// d3dInfoQueue->SetBreakOnSeverity( D3D11_MESSAGE_SEVERITY_CORRUPTION, true );
			// d3dInfoQueue->SetBreakOnSeverity( D3D11_MESSAGE_SEVERITY_ERROR, true );
			d3dInfoQueue->Release();
		}
		d3dDebug->Release();
	}

	// int numTextureUnits = 0;
	// m_boundTextureSlots.assign( numTextureUnits, 0 );
	return true;
#else
	return false;
#endif
}

void wv::cDirectX11GraphicsDevice::terminate()
{
	WV_TRACE();
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::onResize( int _width, int _height )
{
	WV_TRACE();
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::setViewport( int _width, int _height )
{
	WV_TRACE();

#ifdef WV_SUPPORT_D3D11
	D3D11_VIEWPORT viewport = {
		0.0f,
		0.0f,
		_width,
		_height,
		0.0f,
		1.0f };
	m_deviceContext->RSSetViewports( 1, &viewport );
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

wv::RenderTarget* wv::cDirectX11GraphicsDevice::createRenderTarget( RenderTargetDesc* _desc )
{
	WV_TRACE();

#ifdef WV_SUPPORT_D3D11
	RenderTargetDesc& desc = *_desc;
	RenderTarget* target = new RenderTarget();
	
	/*
	glGenFramebuffers( 1, &target->fbHandle );
	glBindFramebuffer( GL_FRAMEBUFFER, target->fbHandle );
	
	target->numTextures = desc.numTextures;
	GLenum* drawBuffers = new GLenum[ desc.numTextures ];
	target->textures = new Texture * [ desc.numTextures ];
	for ( int i = 0; i < desc.numTextures; i++ )
	{
		desc.pTextureDescs[ i ].width = desc.width;
		desc.pTextureDescs[ i ].height = desc.height;

		std::string texname = "buffer_tex" + std::to_string( i );

		target->textures[ i ] = new Texture( texname );
		createTexture( target->textures[i], &desc.pTextureDescs[i] );

		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, target->textures[ i ]->getHandle(), 0);

		drawBuffers[ i ] = GL_COLOR_ATTACHMENT0 + i;
	}

	glGenRenderbuffers( 1, &target->rbHandle );
	glBindRenderbuffer( GL_RENDERBUFFER, target->rbHandle );
	glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, desc.width, desc.height );
	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, target->rbHandle );
	
	glDrawBuffers( desc.numTextures, drawBuffers );
	delete[] drawBuffers;
	*/
	target->width  = desc.width;
	target->height = desc.height;

	// glBindRenderbuffer( GL_RENDERBUFFER, 0 );
	// glBindTexture( GL_TEXTURE_2D, 0 );
	// glBindFramebuffer( GL_FRAMEBUFFER, 0 );

	return target;
#else
	return nullptr;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::destroyRenderTarget( RenderTarget** _renderTarget )
{
	WV_TRACE();

#ifdef WV_SUPPORT_OPENGL_TEMP
	RenderTarget* rt = *_renderTarget;

	glDeleteFramebuffers( 1, &rt->fbHandle );
	glDeleteRenderbuffers( 1, &rt->rbHandle );

	for ( int i = 0; i < rt->numTextures; i++ )
		destroyTexture( &rt->textures[ i ] );
	
	*_renderTarget = nullptr;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::setRenderTarget( RenderTarget* _target )
{
	WV_TRACE();

#ifdef WV_SUPPORT_OPENGL_TEMP
	if ( m_activeRenderTarget == _target )
		return;

	unsigned int handle = _target ? _target->fbHandle : 0;
	
	glBindFramebuffer( GL_FRAMEBUFFER, handle );
	if ( _target )
	{
		glViewport( 0, 0, _target->width, _target->height );
	}
	
	m_activeRenderTarget = _target;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::setClearColor( const wv::cColor& _color )
{
	WV_TRACE();

#ifdef WV_SUPPORT_D3D11
	m_clearBackgroundColor[ 0 ] = _color.r;
	m_clearBackgroundColor[ 1 ] = _color.g;
	m_clearBackgroundColor[ 2 ] = _color.b;
	m_clearBackgroundColor[ 3 ] = _color.a;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::clearRenderTarget( bool _color, bool _depth )
{
	WV_TRACE();

#ifdef WV_SUPPORT_OPENGL_TEMP
	glClear( (GL_COLOR_BUFFER_BIT * _color) | (GL_DEPTH_BUFFER_BIT * _depth) );
#endif
}

wv::sShaderProgram* wv::cDirectX11GraphicsDevice::createProgram( sShaderProgramDesc* _desc )
{
	eShaderProgramType&   type   = _desc->type;
	sShaderProgramSource& source = _desc->source;

#ifdef WV_SUPPORT_D3D11
	if ( source.data->size == 0 )
	{
		Debug::Print( Debug::WV_PRINT_ERROR, "Cannot compile shader with null source\n" );
		return nullptr;
	}

	WV_TRACE();

	HRESULT hr;

	if ( type == eShaderProgramType::WV_SHADER_TYPE_VERTEX )
	{
		const char* pEntrypoint = "main";
		ID3DBlob* errorBlob;
		ID3DBlob* shaderBlob;
		if ( FAILED( D3DCompile(
			_desc->source.data->data,
			_desc->source.data->size,
			nullptr,
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			pEntrypoint,
			"vs_5_0",
			D3DCOMPILE_ENABLE_STRICTNESS,
			0,
		    &shaderBlob,
			&errorBlob
			) ) )
		{
			Debug::Print( Debug::WV_PRINT_ERROR, "D3D11 Failed to read Vertex shader\n" );
			if ( errorBlob != nullptr )
			{
				Debug::Print( Debug::WV_PRINT_ERROR, "Message: %s\n", (char*) errorBlob->GetBufferPointer() );
				errorBlob->Release();
			}
			return nullptr;
		}

		ID3D11VertexShader* vsShader;
		hr = m_device->CreateVertexShader(
			shaderBlob->GetBufferPointer(),
			shaderBlob->GetBufferSize(),
			nullptr,
			&vsShader);
		if ( FAILED( hr ) )
		{
			Debug::Print( Debug::WV_PRINT_FATAL, "Failed to compile vertex shader. (%d)\n", hr );
			return nullptr;
		}

		m_vertexShaderMap[ ++m_vertexShaders ] = vsShader;
		m_vertexShaderBlobMap[ m_vertexShaders ] = shaderBlob;
		sShaderProgram* program = new sShaderProgram();
		program->handle = m_vertexShaders;
		// Debug::Print( Debug::WV_PRINT_INFO, "Success compiling vertex shader\n" );
		return program;
	}
	else if ( type == eShaderProgramType::WV_SHADER_TYPE_FRAGMENT )
	{
		const char* pEntrypoint = "main";
		ID3DBlob* errorBlob;
		ID3DBlob* shaderBlob;
		if ( FAILED( D3DCompile(
			_desc->source.data->data,
			_desc->source.data->size,
			nullptr,
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			pEntrypoint,
			"ps_5_0",
			D3DCOMPILE_ENABLE_STRICTNESS,
			0,
			&shaderBlob,
			&errorBlob ) ) )
		{
			Debug::Print( Debug::WV_PRINT_ERROR, "D3D11 Failed to read fragment shader\n" );
			if ( errorBlob != nullptr )
			{
				Debug::Print( Debug::WV_PRINT_ERROR, "Message: %s\n", (char*) errorBlob->GetBufferPointer() );
				errorBlob->Release();
			}
			return nullptr;
		}

		ID3D11PixelShader* piShader;
		hr = m_device->CreatePixelShader(
			shaderBlob->GetBufferPointer(),
			shaderBlob->GetBufferSize(),
			nullptr,
			&piShader );
		if ( FAILED( hr ) )
		{
			Debug::Print( Debug::WV_PRINT_FATAL, "Failed to create fragment shader. (%d)\n", hr );
			return nullptr;
		}

		m_pixelShaderMap[ ++m_pixelShaders ] = piShader;
		sShaderProgram* program = new sShaderProgram();
		program->handle = m_pixelShaders;
		// Debug::Print( Debug::WV_PRINT_INFO, "Success compiling fragment shader\n" );
		shaderBlob->Release();
		return program;
	}

	return nullptr;
#else
	return nullptr;
#endif
}

void wv::cDirectX11GraphicsDevice::destroyProgram( sShaderProgram* _pProgram )
{
	WV_TRACE();

#ifdef WV_SUPPORT_D3D11
	if ( _pProgram->type == eShaderProgramType::WV_SHADER_TYPE_VERTEX )
	{
		auto iter = m_vertexShaderMap.find( _pProgram->handle );
		if ( iter != m_vertexShaderMap.end() )
		{
			iter->second->Release();
			m_vertexShaderMap.erase( iter );
		}

		auto iter2 = m_vertexShaderBlobMap.find( _pProgram->handle );
		if ( iter2 != m_vertexShaderBlobMap.end() )
		{
			iter->second->Release();
			m_vertexShaderBlobMap.erase( iter2 );
		}
	}
	else if ( _pProgram->type == eShaderProgramType::WV_SHADER_TYPE_FRAGMENT )
	{
		auto iter = m_pixelShaderMap.find( _pProgram->handle );
		if ( iter != m_pixelShaderMap.end() )
		{
			iter->second->Release();
			m_pixelShaderMap.erase( iter );
		}
	}
	_pProgram->handle = 0;
#endif
}

wv::sPipeline* wv::cDirectX11GraphicsDevice::createPipeline( sPipelineDesc* _desc )
{
	WV_TRACE();

	sPipelineDesc& desc = *_desc;
	
#ifdef WV_SUPPORT_D3D11
	ID3D11InputLayout* inputLayout;
	
	/*
	wv::sVertexAttribute attributes[] = {
		{ "aPosition", 3, wv::WV_FLOAT, false, sizeof( float ) * 3 }, // vec3f pos
		{ "aNormal", 3, wv::WV_FLOAT, false, sizeof( float ) * 3 },   // vec3f normal
		{ "aTangent", 3, wv::WV_FLOAT, false, sizeof( float ) * 3 },  // vec3f tangent
		{ "aColor", 4, wv::WV_FLOAT, false, sizeof( float ) * 4 },    // vec4f col
		{ "aTexCoord0", 2, wv::WV_FLOAT, false, sizeof( float ) * 2 } // vec2f texcoord0
	};
	*/

	wv::Handle vertexHandle = ( *_desc->pVertexProgram )->handle;
	auto iter = m_vertexShaderBlobMap.find(vertexHandle);
	if ( iter == m_vertexShaderBlobMap.end() )
	{
		Debug::Print( Debug::WV_PRINT_FATAL, "Failed to create input layout could not find vertex shader blob\n" );
		return nullptr;
	}

	D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 2, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	HRESULT hr;


	hr = m_device->CreateInputLayout(
		inputElementDesc,
		ARRAYSIZE( inputElementDesc ),
		iter->second->GetBufferPointer(),
		iter->second->GetBufferSize(),
		&inputLayout );
	if ( FAILED( hr ) )
	{
		Debug::Print( Debug::WV_PRINT_FATAL, "Failed to create input layout. (%d) %s\n", hr, getErrorMessage( hr ).c_str() );
		return nullptr;
	}

	sPipeline* pipeline = new sPipeline();
	m_inputLayoutMap[ ++m_inputLayouts ] = inputLayout;
	pipeline->handle = m_inputLayouts;
	return pipeline;
#else
	return nullptr;
#endif
}

void wv::cDirectX11GraphicsDevice::destroyPipeline( sPipeline* _pPipeline )
{
	WV_TRACE();

#ifdef WV_SUPPORT_OPENGL_TEMP
	glDeleteProgramPipelines( 1, &_pPipeline->handle );
	WV_ASSERT_ERR( "ERROR\n" );

	_pPipeline->handle = 0;
#endif
}

void wv::cDirectX11GraphicsDevice::bindPipeline( sPipeline* _pPipeline )
{
	WV_TRACE();

#ifdef WV_SUPPORT_OPENGL_TEMP
	if( m_activePipeline == _pPipeline )
		return;

	glBindProgramPipeline( _pPipeline ? _pPipeline->handle : 0 );
	WV_ASSERT_ERR( "ERROR\n" );

	m_activePipeline = _pPipeline;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

wv::cGPUBuffer* wv::cDirectX11GraphicsDevice::createGPUBuffer( sGPUBufferDesc* _desc )
{
	WV_TRACE();

#ifdef WV_SUPPORT_D3D11

	D3D11_BUFFER_DESC vertexBufferDesc = {};
	vertexBufferDesc.ByteWidth = _desc->size;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	switch ( _desc->type )
	{
	case WV_BUFFER_TYPE_INDEX: vertexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER; break;
	case WV_BUFFER_TYPE_VERTEX: vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER; break;
	case WV_BUFFER_TYPE_UNIFORM: vertexBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; break;
	default:
		return nullptr;
	}

	// TODO immutable
	switch ( _desc->usage )
	{
	case WV_BUFFER_USAGE_STATIC_DRAW: vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC; break;
	case WV_BUFFER_USAGE_DYNAMIC_DRAW: vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC; break;
	default:
		return nullptr;
	}

	ID3D11Buffer* gpuBuffer;

	HRESULT hr;
	hr = m_device->CreateBuffer(
		&vertexBufferDesc,
		nullptr,
		&gpuBuffer);
	if ( FAILED( hr ) )
	{
		Debug::Print( Debug::WV_PRINT_ERROR, "Failed to create gpu buffer. (%d) %s\n", hr, getErrorMessage( hr ).c_str() );
		return nullptr;
	}

	m_dataBufferMap[ ++m_dataBuffers ] = gpuBuffer;

	cGPUBuffer& buffer = *new cGPUBuffer();
	buffer.type = _desc->type;
	buffer.usage = _desc->usage;
	buffer.name = _desc->name;
	buffer.handle = m_dataBuffers;

	if ( _desc->size > 0 )
		allocateBuffer( &buffer, _desc->size );
	
	return &buffer;
#else 
	return nullptr;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::bufferData( cGPUBuffer* _buffer )
{
	WV_TRACE();

#ifdef WV_SUPPORT_D3D11
	auto iter = m_dataBufferMap.find( _buffer->handle );
	if ( iter == m_dataBufferMap.end() )
	{
		return; // todo
	}


	cGPUBuffer& buffer = *_buffer;
	if ( buffer.pData == nullptr || buffer.size == 0 )
	{
		Debug::Print( Debug::WV_PRINT_ERROR, "Cannot submit buffer with 0 data or size\n" );
		return;
	}

	ID3D11Buffer* d3d11buffer = iter->second;

	D3D11_MAPPED_SUBRESOURCE resource;
	m_deviceContext->Map( d3d11buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource );
	// TODO: Validate size
	memcpy( resource.pData, _buffer->pData, buffer.size );
	m_deviceContext->Unmap( d3d11buffer, 0 );
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::allocateBuffer( cGPUBuffer* _buffer, size_t _size )
{
	WV_TRACE();

#ifdef WV_SUPPORT_D3D11
	auto iter = m_dataBufferMap.find( _buffer->handle );
	if ( iter == m_dataBufferMap.end() )
	{
		return; // todo
	}

	
	cGPUBuffer& buffer = *_buffer;
	if ( buffer.pData )
		delete[] buffer.pData;
	buffer.pData = new uint8_t[ _size ];
	buffer.size = _size;

	ID3D11Buffer* d3d11buffer = iter->second;

	D3D11_MAPPED_SUBRESOURCE resource;
	m_deviceContext->Map( d3d11buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource );
	// TODO: Validate size
	memcpy( resource.pData, _buffer->pData, buffer.size );
	m_deviceContext->Unmap( d3d11buffer, 0 );
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::destroyGPUBuffer( cGPUBuffer* _buffer )
{
	WV_TRACE();

#ifdef WV_SUPPORT_D3D11
	auto iter = m_dataBufferMap.find( _buffer->handle );
	if ( iter != m_dataBufferMap.end() )
	{
		iter->second->Release();
		m_dataBufferMap.erase(iter);
	}
	
	if ( _buffer->pData )
	{
		delete[] _buffer->pData;
		_buffer->pData = nullptr;
	}

	if ( _buffer->pPlatformData )
	{
		delete _buffer->pPlatformData;
		_buffer->pPlatformData = nullptr;
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

wv::cMesh* wv::cDirectX11GraphicsDevice::createMesh( sMeshDesc* _desc )
{
	WV_TRACE();
	Debug::Print( Debug::WV_PRINT_INFO, "createMesh\n" );

#ifdef WV_SUPPORT_D3D11
	cMesh& mesh = *new cMesh();
	/*
	glGenVertexArrays( 1, &mesh.handle );
	glBindVertexArray( mesh.handle );

	WV_ASSERT_ERR( "ERROR\n" );
	*/

	sGPUBufferDesc vbDesc;
	vbDesc.name  = "vbo";
	vbDesc.type  = WV_BUFFER_TYPE_VERTEX;
	vbDesc.usage = WV_BUFFER_USAGE_STATIC_DRAW;
	vbDesc.size  = _desc->sizeVertices;
	mesh.vertexBuffer = createGPUBuffer( &vbDesc );
	mesh.material = _desc->pMaterial;

	uint32_t count = _desc->sizeVertices / sizeof( Vertex );
	mesh.vertexBuffer->count = count;
	
	/*
	glBindBuffer( GL_ARRAY_BUFFER, mesh.vertexBuffer->handle );
	
	WV_ASSERT_ERR( "ERROR\n" );
	*/

	mesh.vertexBuffer->buffer( (uint8_t*)_desc->vertices, _desc->sizeVertices );
	bufferData( mesh.vertexBuffer );

	if ( _desc->numIndices > 0 )
	{
		mesh.drawType = WV_MESH_DRAW_TYPE_INDICES;

		sGPUBufferDesc ibDesc;
		ibDesc.name  = "ebo";
		ibDesc.type  = WV_BUFFER_TYPE_INDEX;
		ibDesc.usage = WV_BUFFER_USAGE_STATIC_DRAW;

		mesh.indexBuffer = createGPUBuffer( &ibDesc );
		mesh.indexBuffer->count = _desc->numIndices;
		
		// glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mesh.indexBuffer->handle );
		
		WV_ASSERT_ERR( "ERROR\n" );

		if ( _desc->indices16 )
		{
			const size_t bufferSize = _desc->numIndices * sizeof( uint16_t );

			allocateBuffer( mesh.indexBuffer, bufferSize );
			mesh.indexBuffer->buffer( _desc->indices16, bufferSize );
		}
		else if ( _desc->indices32 )
		{
			const size_t bufferSize = _desc->numIndices * sizeof( uint32_t );

			allocateBuffer( mesh.indexBuffer, bufferSize );
			mesh.indexBuffer->buffer( _desc->indices32, bufferSize );
		}

		bufferData( mesh.indexBuffer );
	}
	else
	{
		mesh.drawType = WV_MESH_DRAW_TYPE_VERTICES;
	}
	
	int offset = 0;
	int stride = 0;
	for ( unsigned int i = 0; i < _desc->layout.numElements; i++ )
	{
		sVertexAttribute& element = _desc->layout.elements[ i ];
		stride += element.size;
	}

	/*
	for ( unsigned int i = 0; i < _desc->layout.numElements; i++ )
	{
		sVertexAttribute& element = _desc->layout.elements[ i ];

		GLenum type = GL_FLOAT;
		switch ( _desc->layout.elements[ i ].type )
		{
		case WV_BYTE:           type = GL_BYTE;           break;
		case WV_UNSIGNED_BYTE:  type = GL_UNSIGNED_BYTE;  break;
		case WV_SHORT:          type = GL_SHORT;          break;
		case WV_UNSIGNED_SHORT: type = GL_UNSIGNED_SHORT; break;
		case WV_INT:            type = GL_INT;            break;
		case WV_UNSIGNED_INT:   type = GL_UNSIGNED_INT;   break;
		case WV_FLOAT:          type = GL_FLOAT;          break;
		#ifndef EMSCRIPTEN // WebGL does not support GL_DOUBLE
		case WV_DOUBLE:         type = GL_DOUBLE; break;
		#endif
		}

		glVertexAttribPointer( i, element.componentCount, type, element.normalized, stride, VPTRi32( offset ) );
		glEnableVertexAttribArray( i );

		WV_ASSERT_ERR( "ERROR\n" );

		offset += element.size;
	}

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindVertexArray( 0 );
	*/

	mesh.vertexBuffer->stride = stride;
	
	if( _desc->pParentTransform != nullptr )
		_desc->pParentTransform->addChild( &mesh.transform );

	return &mesh;
#else
	return nullptr;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::destroyMesh( cMesh* _pMesh )
{
	WV_TRACE();

#ifdef WV_SUPPORT_OPENGL_TEMP
	cMesh& pr = *_pMesh;
	destroyGPUBuffer( pr.indexBuffer );
	destroyGPUBuffer( pr.vertexBuffer );

	glDeleteVertexArrays( 1, &pr.handle );
	WV_ASSERT_ERR( "ERROR\n" );
	delete _pMesh;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::createTexture( Texture* _pTexture, TextureDesc* _desc )
{
	WV_TRACE();

#ifdef WV_SUPPORT_OPENGL_TEMP
	GLenum internalFormat = GL_R8;
	GLenum format = GL_RED;

	unsigned char* data = nullptr;
	if ( _pTexture->getData() )
	{
		data = _pTexture->getData();
		_desc->width = _pTexture->getWidth();
		_desc->height = _pTexture->getHeight();
		_desc->channels = static_cast<wv::TextureChannels>( _pTexture->getNumChannels() );
	}
	
	switch ( _desc->channels )
	{
	case wv::WV_TEXTURE_CHANNELS_R:
		format = GL_RED;
		switch ( _desc->format )
		{
		case wv::WV_TEXTURE_FORMAT_BYTE:  internalFormat = GL_R8;  break;
		case wv::WV_TEXTURE_FORMAT_FLOAT: internalFormat = GL_R32F; break;
		case wv::WV_TEXTURE_FORMAT_INT:   internalFormat = GL_R32I; format = GL_RED_INTEGER; break;
		}
		break;
	case wv::WV_TEXTURE_CHANNELS_RG:
		format = GL_RG;
		switch ( _desc->format )
		{
		case wv::WV_TEXTURE_FORMAT_BYTE:  internalFormat = GL_RG8;    break;
		case wv::WV_TEXTURE_FORMAT_FLOAT: internalFormat = GL_RG32F; break;
		case wv::WV_TEXTURE_FORMAT_INT:   internalFormat = GL_RG32I; format = GL_RG_INTEGER; break;
		}
		break;
	case wv::WV_TEXTURE_CHANNELS_RGB:
		format = GL_RGB;
		glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // 3 (channels) is not divisible by 4. Set pixel alignment to 1
		switch ( _desc->format )
		{
		case wv::WV_TEXTURE_FORMAT_BYTE:  internalFormat = GL_RGB8;   break;
		case wv::WV_TEXTURE_FORMAT_FLOAT: internalFormat = GL_RGB32F; break;
		case wv::WV_TEXTURE_FORMAT_INT:   internalFormat = GL_RGB32I; format = GL_RGB_INTEGER; break;
		}
		break;
	case wv::WV_TEXTURE_CHANNELS_RGBA:
		format = GL_RGBA;
		switch ( _desc->format )
		{
		case wv::WV_TEXTURE_FORMAT_BYTE:  internalFormat = GL_RGBA8;   break;
		case wv::WV_TEXTURE_FORMAT_FLOAT: internalFormat = GL_RGBA32F; break;
		case wv::WV_TEXTURE_FORMAT_INT:   internalFormat = GL_RGBA32I; format = GL_RGBA_INTEGER; break;
		}
		break;
	}

	GLuint handle;
	glGenTextures( 1, &handle );
	WV_ASSERT_ERR( "Failed to gen texture\n" );

	_pTexture->setHandle( handle );

	glBindTexture( GL_TEXTURE_2D, handle );

	WV_ASSERT_ERR( "Failed to bind texture\n" );
	
	GLenum filter = GL_NEAREST;
	switch ( _desc->filtering )
	{
	case WV_TEXTURE_FILTER_NEAREST: filter = GL_NEAREST; break;
	case WV_TEXTURE_FILTER_LINEAR:  filter = GL_LINEAR; break;
	}
	
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	
	GLenum type = GL_UNSIGNED_BYTE;
	switch ( _desc->format )
	{
	case wv::WV_TEXTURE_FORMAT_FLOAT: type = GL_FLOAT; break;
	case wv::WV_TEXTURE_FORMAT_INT:   type = GL_INT; break;
	}

	glTexImage2D( GL_TEXTURE_2D, 0, internalFormat, _desc->width, _desc->height, 0, format, type, data );
#ifdef WV_DEBUG
	assertGLError( "Failed to create Texture\n" );
#endif

	if ( _desc->generateMipMaps )
	{
		glGenerateMipmap( GL_TEXTURE_2D );

		WV_ASSERT_ERR( "ERROR\n" );
	}
	_pTexture->setWidth( _desc->width );
	_pTexture->setHeight( _desc->height );

	// Debug::Print( Debug::WV_PRINT_DEBUG, "Created texture %s\n", _pTexture->getName().c_str() );
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::destroyTexture( Texture** _texture )
{
	WV_TRACE();

#ifdef WV_SUPPORT_OPENGL_TEMP
	// Debug::Print( Debug::WV_PRINT_DEBUG, "Destroyed texture %s\n", (*_texture)->getName().c_str() );

	wv::Handle handle = ( *_texture )->getHandle();
	glDeleteTextures( 1, &handle );
	delete *_texture;
	*_texture = nullptr;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::bindTextureToSlot( Texture* _texture, unsigned int _slot )
{
	WV_TRACE();

#ifdef WV_SUPPORT_OPENGL_TEMP
	/// TODO: some cleaner way of checking version/supported features
	if ( m_graphicsApiVersion.major == 4 && m_graphicsApiVersion.minor >= 5 ) // if OpenGL 4.5 or higher
	{
		glBindTextureUnit( _slot, _texture->getHandle() );

		WV_ASSERT_ERR( "ERROR\n" );
	}
	else 
	{
		glActiveTexture( GL_TEXTURE0 + _slot );
		glBindTexture( GL_TEXTURE_2D, _texture->getHandle() );

		WV_ASSERT_ERR( "ERROR\n" );
	}

	m_boundTextureSlots[ _slot ] = _texture->getHandle();
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::swapBuffers()
{
	WV_TRACE();

#ifdef WV_SUPPORT_D3D11
	m_deviceContext->ClearRenderTargetView( m_renderTargetView, m_clearBackgroundColor );
	m_swapChain->Present( 1, 0 );
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::draw( cMesh* _pMesh )
{
	WV_TRACE();

#ifdef WV_SUPPORT_OPENGL_TEMP
	glBindVertexArray( _pMesh->handle );

	WV_ASSERT_ERR( "ERROR\n" );

	std::vector<cGPUBuffer*>& shaderBuffers = m_activePipeline->pVertexProgram->shaderBuffers;
	for ( auto& buf : shaderBuffers )
		bufferData( buf );
	
	/// TODO: change GL_TRIANGLES
	if ( _pMesh->drawType == WV_MESH_DRAW_TYPE_INDICES )
	{
		glDrawElements( GL_TRIANGLES, _pMesh->indexBuffer->count, GL_UNSIGNED_INT, 0 );

		WV_ASSERT_ERR( "ERROR\n" );
	}
	else
	{ 
	#ifndef EMSCRIPTEN
		/// this does not work on WebGL
		wv::Handle vbo = _pMesh->vertexBuffer->handle;
		size_t numVertices = _pMesh->vertexBuffer->count;
		glBindVertexBuffer( 0, vbo, 0, _pMesh->vertexBuffer->stride );
		WV_ASSERT_ERR( "ERROR\n" );

		glDrawArrays( GL_TRIANGLES, 0, numVertices );
		WV_ASSERT_ERR( "ERROR\n" );
	#else
		Debug::Print( Debug::WV_PRINT_FATAL, "glBindVertexBuffer is not supported on WebGL. Index Buffer is required\n" );
	#endif
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

bool wv::cDirectX11GraphicsDevice::getError( std::string* _out )
{
	WV_TRACE();

#ifdef WV_SUPPORT_OPENGL_TEMP
	bool hasError = false;

	GLenum err;
	while ( ( err = glGetError() ) != GL_NO_ERROR ) // is while needed here?
	{
		hasError = true;
		switch ( err )
		{
		case GL_INVALID_ENUM:      *_out = "GL_INVALID_ENUM";      break;
		case GL_INVALID_VALUE:     *_out = "GL_INVALID_VALUE";     break;
		case GL_INVALID_OPERATION: *_out = "GL_INVALID_OPERATION"; break;
		
		case GL_STACK_OVERFLOW:  *_out = "GL_STACK_OVERFLOW";    break;
		case GL_STACK_UNDERFLOW: *_out = "GL_STACK_UNDERFLOW";   break;
		case GL_OUT_OF_MEMORY:   *_out = "GL_OUT_OF_MEMORY";     break;
		case GL_CONTEXT_LOST:    *_out = "GL_OUT_OF_MEMORY";     break;

		case GL_INVALID_FRAMEBUFFER_OPERATION: *_out = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
		}
	}

	return hasError;
#else
	return false;
#endif
}
