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

///////////////////////////////////////////////////////////////////////////////////////

wv::cDirectX11GraphicsDevice::cDirectX11GraphicsDevice()
{
	WV_TRACE();
	m_graphicsApi = WV_GRAPHICS_API_D3D11;

	IDXGIFactory* pFactory;

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


	// TODO : delete pFactory ?????
}

///////////////////////////////////////////////////////////////////////////////////////

bool wv::cDirectX11GraphicsDevice::initialize( GraphicsDeviceDesc* _desc )
{
	WV_TRACE();

#ifdef WV_SUPPORT_D3D11
	m_graphicsApi = WV_GRAPHICS_API_D3D11;
	
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

	// D3D11CreateDeviceAndSwapChain
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

	HWND window = 0;
	switch ( _desc->pContext->getContextAPI() )
	{
#ifdef WV_SUPPORT_GLFW
	case WV_DEVICE_CONTEXT_API_GLFW:
	{
		auto* pointer = dynamic_cast<GLFWDeviceContext*>( _desc->pContext );
		if ( pointer == nullptr )
			return false;

		window = glfwGetWin32Window( pointer->m_windowContext );
		break;
	}
#endif
#ifdef WV_SUPPORT_SDL2
	case WV_DEVICE_CONTEXT_API_SDL:
	{
		auto* pointer = dynamic_cast<SDLDeviceContext*>( _desc->pContext );
		if ( pointer == nullptr )
			return false;

		SDL_SysWMinfo wmInfo;
		SDL_VERSION( &wmInfo.version );
		SDL_GetWindowWMInfo( pointer->m_windowContext, &wmInfo );
		window = wmInfo.info.win.window;
		break;
	}
#endif
	default:
		return false;
	}

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
										NULL, //FLAGS FOR RUNTIME LAYERS
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

#ifdef WV_SUPPORT_OPENGL_TEMP
	glViewport( 0, 0, _width, _height );
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

#ifdef WV_SUPPORT_OPENGL_TEMP
	glClearColor( _color.r, _color.g, _color.b, _color.a );
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
		ID3D11VertexShader* vsShader;
		hr = m_device->CreateVertexShader(
			_desc->source.data,
			_desc->source.data->size,
			nullptr,
			&vsShader);
		if ( FAILED( hr ) )
		{
			Debug::Print( Debug::WV_PRINT_FATAL, "Failed to create vertex shader. (%d)\n", hr );
			return nullptr;
		}

		m_vertexShaderMap[ ++m_vertexShaders ] = vsShader;
		sShaderProgram* program = new sShaderProgram();
		program->handle = m_vertexShaders;
		return program;
	}
	else if ( type == eShaderProgramType::WV_SHADER_TYPE_FRAGMENT )
	{
		ID3D11PixelShader* piShader;
		hr = m_device->CreatePixelShader(
			_desc->source.data,
			_desc->source.data->size,
			nullptr,
			&piShader );
		if ( FAILED( hr ) )
		{
			Debug::Print( Debug::WV_PRINT_FATAL, "Failed to create pixel (fragment) shader. (%d)\n", hr );
			return nullptr;
		}

		m_pixelShaderMap[ ++m_pixelShaders ] = piShader;
		sShaderProgram* program = new sShaderProgram();
		program->handle = m_pixelShaders;
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
	D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
		{ "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		/*
		{ "COL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		 */
	};
	HRESULT hr;

	sShaderProgramSource& source = ( *_desc->pVertexProgram )->source;
	hr = m_device->CreateInputLayout(
		inputElementDesc,
		ARRAYSIZE( inputElementDesc ),
		source.data->data,
		source.data->size,
		&inputLayout );
	if ( FAILED( hr ) )
	{
		Debug::Print( Debug::WV_PRINT_FATAL, "Failed to create input layout. (%d)", hr );
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

#ifdef WV_SUPPORT_OPENGL_TEMP
	cGPUBuffer& buffer = *new cGPUBuffer();
	buffer.type  = _desc->type;
	buffer.usage = _desc->usage;
	buffer.name  = _desc->name;

	GLenum target = getGlBufferEnum( buffer.type );
	
	//glGenBuffers( 1, &buffer.handle );

	glCreateBuffers( 1, &buffer.handle );
	glBindBuffer( target, buffer.handle );

	assertGLError( "Failed to create buffer\n" );

	if ( _desc->size > 0 )
		allocateBuffer( &buffer, _desc->size );
	
	glBindBuffer( target, 0 );
	
	return &buffer;
#else 
	return nullptr;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::bufferData( cGPUBuffer* _buffer )
{
	WV_TRACE();

#ifdef WV_SUPPORT_OPENGL_TEMP
	cGPUBuffer& buffer = *_buffer;

	if ( buffer.pData == nullptr || buffer.size == 0 )
	{
		Debug::Print( Debug::WV_PRINT_ERROR, "Cannot submit buffer with 0 data or size\n" );
		return;
	}

	glNamedBufferSubData( buffer.handle, 0, buffer.size, buffer.pData );
	
	WV_ASSERT_ERR( "Failed to buffer data\n" );
	
#endif
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::allocateBuffer( cGPUBuffer* _buffer, size_t _size )
{
#ifdef WV_SUPPORT_D3D11
	D3D11_BUFFER_DESC desc;
	desc.ByteWidth = _size;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 1;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
	
	ID3D11Buffer* buffer = nullptr;

	HRESULT hr;
	hr = m_device->CreateBuffer( &desc, nullptr, &buffer );
	if ( FAILED( hr ) )
	{
		Debug::Print( Debug::WV_PRINT_FATAL, "Failed to allocate buffer. (%d)", hr );
		return;
	}
#endif

	/*
	cGPUBuffer& buffer = *_buffer;

	if ( buffer.pData )
		delete[] buffer.pData;
	
	GLenum usage = getGlBufferUsage( buffer.usage );
	buffer.pData = new uint8_t[ _size ];
	buffer.size  = _size;

	glNamedBufferData( buffer.handle, _size, 0, usage );

	WV_ASSERT_ERR( "Failed to allocate buffer\n" );
*/
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::cDirectX11GraphicsDevice::destroyGPUBuffer( cGPUBuffer* _buffer )
{
	WV_TRACE();

#ifdef WV_SUPPORT_OPENGL_TEMP
	glDeleteBuffers( 1, &_buffer->handle );
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

#ifdef WV_SUPPORT_OPENGL_TEMP
	cMesh& mesh = *new cMesh();
	glGenVertexArrays( 1, &mesh.handle );
	glBindVertexArray( mesh.handle );

	WV_ASSERT_ERR( "ERROR\n" );

	sGPUBufferDesc vbDesc;
	vbDesc.name  = "vbo";
	vbDesc.type  = WV_BUFFER_TYPE_VERTEX;
	vbDesc.usage = WV_BUFFER_USAGE_STATIC_DRAW;
	vbDesc.size  = _desc->sizeVertices;
	mesh.vertexBuffer = createGPUBuffer( &vbDesc );
	mesh.material = _desc->pMaterial;

	uint32_t count = _desc->sizeVertices / sizeof( Vertex );
	mesh.vertexBuffer->count = count;
	
	glBindBuffer( GL_ARRAY_BUFFER, mesh.vertexBuffer->handle );
	
	WV_ASSERT_ERR( "ERROR\n" );

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
		
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mesh.indexBuffer->handle );
		
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
	
	WV_ASSERT_ERR( "ERROR\n" );

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
