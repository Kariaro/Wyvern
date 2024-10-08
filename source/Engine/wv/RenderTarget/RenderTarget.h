#pragma once

#include <wv/Types.h>
#include <wv/Texture/Texture.h>

///////////////////////////////////////////////////////////////////////////////////////

namespace wv
{

///////////////////////////////////////////////////////////////////////////////////////

	class RenderTarget;

	struct RenderTargetDesc
	{
		int width = 0;
		int height = 0;
		
		wv::TextureDesc* pTextureDescs = nullptr;
		int numTextures = 0;
	};

///////////////////////////////////////////////////////////////////////////////////////

	class RenderTarget
	{

	public:
		wv::Handle fbHandle = 0;
		wv::Handle rbHandle = 0;

		wv::Texture** textures = 0;
		int numTextures = 0;

		int width = 0;
		int height = 0;

		void* pPlatformData = nullptr;

	};

}