#include "mesh.h"

#include "MikkTSpace/mikktspace.h"

namespace gltfviewer {

struct UserData {
  Mesh*   mesh = nullptr;
  size_t  textureCoordinateIndex = 0;
};

static int getNumFaces( const SMikkTSpaceContext* pContext )
{
  const UserData* data = static_cast<const UserData*>( pContext->m_pUserData );
  Mesh* mesh = data->mesh;
  const int faces = data->mesh->m_indices.size() / 3;
  return faces;
}

static int getNumVerticesOfFace( const SMikkTSpaceContext* pContext, const int iFace )
{
  return 3;
}

void getPosition( const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert )
{
  const UserData* data = static_cast<const UserData*>( pContext->m_pUserData );
  Mesh* mesh = data->mesh;
  if ( const size_t index = mesh->m_indices[ iFace * 3 + iVert ]; index < mesh->m_positions.size() ) {
    fvPosOut[ 0 ] = mesh->m_positions[ index ].x;
    fvPosOut[ 1 ] = mesh->m_positions[ index ].y;
    fvPosOut[ 2 ] = mesh->m_positions[ index ].z;
  }
}

void getNormal( const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert )
{
  const UserData* data = static_cast<const UserData*>( pContext->m_pUserData );
  Mesh* mesh = data->mesh;
  if ( const size_t index = mesh->m_indices[ iFace * 3 + iVert ]; index < mesh->m_normals.size() ) {
    fvNormOut[ 0 ] = mesh->m_normals[ index ].x;
    fvNormOut[ 1 ] = mesh->m_normals[ index ].y;
    fvNormOut[ 2 ] = mesh->m_normals[ index ].z;
  }
}

void getTexCoord( const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert )
{
  const UserData* data = static_cast<const UserData*>( pContext->m_pUserData );
  Mesh* mesh = data->mesh;
  if ( const size_t index = mesh->m_indices[ iFace * 3 + iVert ]; index < mesh->m_texcoords[ data->textureCoordinateIndex ].size() ) {
    fvTexcOut[ 0 ] = mesh->m_texcoords[ data->textureCoordinateIndex ][ index ].x;
    fvTexcOut[ 1 ] = -mesh->m_texcoords[ data->textureCoordinateIndex ][ index ].y;
  }
}

void setTSpaceBasic( const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert )
{
  UserData* data = static_cast<UserData*>( pContext->m_pUserData );
  Mesh* mesh = data->mesh;
  if ( const size_t index = mesh->m_indices[ iFace * 3 + iVert ]; index < mesh->m_tangents.size() ) {  
    const Vec4 tangent = { fvTangent[ 0 ], fvTangent[ 1 ], fvTangent[ 2 ], fSign };
    mesh->m_tangents[ index ] = tangent;
  }
}

void Mesh::GenerateTangents( const size_t textureCoordinateIndex )
{
  if ( textureCoordinateIndex < m_texcoords.size() ) {
    m_tangents.resize( m_positions.size() );

    SMikkTSpaceInterface mikkInterface = {};
    mikkInterface.m_getNumFaces = getNumFaces;
    mikkInterface.m_getNumVerticesOfFace = getNumVerticesOfFace;
    mikkInterface.m_getPosition = getPosition;
    mikkInterface.m_getNormal = getNormal;
    mikkInterface.m_getTexCoord = getTexCoord;
    mikkInterface.m_setTSpaceBasic = setTSpaceBasic;
    mikkInterface.m_setTSpace = nullptr;

    UserData userData = { this, textureCoordinateIndex };
    SMikkTSpaceContext context = {};
    context.m_pUserData = &userData;
    context.m_pInterface = &mikkInterface;
    genTangSpaceDefault( &context );
  }
}

} // namespace gltfviewer
