#pragma once

#include <wv/Events/MouseListener.h>
#include <wv/Events/InputListener.h>

#include <wv/Math/Vector2.h>

#include <wv/Types.h>

namespace wv
{

///////////////////////////////////////////////////////////////////////////////////////

	class iDeviceContext;
	class iGraphicsDevice;
	class AudioDevice;

	class iIntermediateRenderTargetHandler;

	class Mesh;
	class ICamera;
	class RenderTarget;
	class cFileSystem;
	class cSceneRoot;
	class cModelObject;
	class iMaterial;

	class cApplicationState;

	class cShaderRegistry;
	class cShaderProgram;

	class cMaterialRegistry;
	class cJoltPhysicsEngine;

///////////////////////////////////////////////////////////////////////////////////////

	struct EngineDesc
	{
		const char* title;
		
		int windowWidth  = 800;
		int windowHeight = 600;

		bool showDebugConsole = true;

		struct
		{
			iDeviceContext*  pContext;
			iGraphicsDevice* pGraphics;
			AudioDevice*     pAudio;
		} device;

		struct
		{
			cFileSystem* pFileSystem;
			cShaderRegistry* pShaderRegistry;
		} systems;

		/// <summary>
		/// Optional intermediate render target. Can be used to render to viewport
		/// </summary>
		iIntermediateRenderTargetHandler* pIRTHandler = nullptr;

		cApplicationState* pApplicationState = nullptr;
	};

///////////////////////////////////////////////////////////////////////////////////////

	class cEngine : IMouseListener, IInputListener
	{

	public:

		cEngine( EngineDesc* _desc );
		static cEngine* get();
		
		static wv::UUID   getUniqueUUID();
		static wv::Handle getUniqueHandle();

		void onResize( int _width, int _height );
		void onMouseEvent( MouseEvent _event ) override;
		void onInputEvent( InputEvent _event ) override;

		void setSize( int _width, int _height, bool _notify = true );

		wv::Vector2i getMousePosition() { return m_mousePosition; }

		wv::Vector2i getViewportSize();
		float getViewportAspect()
		{
			wv::Vector2i size = getViewportSize();
			return ( float )size.x / ( float )size.y;
		}

		void run();
		void terminate();
		void tick();
		void quit();

///////////////////////////////////////////////////////////////////////////////////////

		// deferred rendering
		Mesh*           m_screenQuad      = nullptr;
		cShaderProgram* m_deferredProgram = nullptr;
		RenderTarget*   m_gbuffer         = nullptr;

		// engine
		iDeviceContext*  context  = nullptr;
		iGraphicsDevice* graphics = nullptr;
		AudioDevice*     audio    = nullptr;

		// camera 
		/// TODO: move to applicationstate
		ICamera* currentCamera    = nullptr;
		ICamera* orbitCamera      = nullptr;
		ICamera* freeflightCamera = nullptr;

		iIntermediateRenderTargetHandler* m_pIRTHandler = nullptr;
		RenderTarget* m_pScreenRenderTarget = nullptr;

		cApplicationState* m_pApplicationState = nullptr;

		// modules
		cFileSystem*        m_pFileSystem       = nullptr;
		cShaderRegistry*    m_pShaderRegistry   = nullptr;
		cMaterialRegistry*  m_pMaterialRegistry = nullptr;
		cJoltPhysicsEngine* m_pPhysicsEngine    = nullptr;

///////////////////////////////////////////////////////////////////////////////////////

	private:

		void initImgui();
		void shutdownImgui();

		void createScreenQuad();
		void createGBuffer();

		void recreateScreenRenderTarget( int _width, int _height );

///////////////////////////////////////////////////////////////////////////////////////

	#define FPS_CACHE_NUM 200
		unsigned int m_fpsCacheCounter = 0;
		double m_fpsCache[ FPS_CACHE_NUM ] = { 0.0 };
		double m_averageFps = 0.0;
		double m_maxFps = 0.0;

		/*
		 * technically not a singleton but getting a reference 
		 * to the application can sometimes be very useful.
		 * 
		 * this will have to change in case multiple applications 
		 * are to be supported
		 * 
		 * might remove
		 */
		static cEngine* s_pInstance; 

		wv::Vector2i m_mousePosition;

	};

}