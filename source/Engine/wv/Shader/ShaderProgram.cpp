#include "ShaderProgram.h"

#include <wv/Memory/FileSystem.h>

#include <wv/Auxiliary/json/json11.hpp>
#include <wv/Engine/Engine.h>
#include <wv/Device/GraphicsDevice.h>

void wv::cProgramPipeline::load( cFileSystem* _pFileSystem, iGraphicsDevice* _pGraphicsDevice )
{
	Debug::Print( Debug::WV_PRINT_DEBUG, "Loading Shader '%s'\n", m_name.c_str() );

	std::string basepath = "res/shaders/";
	std::string ext;

#ifdef WV_PLATFORM_WINDOWS
	ext = ".glsl";
#elif defined( WV_PLATFORM_PSVITA )
	ext = "_cg.gxp";
	basepath += "psvita/";
#endif

	m_vsSource.data = _pFileSystem->loadMemory( basepath + m_name + "_vs" + ext );
	m_fsSource.data = _pFileSystem->loadMemory( basepath + m_name + "_fs" + ext );



	sShaderProgramDesc vsDesc;
	vsDesc.source = m_vsSource;
	vsDesc.type = WV_SHADER_TYPE_VERTEX;

	sShaderProgramDesc fsDesc;
	fsDesc.source = m_fsSource;
	fsDesc.type = WV_SHADER_TYPE_FRAGMENT;

	sPipelineDesc desc;
	desc.name = m_name;

	/// TODO: generalize 
	wv::sVertexAttribute attributes[] = {
			{ "aPosition",  3, wv::WV_FLOAT, false, sizeof( float ) * 3 }, // vec3f pos
			{ "aNormal",    3, wv::WV_FLOAT, false, sizeof( float ) * 3 }, // vec3f normal
			{ "aTangent",   3, wv::WV_FLOAT, false, sizeof( float ) * 3 }, // vec3f tangent
			{ "aColor",     4, wv::WV_FLOAT, false, sizeof( float ) * 4 }, // vec4f col
			{ "aTexCoord0", 2, wv::WV_FLOAT, false, sizeof( float ) * 2 }  // vec2f texcoord0
	};
	wv::sVertexLayout layout;
	layout.elements = attributes;
	layout.numElements = 5;

	desc.pVertexLayout    = &layout;
	desc.pVertexProgram   = &m_vs;
	desc.pFragmentProgram = &m_fs;

	uint32_t cmdBuffer = _pGraphicsDevice->getCommandBuffer();
	_pGraphicsDevice->bufferCommand( cmdBuffer, WV_GPUTASK_CREATE_PROGRAM, &m_vs, &vsDesc );
	_pGraphicsDevice->bufferCommand( cmdBuffer, WV_GPUTASK_CREATE_PROGRAM, &m_fs, &fsDesc );
	_pGraphicsDevice->bufferCommand( cmdBuffer, WV_GPUTASK_CREATE_PIPELINE, &m_pPipeline, &desc );

	/// this is disgusting
	auto cb =
		[]( void* _c ) 
		{ 
			iResource* res = (iResource*)_c;
			res->setComplete( true ); 
		};

	_pGraphicsDevice->setCommandBufferCallback( cmdBuffer, cb, (void*)this );

	_pGraphicsDevice->submitCommandBuffer( cmdBuffer );

	if( _pGraphicsDevice->getThreadID() == std::this_thread::get_id() )
		_pGraphicsDevice->executeCommandBuffer( cmdBuffer );
}

void wv::cProgramPipeline::unload( cFileSystem* _pFileSystem, iGraphicsDevice* _pGraphicsDevice )
{
	
}

void wv::cProgramPipeline::use( iGraphicsDevice* _pGraphicsDevice )
{
	_pGraphicsDevice->bindPipeline( m_pPipeline );
}

wv::cGPUBuffer* wv::cProgramPipeline::getShaderBuffer( const std::string& _name )
{
	if ( m_pPipeline->pVertexProgram )
	{
		for( auto& buf : m_pPipeline->pVertexProgram->shaderBuffers )
		{
			if( buf->name == _name )
				return buf;
		}
	}

	if ( m_pPipeline->pFragmentProgram )
	{
		for ( auto& buf : m_pPipeline->pFragmentProgram->shaderBuffers )
		{
			if ( buf->name == _name )
				return buf;
		}
	}

	return nullptr;
}