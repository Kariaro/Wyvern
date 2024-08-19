#include "SceneObject.h"

///////////////////////////////////////////////////////////////////////////////////////

wv::iSceneObject::iSceneObject( const uint64_t& _uuid, const std::string& _name ):
	m_uuid{ _uuid },
	m_name{ _name }
{
	
}

///////////////////////////////////////////////////////////////////////////////////////

wv::iSceneObject::~iSceneObject()
{

}

///////////////////////////////////////////////////////////////////////////////////////

void wv::iSceneObject::addChild( iSceneObject* _node )
{
	if ( !_node )
		return;
	for ( size_t i = 0; i < m_children.size(); i++ )
		if ( m_children[ i ] == _node ) return; // node already has child
	m_children.push_back( _node );

	_node->m_parent = this;
	_node->m_transform.parent = &m_transform;
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::iSceneObject::removeChild( iSceneObject* _node )
{
	if ( !_node )
		return;

	for ( size_t i = 0; i < m_children.size(); i++ )
	{
		if ( m_children[ i ] != _node )
			continue;

		m_children.erase( m_children.begin() + i );
		_node->m_parent = nullptr;
		return;
	}
}

///////////////////////////////////////////////////////////////////////////////////////

void wv::iSceneObject::moveChild( iSceneObject* _node, iSceneObject* _newParent )
{
	if ( !_node || !_newParent )
		return;

	removeChild( _node );
	_newParent->addChild( _node );
}
