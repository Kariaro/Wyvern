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
}
#endif

std::string wv::cDirectX11GraphicsDevice::getAllErrors()
{
	std::string result;
#ifdef WV_SUPPORT_D3D11
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
#endif
	return result;
}

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

	D3D11_RASTERIZER_DESC rasterDesc =
	{
		.FillMode = D3D11_FILL_SOLID,
		.CullMode = D3D11_CULL_NONE,
		.DepthClipEnable = TRUE,
	};
	m_device->CreateRasterizerState( &rasterDesc, &m_rasterizerState );

	
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
		Debug::Print( Debug::WV_PRINT_FATAL, "Failed to create input layout. (%d) %s\n", hr, getErrorMessage( hr ).c_str() );
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
		Debug::Print( Debug::WV_PRINT_ERROR, "Failed to create gpu buffer. (%d) %s %s\n",
			hr, getErrorMessage( hr ).c_str(), getAllErrors().c_str() );
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

	MeshData* pMeshDataTest = new MeshData{};
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
		
		WV_ASSERT_ERR( "ERROR\n" );

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

	// glDeleteVertexArrays( 1, &pr.handle );
	// WV_ASSERT_ERR( "ERROR\n" );
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
