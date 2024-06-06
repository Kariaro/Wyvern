#pragma once

#include <wv/Math/Vector2.h>
#include <wv/Math/Vector3.h>
#include <wv/Math/Vector4.h>

#include <string>
#include <vector>

namespace wv
{
	class Mesh;

	namespace assimp
	{
		class Parser
		{
		public:
			Parser() { }
			
			Mesh* load( const char* _path );
		};
	}
}