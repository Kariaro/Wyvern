#pragma once

#include "SceneObject.h"

#include <string>
#include <vector>

#include <wv/Reflection/Reflection.h>

///////////////////////////////////////////////////////////////////////////////////////

namespace wv
{

///////////////////////////////////////////////////////////////////////////////////////

	class Mesh;
	class cMaterial;

///////////////////////////////////////////////////////////////////////////////////////

	class cSkyboxObject : public iSceneObject
	{

	public:

		 cSkyboxObject( const UUID& _uuid, const std::string& _name );
		~cSkyboxObject();

///////////////////////////////////////////////////////////////////////////////////////

		static cSkyboxObject* parseInstance( sParseData& _data );

///////////////////////////////////////////////////////////////////////////////////////

	protected:

		void onLoadImpl   () override;
		void onUnloadImpl () override;
		void onCreateImpl () override;
		void onDestroyImpl() override;

		virtual void updateImpl( double _deltaTime ) override;
		virtual void drawImpl  ( iDeviceContext* _context, iGraphicsDevice* _device ) override;

		Mesh*      m_skyboxMesh  = nullptr;
		cMaterial* m_skyMaterial = nullptr;

	};

	REFLECT_CLASS( cSkyboxObject );

}