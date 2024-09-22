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
		std::vector<sD3D11AdapterData> m_d3d11adapters;

		ID3D11Device* m_device;
		IDXGISwapChain* m_swapChain;
		ID3D11DeviceContext* m_deviceContext;
		ID3D11Texture2D* m_backBuffer;
		ID3D11RenderTargetView* m_renderTargetView;

		ID3D11RasterizerState* m_rasterizerState;

		FLOAT m_clearBackgroundColor[ 4 ] = { 0.1f, 0.2f, 0.6f, 1.0f };

		template <typename _Type>
		struct ResourceMap
		{
			using _MapType = std::unordered_map<wv::Handle, _Type*>;
			using _MapIter = typename _MapType::local_iterator;

			uint32_t count{ 0 };
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

		ResourceMap<ID3D11VertexShader> m_vertexShaderMap;
		ResourceMap<ID3DBlob> m_vertexShaderBlobMap;
		ResourceMap<ID3D11PixelShader> m_pixelShaderMap;
		ResourceMap<ID3D11InputLayout> m_inputLayoutMap;
		ResourceMap<ID3D11Buffer> m_dataBufferMap;

		
		struct MeshData
		{
			DXGI_FORMAT index_format;

			void Release()
			{
				// using A = std::unordered_map<wv::Handle, std::string>;
				// A test;
				// A::local_iterator a = test.find( 0 );
				
				delete this;
			}
		};

		ResourceMap<MeshData> m_meshData;

		
		ID3D11InfoQueue* m_infoQueue;

		void reflectShader( wv::sShaderProgram* _program, ID3DBlob* _shader );
#endif

		virtual bool initialize( GraphicsDeviceDesc* _desc ) override;

		template<typename... Args>
		bool assertGLError( const std::string _msg, Args..._args );
		bool getError( std::string* _out );

		std::string getAllErrors();

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