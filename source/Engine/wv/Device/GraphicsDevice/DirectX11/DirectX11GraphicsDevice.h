#pragma once

#include <wv/Types.h>
#include <wv/Shader/Shader.h>
#include <wv/Shader/ShaderProgram.h>
#include <wv/Misc/Color.h>
#include <wv/Graphics/GPUBuffer.h>

#include <unordered_map>

#include <wv/Device/GraphicsDevice.h>

#ifdef WV_SUPPORT_D3D11>
#include <d3d11.h>
#endif

///////////////////////////////////////////////////////////////////////////////////////

namespace wv
{

#ifdef WV_SUPPORT_D3D11
	struct sD3D11AdapterData
	{
		IDXGIAdapter* pAdapter = nullptr;
		DXGI_ADAPTER_DESC description;
	};

	struct sD3D11UniformBufferData
	{
		wv::Handle bindingIndex = 0;
	};

	struct sD3D11RenderTargetData
	{
		std::vector<ID3D11RenderTargetView*> renderTargets;
		void Release() {
			for ( auto* item : renderTargets )
			{
				item->Release();
			}
			delete this;
		}
	};

	struct sMeshData
	{
		DXGI_FORMAT index_format;
		void Release() { delete this; }
	};

	template<typename _Type>
	struct sD3D11ResourceMap
	{
		using _MapType = std::unordered_map<wv::Handle, _Type*>;
		using _MapIter = typename _MapType::local_iterator;

		uint32_t count { 0 };
		std::unordered_map<wv::Handle, _Type*> map;

		uint32_t add( _Type* _value )
		{
			map[ ++count ] = _value;
			return count;
		}

		uint32_t set( uint32_t _index, _Type* _value )
		{
			remove( _index );
			map[ _index ] = _value;
			return count;
		}

		_Type* get( wv::Handle _handle )
		{
			_MapIter iter = map.find( _handle );
			if ( iter != map.end() )
			{
				return iter->second;
			}
			return nullptr;
		}

		void remove( wv::Handle _handle )
		{
			_MapIter iter = map.find( _handle );
			if ( iter != map.end() )
			{
				iter->second->Release();
				map.erase( iter );
			}
		}
	};
#endif

///////////////////////////////////////////////////////////////////////////////////////

	class cDirectX11GraphicsDevice final : public iGraphicsDevice
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
		
		virtual sMesh* createMesh ( sMeshDesc* _desc )      override;
		virtual void       destroyMesh( sMesh* _pMesh ) override;

		virtual void createTexture( Texture* _pTexture, TextureDesc* _desc ) override;
		virtual void destroyTexture( Texture** _texture ) override;

		virtual void bindTextureToSlot( Texture* _texture, unsigned int _slot ) override;
		virtual void bindVertexBuffer( cGPUBuffer* _pVertexBuffer ) override {};

		virtual void setFillMode( eFillMode _mode ) override {};

		virtual void draw( sMesh* _pMesh ) override;
		virtual void drawIndices( uint32_t _numIndices ) override {};

		virtual void swapBuffers() override;

///////////////////////////////////////////////////////////////////////////////////////

	protected:
#ifdef WV_SUPPORT_D3D11
		void internal_createTexture( Texture* _pTexture, TextureDesc* _desc, bool _renderTarget );

		std::vector<sD3D11AdapterData> m_d3d11adapters;

		ID3D11Device* m_device;
		IDXGISwapChain* m_swapChain;
		ID3D11DeviceContext* m_deviceContext;

		// Remove these two (part of render target)
		ID3D11Texture2D* m_backBuffer;
		ID3D11RenderTargetView* m_renderTargetView;


		// This could stay
		ID3D11Texture2D* m_depthBuffer;
		ID3D11DepthStencilView* m_depthBufferStencil;

		// Rasterizer state
		ID3D11RasterizerState* m_rasterizerState;

		// For debug
		ID3D11InfoQueue* m_infoQueue;

		FLOAT m_clearBackgroundColor[ 4 ] = { 0.1f, 0.2f, 0.6f, 1.0f };
		D3D11_VIEWPORT m_viewport = {
			0.0f,
			0.0f,
			2.0f, // width
			2.0f, // height
			0.0f,
			1.0f
		};


		sD3D11ResourceMap<ID3D11VertexShader> m_vertexShaderMap;
		sD3D11ResourceMap<ID3DBlob> m_vertexShaderBlobMap;
		sD3D11ResourceMap<ID3D11PixelShader> m_pixelShaderMap;
		sD3D11ResourceMap<ID3D11InputLayout> m_inputLayoutMap;
		sD3D11ResourceMap<ID3D11Buffer> m_dataBufferMap;
		sD3D11ResourceMap<ID3D11Texture2D> m_texture2dMap;
		sD3D11ResourceMap<ID3D11ShaderResourceView> m_textureShaderResourceViewMap;
		sD3D11ResourceMap<ID3D11SamplerState> m_textureSamplerResourceMap;

		sD3D11ResourceMap<sD3D11RenderTargetData> m_renderTargetData;
		sD3D11ResourceMap<sMeshData> m_meshData;

		void reflectShader( wv::sShaderProgram* _program, ID3DBlob* _shader );
		std::string getAllErrors();
#endif

		virtual bool initialize( GraphicsDeviceDesc* _desc ) override;

		sPipeline* m_activePipeline = nullptr;
		RenderTarget* m_activeRenderTarget = nullptr;

		int m_numTotalUniformBlocks = 0;

		std::unordered_map<int, wv::Handle> m_boundTextureSlots;
	};
}