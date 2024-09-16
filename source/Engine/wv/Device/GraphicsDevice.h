#pragma once

#include <wv/Types.h>

#include <wv/Shader/Shader.h>
#include <wv/Shader/ShaderProgram.h>

#include <wv/Misc/Color.h>
#include <wv/Graphics/GPUBuffer.h>

#include <wv/Graphics/CommandBuffer.h>

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <shared_mutex>

///////////////////////////////////////////////////////////////////////////////////////

namespace wv
{

///////////////////////////////////////////////////////////////////////////////////////

	class Primitive;
	class Texture;
	class RenderTarget;
	class cMaterial;

	struct PipelineDesc;
	struct PrimitiveDesc;
	struct TextureDesc;
	struct RenderTargetDesc;
	
	struct iDeviceContext;
	struct sMeshNode;
	struct sMesh;

	struct sShaderProgramSource;
	struct sShaderProgram;

	struct sPipelineDesc;
	struct sPipeline;

	struct sGPUBufferDesc;

	
///////////////////////////////////////////////////////////////////////////////////////

	struct GraphicsDeviceDesc
	{
		GraphicsDriverLoadProc loadProc;
		iDeviceContext* pContext;
	};

///////////////////////////////////////////////////////////////////////////////////////

	class iGraphicsDevice
	{
	public:

		virtual ~iGraphicsDevice() { };

		static iGraphicsDevice* createGraphicsDevice( GraphicsDeviceDesc* _desc );

		void initEmbeds();

		void draw( sMesh* _mesh );
		void drawNode( sMeshNode* _node );

		std::thread::id getThreadID() { return m_threadID; }

		[[nodiscard]] uint32_t getCommandBuffer();
		
		void submitCommandBuffer( uint32_t& _buffer );
		void executeCommandBuffer( uint32_t& _index );

		template<typename R, typename T>
		void bufferCommand( uint32_t& _buffer, const eGPUTaskType& _type, R** _ppReturn, T* _pInfo );
		template<typename T>
		void bufferCommand( uint32_t& _buffer, const eGPUTaskType& _type, T* _pInfo ) { bufferCommand<T, T>( _buffer, _type, nullptr, _pInfo ); }
		void bufferCommand( uint32_t& _buffer, const eGPUTaskType& _type ) { bufferCommand<char, char>( _buffer, _type, nullptr, nullptr ); }

		void setCommandBufferCallback( uint32_t& _buffer, wv::Function<void, void*>::fptr_t _func, void* _caller );

		virtual void terminate() = 0;

		virtual void onResize( int _width, int _height ) = 0;
		virtual void setViewport( int _width, int _height ) = 0;

		virtual void beginRender();
		virtual void endRender();

		virtual RenderTarget* createRenderTarget( RenderTargetDesc* _desc ) = 0;
		virtual void destroyRenderTarget( RenderTarget** _renderTarget ) = 0;

		virtual void setRenderTarget( RenderTarget* _target ) = 0;
		virtual void setClearColor( const wv::cColor& _color ) = 0;
		virtual void clearRenderTarget( bool _color, bool _depth ) = 0;

		virtual sShaderProgram* createProgram( sShaderProgramDesc* _desc ) = 0;
		virtual void destroyProgram( sShaderProgram* _pProgram ) = 0;

		virtual sPipeline* createPipeline( sPipelineDesc* _desc ) = 0;
		virtual void destroyPipeline( sPipeline* _pPipeline ) = 0;
		virtual void bindPipeline   ( sPipeline* _pPipeline ) = 0;

		virtual cGPUBuffer* createGPUBuffer ( sGPUBufferDesc* _desc ) = 0;
		virtual void        allocateBuffer  ( cGPUBuffer* _buffer, size_t _size ) = 0;
		virtual void        bufferData      ( cGPUBuffer* _buffer ) = 0;
		virtual void        destroyGPUBuffer( cGPUBuffer* _buffer ) = 0;
		
		virtual sMesh* createMesh() = 0;
		virtual void destroyMesh( sMesh** _mesh ) = 0;

		virtual Primitive* createPrimitive( PrimitiveDesc* _desc ) = 0;
		virtual void destroyPrimitive( Primitive* _primitive ) = 0;

		virtual void createTexture( Texture* _pTexture, TextureDesc* _desc ) = 0;
		virtual void destroyTexture( Texture** _texture ) = 0;

		virtual void bindTextureToSlot( Texture* _texture, unsigned int _slot ) = 0;

		virtual void drawPrimitive( Primitive* _primitive ) = 0;

///////////////////////////////////////////////////////////////////////////////////////
		
	protected:

		iGraphicsDevice() { };

		std::thread::id m_threadID;

		virtual bool initialize( GraphicsDeviceDesc* _desc ) = 0;

		GraphicsAPI    m_graphicsApi;
		GenericVersion m_graphicsApiVersion;

		std::mutex m_mutex;
		bool m_reallocatingCommandBuffers = false;

		std::vector<cCommandBuffer> m_commandBuffers;
		std::queue <uint32_t>       m_availableCommandBuffers;
		std::vector<uint32_t>       m_recordingCommandBuffers;
		std::vector<uint32_t>       m_submittedCommandBuffers;

		cMaterial* m_emptyMaterial;
	};
	template<typename R, typename T>
	inline void iGraphicsDevice::bufferCommand( uint32_t& _buffer, const eGPUTaskType& _type, R** _ppReturn, T* _pInfo )
	{
		m_mutex.lock();
		m_commandBuffers[ _buffer ].push<R, T>( _type, _ppReturn, _pInfo );
		m_mutex.unlock();
	}
}