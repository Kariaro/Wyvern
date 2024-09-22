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

#include <iostream>

#include <wv/Device/DeviceContext.h>

#ifdef WV_SUPPORT_D3D11
#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "DXGI.lib" )
#pragma comment( lib, "d3dcompiler.lib" )
#pragma comment( lib, "dxguid.lib" )
#include <d3dcompiler.h>
#include <d3d11shader.h>
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


#ifdef WV_SUPPORT_D3D11
std::string getErrorMessage_DO_NOT_USE( HRESULT _hr )
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
}

std::string wv::cDirectX11GraphicsDevice::getAllErrors()
{
	std::string result;
	uint64_t message_count = m_infoQueue->GetNumStoredMessages();

	for ( uint64_t i = 0; i < message_count; i++ )
	{
		size_t message_size = 0;
		m_infoQueue->GetMessage( i, nullptr, &message_size ); //get the size of the message

		D3D11_MESSAGE* message = (D3D11_MESSAGE*)malloc( message_size );
		m_infoQueue->GetMessage( i, message, &message_size );
		
		//do whatever you want to do with it
		result += "Directx11: " + std::string( message->pDescription, message->DescriptionByteLength ) + "\n";

		free( message );
	}

	m_infoQueue->ClearStoredMessages();
	return result;
}
#endif

///////////////////////////////////////////////////////////////////////////////////////

