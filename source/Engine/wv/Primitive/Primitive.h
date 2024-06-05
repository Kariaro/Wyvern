#pragma once

#include <wv/Types.h>

namespace wv
{
	class IMaterial;

	struct InputLayoutElement
	{
		unsigned int num;
		DataType type;
		bool normalized;
		unsigned int size;
	};

	struct InputLayout
	{
		InputLayoutElement* elements;
		unsigned int numElements;
	};
	
	////////////////////////////////////////////

	enum PrimitiveBufferMode
	{
		WV_PRIMITIVE_TYPE_STATIC
	};

	enum PrimitiveDrawType
	{
		WV_PRIMITIVE_DRAW_TYPE_VERTICES,
		WV_PRIMITIVE_DRAW_TYPE_INDICES
	};

	struct PrimitiveDesc
	{
		PrimitiveBufferMode type = WV_PRIMITIVE_TYPE_STATIC;
		InputLayout* layout = nullptr;

		void* vertexBuffer = nullptr;
		unsigned int vertexBufferSize = 0;
		unsigned int numVertices = 0;

		void* indexBuffer = nullptr;
		unsigned int indexBufferSize = 0;
		unsigned int numIndices = 0;
	};

	class Primitive
	{
	public:
		wv::Handle vboHandle = 0;
		wv::Handle eboHandle = 0;
		PrimitiveBufferMode mode = WV_PRIMITIVE_TYPE_STATIC;
		PrimitiveDrawType drawType = WV_PRIMITIVE_DRAW_TYPE_VERTICES;
		uint32_t numVertices = 0;
		uint32_t numIndices = 0;
		uint32_t stride = 0;
		IMaterial* material = nullptr;
	};
}