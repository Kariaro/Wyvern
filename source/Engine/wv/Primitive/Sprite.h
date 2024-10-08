#pragma once

#include <wv/Math/Transform.h>

namespace wv
{

///////////////////////////////////////////////////////////////////////////////////////

	class sMesh;
	class cMaterial;
	class iGraphicsDevice;

///////////////////////////////////////////////////////////////////////////////////////

	namespace Internal
	{
		static sMesh* S_SPRITE_QUAD = nullptr;
	}

///////////////////////////////////////////////////////////////////////////////////////

	struct SpriteDesc
	{
		sMesh* mesh = nullptr;
		cMaterial* material = nullptr;
		cVector3f position{};
		cVector3f size{ 64.0f, 64.0f, 1.0f };
	};

///////////////////////////////////////////////////////////////////////////////////////

	class Sprite
	{

	public:

		~Sprite();

		static Sprite* create( SpriteDesc* _desc );

		Transformf& getTransform() { return m_transform; }
		void draw( iGraphicsDevice* _device );
		cMaterial* getMaterial() { return m_material; }

///////////////////////////////////////////////////////////////////////////////////////

	private:

		Sprite() { };

		static void createQuad();

		cMaterial* m_material = nullptr;
		Transformf m_transform;
		sMesh* m_mesh = nullptr;

	};

}
