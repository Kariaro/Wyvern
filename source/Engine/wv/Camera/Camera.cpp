#include "Camera.h"

#include <wv/Engine/Engine.h>
#include <wv/Device/DeviceContext.h>

///////////////////////////////////////////////////////////////////////////////////////

wv::ICamera::ICamera( CameraType _type, float _fov, float _near, float _far ) :
	m_type{ _type },
	fov{ _fov },
	m_near{ _near },
	m_far{ _far }
{
	
}

///////////////////////////////////////////////////////////////////////////////////////

wv::cMatrix4x4f wv::ICamera::getProjectionMatrix( void )
{
	switch( m_type )
	{
	case WV_CAMERA_TYPE_PERSPECTIVE:  return getPerspectiveMatrix (); break;
	case WV_CAMERA_TYPE_ORTHOGRAPHIC: return getOrthographicMatrix(); break;
	}

	return cMatrix4x4f{ 1.0f };
}

///////////////////////////////////////////////////////////////////////////////////////

wv::cMatrix4x4f wv::ICamera::getPerspectiveMatrix( void )
{
	wv::cEngine* engine = wv::cEngine::get();
	
	return Matrix::perspective( engine->getViewportAspect(), Math::radians( fov ), m_near, m_far );
}

///////////////////////////////////////////////////////////////////////////////////////

wv::cMatrix4x4f wv::ICamera::getOrthographicMatrix( void )
{
	wv::iDeviceContext* ctx = wv::cEngine::get()->context;
	float w = (float)ctx->getWidth()  / 2.0f;
	float h = (float)ctx->getHeight() / 2.0f;

	return Matrix::orthographic( w, h, -1000.0f, 1000.0f );
}

///////////////////////////////////////////////////////////////////////////////////////

wv::cMatrix4x4f wv::ICamera::getViewMatrix( void )
{
	m_transform.update();
	return Matrix::inverse( m_transform.getMatrix() );
}

///////////////////////////////////////////////////////////////////////////////////////

wv::cVector3f wv::ICamera::getViewDirection()
{
	float yaw   = Math::radians( m_transform.rotation.y - 90.0f );
	float pitch = Math::radians( m_transform.rotation.x );

	cVector3f direction;
	direction.x = std::cos( yaw ) * std::cos( pitch );
	direction.y = std::sin( pitch );
	direction.z = std::sin( yaw ) * std::cos( pitch );
    return direction;
}
