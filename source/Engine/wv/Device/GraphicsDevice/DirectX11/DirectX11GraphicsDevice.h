#pragma once

#include <wv/Types.h>
#include <wv/Shader/Shader.h>
#include <wv/Shader/ShaderProgram.h>
#include <wv/Misc/Color.h>
#include <wv/Graphics/GPUBuffer.h>

#include <unordered_map>

#include <wv/Device/GraphicsDevice.h>

#ifdef WV_SUPPORT_D3D11>
#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "DXGI.lib" )
#include <d3d11.h>
#endif

///////////////////////////////////////////////////////////////////////////////////////

namespace wv
{

	struct sD3D11AdapterData
	{
#ifdef WV_SUPPORT_D3D11
		IDXGIAdapter* pAdapter = nullptr;
		DXGI_ADAPTER_DESC description;
#endif
	};

///////////////////////////////////////////////////////////////////////////////////////

	class cDirectX11GraphicsDevice : public iGraphicsDevice
	{
	public:
		cDirectX11GraphicsDevice();
		~cDirectX11GraphicsDevice() {}

		virtual void terminate() override;

		virtual void onResize( int _width, int _height ) override;
		virtual void setViewport( int _width, int _height ) override;

		virtual RenderTarget* createRenderTarget( RenderTargetDesc* _desc ) override;
		virtual void destroyRenderTarget( RenderTarget** _renderTarget ) override;

		virtual void setRenderTarget( RenderTarget* _target ) override;
		virtual void setClearColor( const wv::cColor& _color ) override;
		virtual void clearRenderTarget( bool _color, bool _depth ) override;

		virtual sShaderProgram* createProgram( sShaderProgramDesc* _desc ) override;
		virtual void destroyProgram( sShaderProgram* _pProgram ) override;

		virtual sPipeline* createPipeline( sPipelineDesc* _desc ) override;
		virtual void destroyPipeline( sPipeline* _pPipeline ) override;
		virtual void bindPipeline( sPipeline* _pPipeline ) override;

		virtual cGPUBuffer* createGPUBuffer ( sGPUBufferDesc* _desc ) override;
		virtual void        allocateBuffer  ( cGPUBuffer* _buffer, size_t _size ) override;
		virtual void        bufferData      ( cGPUBuffer* _buffer ) override;
		virtual void        destroyGPUBuffer( cGPUBuffer* _buffer ) override;
		
		virtual cMesh* createMesh ( sMeshDesc* _desc )      override;
		virtual void       destroyMesh( cMesh* _pMesh ) override;

		virtual void createTexture( Texture* _pTexture, TextureDesc* _desc ) override;
		virtual void destroyTexture( Texture** _texture ) override;

		virtual void bindTextureToSlot( Texture* _texture, unsigned int _slot ) override;

		virtual void draw( cMesh* _pMesh ) override;

///////////////////////////////////////////////////////////////////////////////////////

	protected:
		std::vector<sD3D11AdapterData> m_d3d11adapters;

#ifdef WV_SUPPORT_D3D11
		ID3D11Device* m_device;
		IDXGISwapChain* m_swapChain;
		ID3D11DeviceContext* m_deviceContext;
		ID3D11Texture2D* m_backBuffer;
		ID3D11RenderTargetView* m_renderTargetView;

		uint32_t m_vertexShaders = 0;
		std::unordered_map<wv::Handle, ID3D11VertexShader*> m_vertexShaderMap;
		uint32_t m_pixelShaders = 0;
		std::unordered_map<wv::Handle, ID3D11PixelShader*> m_pixelShaderMap;
		uint32_t m_inputLayouts = 0;
		std::unordered_map<wv::Handle, ID3D11InputLayout*> m_inputLayoutMap;
		
		
#endif

		virtual bool initialize( GraphicsDeviceDesc* _desc ) override;

		template<typename... Args>
		bool assertGLError( const std::string _msg, Args..._args );
		bool getError( std::string* _out );

		/// TODO: remove?
		sPipeline* m_activePipeline = nullptr;
		RenderTarget* m_activeRenderTarget = nullptr;

		int m_numTotalUniformBlocks = 0;

		// states
		std::vector<wv::Handle> m_boundTextureSlots;
	};

	template<typename ...Args>
	inline bool cDirectX11GraphicsDevice::assertGLError( const std::string _msg, Args... _args )
	{
		std::string error;
		if ( !getError( &error ) )
			return true;
		
		Debug::Print( Debug::WV_PRINT_ERROR, _msg.c_str(), _args... );
		Debug::Print( Debug::WV_PRINT_ERROR, "  %s\n", error.c_str() );

		return false;
	}
}