wv::cDirectX11GraphicsDevice::cDirectX11GraphicsDevice()
{
	WV_TRACE();

#ifdef WV_SUPPORT_D3D11
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
#endif
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

	m_viewport.Width = width;
	m_viewport.Height = height;

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

	// Create rasterizer
	D3D11_RASTERIZER_DESC rasterDesc =
	{
		.FillMode = D3D11_FILL_SOLID,
		.CullMode = D3D11_CULL_NONE,
		.DepthClipEnable = TRUE,
	};
	m_device->CreateRasterizerState( &rasterDesc, &m_rasterizerState );

	// Create depth buffer
	D3D11_TEXTURE2D_DESC depthbufferdesc;
	m_backBuffer->GetDesc( &depthbufferdesc );
	depthbufferdesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthbufferdesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	hr = m_device->CreateTexture2D( &depthbufferdesc, nullptr, &m_depthBuffer );
	if ( FAILED( hr ) ) //If error occurred
	{
		Debug::Print( Debug::WV_PRINT_FATAL, "Failed to create depth buffer texture. [%s]\n", getAllErrors().c_str() );
		return false;
	}

	hr = m_device->CreateDepthStencilView( m_depthBuffer, nullptr, &m_depthBufferStencil );
	if ( FAILED( hr ) ) //If error occurred
	{
		Debug::Print( Debug::WV_PRINT_FATAL, "Failed to create depth buffer stencil view. [%s]\n", getAllErrors().c_str() );
		return false;
	}

	m_deviceContext->OMSetRenderTargets( 1, &m_renderTargetView, m_depthBufferStencil );
	
	ID3D11Debug* d3dDebug = nullptr;
	m_device->QueryInterface( __uuidof( ID3D11Debug ), (void**)&d3dDebug );
	if ( d3dDebug )
	{
		m_infoQueue = nullptr;
		if ( SUCCEEDED( d3dDebug->QueryInterface( __uuidof( ID3D11InfoQueue ), (void**)&m_infoQueue ) ) )
		{
			// m_infoQueue->SetBreakOnSeverity( D3D11_MESSAGE_SEVERITY_CORRUPTION, true );
			// m_infoQueue->SetBreakOnSeverity( D3D11_MESSAGE_SEVERITY_ERROR, true );
			// m_infoQueue->Release();
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

#ifdef WV_SUPPORT_D3D11
	m_viewport = {
		0.0f,
		0.0f,
		(float)_width,
		(float)_height,
		0.0f,
		1.0f };
	m_deviceContext->RSSetViewports( 1, &m_viewport );
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::setViewport( int _width, int _height )
{
	WV_TRACE();

#ifdef WV_SUPPORT_D3D11
	m_viewport = {
		0.0f,
		0.0f,
		(float) _width,
		(float)_height,
		0.0f,
		1.0f };
	m_deviceContext->RSSetViewports( 1, &m_viewport );
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

wv::RenderTarget* wv::cDirectX11GraphicsDevice::createRenderTarget( RenderTargetDesc* _desc )
{
	WV_TRACE();

#ifdef WV_SUPPORT_D3D11
	RenderTargetDesc& desc = *_desc;
	RenderTarget* target = new RenderTarget();
	
	target->textures = new Texture*[ desc.numTextures ];

	sD3D11RenderTargetData* data = new sD3D11RenderTargetData();
	target->fbHandle = m_renderTargetData.add ( data );

	data->renderTargets.resize( desc.numTextures );

	HRESULT hr;
	for ( int i = 0; i < desc.numTextures; i++ )
	{
		desc.pTextureDescs[ i ].width = desc.width;
		desc.pTextureDescs[ i ].height = desc.height;
		desc.pTextureDescs[ i ].channels = wv::WV_TEXTURE_CHANNELS_RGBA; // TODO ?

		std::string texname = "buffer_tex" + std::to_string( i );

		target->textures[ i ] = new Texture( texname );
		internal_createTexture( target->textures[ i ], &desc.pTextureDescs[ i ], true );

		ID3D11Texture2D* renderTexture = m_texture2dMap.get( target->textures[ i ]->getHandle() );
		ID3D11RenderTargetView* renderView;
		hr = m_device->CreateRenderTargetView( renderTexture, NULL, &renderView );
		if ( FAILED( hr ) )
		{
			Debug::Print( Debug::WV_PRINT_FATAL, "Failed to create render target. [%s]\n", getAllErrors().c_str() );
			continue;
		}
		data->renderTargets[ i ] = renderView;
	}

	target->width  = desc.width;
	target->height = desc.height;

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

#ifdef WV_SUPPORT_D3D11
	if ( m_activeRenderTarget == _target )
		return;

	unsigned int handle = _target ? _target->fbHandle : 0;

	std::cout << "UPDATE RENDER TARGET " << handle << std::endl;
	if ( _target )
	{
		setViewport( _target->width, _target->height );
		
		auto* data = m_renderTargetData.get( handle );
		if ( data != nullptr )
		{
			m_deviceContext->OMGetRenderTargets( _target->numTextures, data->renderTargets.data(), &m_depthBufferStencil );
		}
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

#ifdef WV_SUPPORT_D3D11
void wv::cDirectX11GraphicsDevice::reflectShader( wv::sShaderProgram* _program, ID3DBlob* _shader )
{
	ID3D11ShaderReflection* pReflector = nullptr;
	D3DReflect( _shader->GetBufferPointer(), _shader->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&pReflector );

	D3D11_SHADER_DESC shaderDesc;
	HRESULT hr;
	hr = pReflector->GetDesc( &shaderDesc );
	if ( FAILED( hr ) )
	{
		wv::Debug::Print( wv::Debug::WV_PRINT_ERROR, "Failed to reflect shader. (%d) %s\n", hr, getAllErrors().c_str() );
		return;
	}

	for ( uint32_t index = 0; index < shaderDesc.ConstantBuffers; index++ )
	{
		ID3D11ShaderReflectionConstantBuffer* buffer = pReflector->GetConstantBufferByIndex( index );
		D3D11_SHADER_BUFFER_DESC bufferDesc;
		hr = buffer->GetDesc( &bufferDesc );
		if ( FAILED( hr ) )
		{
			wv::Debug::Print( wv::Debug::WV_PRINT_ERROR, "Failed to reflect shader buffer index=%d. (%d) %s\n", index, hr, getAllErrors().c_str() );
			return;
		}

		D3D11_SHADER_INPUT_BIND_DESC shaderInputDesc;
		hr = pReflector->GetResourceBindingDesc( index, &shaderInputDesc );
		if ( FAILED( hr ) )
		{
			wv::Debug::Print( wv::Debug::WV_PRINT_ERROR, "Failed to reflect shader input resource desc index=%d. (%d) %s\n", index, hr, getAllErrors().c_str() );
			return;
		}

		sGPUBufferDesc ubDesc;
		ubDesc.name = std::string( bufferDesc.Name );
		ubDesc.size = bufferDesc.Size;
		ubDesc.type = WV_BUFFER_TYPE_UNIFORM;
		ubDesc.usage = WV_BUFFER_USAGE_DYNAMIC_DRAW;

		cGPUBuffer* gpuBuffer = createGPUBuffer( &ubDesc );
		sD3D11UniformBufferData* pUBData = new sD3D11UniformBufferData();
		gpuBuffer->pPlatformData = pUBData;
		pUBData->bindingIndex = shaderInputDesc.BindPoint;

		Debug::Print( "- Creating %s\n", bufferDesc.Name );

		_program->shaderBuffers.push_back( gpuBuffer );
	}

	// pDesc->
	pReflector->Release();
}
#endif

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
		
		uint32_t handle = m_vertexShaderMap.add( vsShader );
		m_vertexShaderBlobMap.set( handle, shaderBlob );

		sShaderProgram* program = new sShaderProgram();
		program->handle = handle;
		Debug::Print( "Reflecting Vertex\n" );
		reflectShader( program, shaderBlob );

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

		sShaderProgram* program = new sShaderProgram();
		program->handle = m_pixelShaderMap.add( piShader );
		Debug::Print( "Reflecting Fragment\n" );
		reflectShader( program, shaderBlob );

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
		m_pixelShaderMap.remove( _pProgram->handle );
		m_vertexShaderBlobMap.remove( _pProgram->handle );
	}
	else if ( _pProgram->type == eShaderProgramType::WV_SHADER_TYPE_FRAGMENT )
	{
		m_pixelShaderMap.remove( _pProgram->handle );
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
	auto* vertexShaderBlob = m_vertexShaderBlobMap.get( vertexHandle );
	if ( vertexShaderBlob == nullptr )
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
		vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize(),
		&inputLayout );
	if ( FAILED( hr ) )
	{
		Debug::Print( Debug::WV_PRINT_FATAL, "Failed to create input layout. [%s]\n", getAllErrors().c_str() );
		return nullptr;
	}

	sPipeline* pipeline = new sPipeline();
	pipeline->handle = m_inputLayoutMap.add( inputLayout );
	pipeline->pFragmentProgram = *_desc->pFragmentProgram;
	pipeline->pVertexProgram = *_desc->pVertexProgram;
	return pipeline;
#else
	return nullptr;
#endif
}

void wv::cDirectX11GraphicsDevice::destroyPipeline( sPipeline* _pPipeline )
{
	WV_TRACE();
#ifdef WV_SUPPORT_D3D11
	wv::Handle layoutHandle = _pPipeline->handle;
	wv::Handle vertexHandle = _pPipeline->pVertexProgram == nullptr ? 0 : _pPipeline->pVertexProgram->handle;
	wv::Handle pixelHandle = _pPipeline->pFragmentProgram == nullptr ? 0 : _pPipeline->pFragmentProgram->handle;

	m_inputLayoutMap.remove( layoutHandle );
	m_vertexShaderMap.remove( vertexHandle );
	m_pixelShaderMap.remove( pixelHandle );
	_pPipeline->handle = 0;
#endif
}

void wv::cDirectX11GraphicsDevice::bindPipeline( sPipeline* _pPipeline )
{
	WV_TRACE();

#ifdef WV_SUPPORT_D3D11
	wv::Handle layoutHandle = _pPipeline->handle;
	wv::Handle vertexHandle = _pPipeline->pVertexProgram == nullptr ? 0 : _pPipeline->pVertexProgram->handle;
	wv::Handle pixelHandle = _pPipeline->pFragmentProgram == nullptr ? 0 : _pPipeline->pFragmentProgram->handle;

	auto* inputLayout = m_inputLayoutMap.get( layoutHandle );
	auto* vertexShader = m_vertexShaderMap.get( vertexHandle );
	auto* pixelShader = m_pixelShaderMap.get( pixelHandle );

	m_deviceContext->IASetInputLayout( inputLayout );
	m_deviceContext->VSSetShader( vertexShader, nullptr, 0 );
	m_deviceContext->PSSetShader( pixelShader, nullptr, 0 );

	// Constant buffers
	if ( _pPipeline->pVertexProgram )
	{
		for ( auto& buf : _pPipeline->pVertexProgram->shaderBuffers )
		{
			if ( buf->pPlatformData != nullptr )
			{
				auto* platformData = (sD3D11UniformBufferData*)buf->pPlatformData;
				auto* buffer = m_dataBufferMap.get( buf->handle );

				m_deviceContext->VSSetConstantBuffers( platformData->bindingIndex, 1, &buffer );
			}
		}
	}

	m_activePipeline = _pPipeline;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

wv::cGPUBuffer* wv::cDirectX11GraphicsDevice::createGPUBuffer( sGPUBufferDesc* _desc )
{
	WV_TRACE();

#ifdef WV_SUPPORT_D3D11

	if ( _desc->size == 0 )
	{
		Debug::Print( Debug::WV_PRINT_ERROR, "Cannot create a GPU buffer with size zero.\n" );
		return nullptr;
	}

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
		Debug::Print( Debug::WV_PRINT_ERROR, "Failed to create gpu buffer. [%s]\n", getAllErrors().c_str() );
		return nullptr;
	}

	uint32_t handle = m_dataBufferMap.add( gpuBuffer );

	cGPUBuffer& buffer = *new cGPUBuffer();
	buffer.type = _desc->type;
	buffer.usage = _desc->usage;
	buffer.name = _desc->name;
	buffer.handle = handle;

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
	ID3D11Buffer* d3d11buffer = m_dataBufferMap.get( _buffer->handle );
	if ( d3d11buffer == nullptr )
	{
		return; // todo
	}

	cGPUBuffer& buffer = *_buffer;
	if ( buffer.pData == nullptr || buffer.size == 0 )
	{
		Debug::Print( Debug::WV_PRINT_ERROR, "Cannot submit buffer with 0 data or size\n" );
		return;
	}

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
	ID3D11Buffer* d3d11buffer = m_dataBufferMap.get( _buffer->handle );
	if ( d3d11buffer == nullptr )
	{
		return; // todo
	}

	
	cGPUBuffer& buffer = *_buffer;
	if ( buffer.pData )
		delete[] buffer.pData;
	buffer.pData = new uint8_t[ _size ];
	buffer.size = _size;

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
	m_dataBufferMap.remove( _buffer->handle );
	
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

wv::sMesh* wv::cDirectX11GraphicsDevice::createMesh( sMeshDesc* _desc )
{
	WV_TRACE();
	Debug::Print( Debug::WV_PRINT_INFO, "createMesh\n" );

#ifdef WV_SUPPORT_D3D11
	sMesh& mesh = *new sMesh();
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
	mesh.pVertexBuffer = createGPUBuffer( &vbDesc );
	mesh.pMaterial = _desc->pMaterial;

	uint32_t count = _desc->sizeVertices / sizeof( Vertex );
	mesh.pVertexBuffer->count = count;

	sMeshData* pMeshDataTest = new sMeshData{};
	mesh.handle = m_meshData.add( pMeshDataTest );
	
	/*
	glBindBuffer( GL_ARRAY_BUFFER, mesh.pVertexBuffer->handle );
	
	WV_ASSERT_ERR( "ERROR\n" );
	*/

	mesh.pVertexBuffer->buffer( (uint8_t*)_desc->vertices, _desc->sizeVertices );
	bufferData( mesh.pVertexBuffer );

	if ( _desc->numIndices > 0 )
	{
		mesh.drawType = WV_MESH_DRAW_TYPE_INDICES;

		sGPUBufferDesc ibDesc;
		ibDesc.name  = "ebo";
		ibDesc.type  = WV_BUFFER_TYPE_INDEX;
		ibDesc.usage = WV_BUFFER_USAGE_STATIC_DRAW;

		if ( _desc->pIndices16 )
		{
			ibDesc.size = _desc->numIndices * sizeof( uint16_t );
			pMeshDataTest->index_format = DXGI_FORMAT_R16_UINT;
		}
		else if ( _desc->pIndices32 )
		{
			ibDesc.size = _desc->numIndices * sizeof( uint32_t );
			pMeshDataTest->index_format = DXGI_FORMAT_R32_UINT;
		}

		mesh.pIndexBuffer = createGPUBuffer( &ibDesc );
		mesh.pIndexBuffer->count = _desc->numIndices;
		
		// glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mesh.pIndexBuffer->handle );
		// WV_ASSERT_ERR( "ERROR\n" );

		if ( _desc->pIndices16 )
		{
			const size_t bufferSize = _desc->numIndices * sizeof( uint16_t );

			allocateBuffer( mesh.pIndexBuffer, bufferSize );
			mesh.pIndexBuffer->buffer( _desc->pIndices16, bufferSize );
		}
		else if ( _desc->pIndices32 )
		{
			const size_t bufferSize = _desc->numIndices * sizeof( uint32_t );

			allocateBuffer( mesh.pIndexBuffer, bufferSize );
			mesh.pIndexBuffer->buffer( _desc->pIndices32, bufferSize );
		}

		bufferData( mesh.pIndexBuffer );
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

	mesh.pVertexBuffer->stride = stride;
	
	if( _desc->pParentTransform != nullptr )
		_desc->pParentTransform->addChild( &mesh.transform );

	return &mesh;
#else
	return nullptr;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::destroyMesh( sMesh* _pMesh )
{
	WV_TRACE();

#ifdef WV_SUPPORT_D3D11
	sMesh& pr = *_pMesh;
	destroyGPUBuffer( pr.pIndexBuffer );
	destroyGPUBuffer( pr.pVertexBuffer );

	// Delete mesh specific data
	m_meshData.remove( _pMesh->handle );

	delete _pMesh;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::createTexture( Texture* _pTexture, TextureDesc* _desc )
{
#ifdef WV_SUPPORT_D3D11
	internal_createTexture( _pTexture, _desc, false );
#endif
}

void wv::cDirectX11GraphicsDevice::internal_createTexture( Texture* _pTexture, TextureDesc* _desc, bool _renderTarget )
{
	WV_TRACE();

#ifdef WV_SUPPORT_D3D11
	unsigned char* data = nullptr;
	if ( _pTexture->getData() )
	{
		data = _pTexture->getData();
		_desc->width = _pTexture->getWidth();
		_desc->height = _pTexture->getHeight();
		_desc->channels = static_cast<wv::TextureChannels>( _pTexture->getNumChannels() );
	}

	DXGI_FORMAT internalFormat;

	int size = 1;

	switch ( _desc->channels )
	{
	case wv::WV_TEXTURE_CHANNELS_R:
		size = 1;
		switch ( _desc->format )
		{
		case wv::WV_TEXTURE_FORMAT_BYTE:
			internalFormat = DXGI_FORMAT_R8_UINT;
			size = 1;
			break;
		case wv::WV_TEXTURE_FORMAT_FLOAT:
			internalFormat = DXGI_FORMAT_R32_FLOAT;
			size = 4 * 1;
			break;
		case wv::WV_TEXTURE_FORMAT_INT:
			internalFormat = DXGI_FORMAT_R32_SINT;
			size = 4 * 1;
			break;
		}
		break;
	case wv::WV_TEXTURE_CHANNELS_RG:
		size = 2;
		switch ( _desc->format )
		{
		case wv::WV_TEXTURE_FORMAT_BYTE:
			internalFormat = DXGI_FORMAT_R8G8_UINT;
			size = 2;
			break;
		case wv::WV_TEXTURE_FORMAT_FLOAT:
			internalFormat = DXGI_FORMAT_R32G32_FLOAT;
			size = 4 * 2;
			break;
		case wv::WV_TEXTURE_FORMAT_INT:
			internalFormat = DXGI_FORMAT_R32G32_SINT;
			size = 4 * 2;
			break;
		}
		break;
	case wv::WV_TEXTURE_CHANNELS_RGB:
		Debug::Print( Debug::WV_PRINT_FATAL, "RGB not supported use RGBA\n" );
		return;
	case wv::WV_TEXTURE_CHANNELS_RGBA:
		switch ( _desc->format )
		{
		case wv::WV_TEXTURE_FORMAT_BYTE:
			internalFormat = DXGI_FORMAT_R8G8B8A8_UINT;
			size = 4;
			break;
		case wv::WV_TEXTURE_FORMAT_FLOAT:
			internalFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
			size = 4 * 4;
			break;
		case wv::WV_TEXTURE_FORMAT_INT:
			internalFormat = DXGI_FORMAT_R32G32B32A32_SINT;
			size = 4 * 4;
			break;
		break;
		}
		break;
	}

	// internalFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	// size = 4 * 4;

	
	D3D11_TEXTURE2D_DESC texturedesc = {};
	texturedesc.Width = _desc->width;
	texturedesc.Height = _desc->height;
	texturedesc.MipLevels = _desc->generateMipMaps ? 4 : 1;
	texturedesc.Format = internalFormat;
	texturedesc.ArraySize = 1;
	texturedesc.CPUAccessFlags = _renderTarget ? 0 : D3D11_CPU_ACCESS_WRITE;
	texturedesc.SampleDesc.Count = 1;
	texturedesc.Usage = _renderTarget ? D3D11_USAGE_DEFAULT : D3D11_USAGE_IMMUTABLE;
	texturedesc.BindFlags = _renderTarget
		? D3D11_BIND_RENDER_TARGET
		: D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA textureSRD = {};
	textureSRD.pSysMem = _pTexture->getData();
	textureSRD.SysMemPitch = _pTexture->getWidth() * size;

	D3D11_SUBRESOURCE_DATA* pTextureSRD = &textureSRD;

	if ( _pTexture->getData() == nullptr )
	{
		pTextureSRD = nullptr;
	}

	HRESULT hr;
	ID3D11Texture2D* texture2d = nullptr;
	hr = m_device->CreateTexture2D(
		&texturedesc,
		pTextureSRD,
		&texture2d);
	if ( FAILED( hr ) )
	{
		Debug::Print( Debug::WV_PRINT_FATAL, "Failed to create texture 2d. [%s]\n", getAllErrors().c_str() );
		return;
	}

	uint32_t handle = m_texture2dMap.add( texture2d );

	if ( !_renderTarget )
	{
		ID3D11ShaderResourceView* textureSRV = nullptr;
		hr = m_device->CreateShaderResourceView( texture2d, nullptr, &textureSRV );
		if ( FAILED( hr ) )
		{
			Debug::Print( Debug::WV_PRINT_FATAL, "Failed to create texture shader view. [%s]\n", getAllErrors().c_str() );
			return;
		}

		m_textureShaderResourceViewMap.set( handle, textureSRV );
	}

	// TODO: D3D11 Texture Filtering MIN MAG
	D3D11_SAMPLER_DESC samplerdesc = {};
	samplerdesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerdesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerdesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerdesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerdesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

	ID3D11SamplerState* textureSampler;
	hr = m_device->CreateSamplerState( &samplerdesc, &textureSampler );
	if ( FAILED( hr ) )
	{
		Debug::Print( Debug::WV_PRINT_FATAL, "Failed to create texture sampler. [%s]\n", getAllErrors().c_str() );
		return;
	}
	m_textureSamplerResourceMap.set( handle, textureSampler );

	_pTexture->setHandle( handle );

#endif
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::destroyTexture( Texture** _texture )
{
	WV_TRACE();

#ifdef WV_SUPPORT_D3D11
	// Debug::Print( Debug::WV_PRINT_DEBUG, "Destroyed texture %s\n", (*_texture)->getName().c_str() );

	wv::Handle handle = ( *_texture )->getHandle();
	m_texture2dMap.remove( handle );
	m_textureShaderResourceViewMap.remove( handle );
	m_textureSamplerResourceMap.remove( handle );
	delete *_texture;
	*_texture = nullptr;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::bindTextureToSlot( Texture* _texture, unsigned int _slot )
{
	WV_TRACE();

#ifdef WV_SUPPORT_D3D11
	wv::Handle handle = _texture->getHandle();
	auto* shaderResource = m_textureShaderResourceViewMap.get( handle );
	auto* shaderSampler = m_textureSamplerResourceMap.get( handle );

	m_deviceContext->PSSetShaderResources( _slot, 1, &shaderResource );
	m_deviceContext->PSSetSamplers( _slot, 1, &shaderSampler );

	m_boundTextureSlots[ _slot ] = _texture->getHandle();
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::swapBuffers()
{
	WV_TRACE();

#ifdef WV_SUPPORT_D3D11
	m_deviceContext->RSSetViewports( 1, &m_viewport );
	m_deviceContext->RSSetState( m_rasterizerState );

	// m_deviceContext->OMSetRenderTargets( 1, &m_renderTargetView, m_depthBufferStencil );
	m_deviceContext->OMSetBlendState( nullptr, nullptr, 0xffffffff );
	m_swapChain->Present( 1, 0 );

	// Clear for next frame
	if ( m_activeRenderTarget )
	{
		auto* data = m_renderTargetData.get( m_activeRenderTarget->fbHandle );
		if ( data != nullptr )
		{
			for ( auto* item : data->renderTargets )
			{
				m_deviceContext->ClearRenderTargetView( item, m_clearBackgroundColor );
			}
		}
	}

	m_deviceContext->ClearDepthStencilView( m_depthBufferStencil, D3D11_CLEAR_DEPTH, 1.0f, 0 );
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::draw( sMesh* _pMesh )
{
	WV_TRACE();

#ifdef WV_SUPPORT_D3D11
	for ( auto& buf : m_activePipeline->pVertexProgram->shaderBuffers )
		bufferData( buf );

	auto* vertexBuffer = m_dataBufferMap.get( _pMesh->pVertexBuffer->handle );
	auto* indexBuffer = m_dataBufferMap.get( _pMesh->pIndexBuffer->handle );
	auto* meshData = m_meshData.get( _pMesh->handle );

	m_deviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST ); // TODO
	m_deviceContext->IASetIndexBuffer( indexBuffer, meshData->index_format, 0 );
	UINT stride = sizeof( struct wv::Vertex );
	UINT offset = 0;
	m_deviceContext->IASetVertexBuffers( 0, 1, &vertexBuffer, &stride, &offset );
	

	m_deviceContext->Draw( _pMesh->pVertexBuffer->count, 0 );
#endif
}
