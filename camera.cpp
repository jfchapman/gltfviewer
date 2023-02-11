#include "camera.h"

#include "model.h"

namespace gltfviewer {

Camera::Camera( const Microsoft::glTF::Camera& camera, const Matrix& matrix ) : m_matrix( matrix )
{
  if ( camera.projection ) {
    if ( Microsoft::glTF::PROJECTION_PERSPECTIVE == camera.projection->GetProjectionType() ) {
      m_projection = Projection::Perspective;
      const auto& perspective = camera.GetPerspective();
      m_perspectiveFOV = RadianToDegree( perspective.yfov );
      m_nearclip = perspective.znear;
      if ( perspective.zfar.HasValue() ) {
        m_farclip = perspective.zfar.Get();
      }
    } else if ( Microsoft::glTF::PROJECTION_ORTHOGRAPHIC == camera.projection->GetProjectionType() ) {
      m_projection = Projection::Orthographic;
      const auto& orthographic = camera.GetOrthographic();
      m_nearclip = orthographic.znear;
      m_farclip = orthographic.zfar;
      m_orthographicWidth = orthographic.xmag;
      m_orthographicHeight = orthographic.ymag;
    }
  }
}

Camera::Camera( const gltfviewer_camera& camera, const Model& model, const uint32_t viewWidth, const uint32_t viewHeight ) :
  m_preset( camera.preset ),
  m_projection( ( gltfviewer_camera_projection_orthographic == camera.projection ) ? Projection::Orthographic : Projection::Perspective ),
  m_matrix( camera.matrix ),
  m_orthographicWidth( camera.orthographic_width ),
  m_orthographicHeight( camera.orthographic_height )
{
  if ( gltfviewer_camera_preset_none != m_preset ) {
    CalculatePresetViewPosition( model, viewWidth, viewHeight );
  } else {
    if ( std::isnormal( camera.nearclip ) ) {
      m_nearclip = camera.nearclip;
    }
    if ( std::isnormal( camera.farclip ) && ( camera.farclip > camera.nearclip ) ) {
      m_farclip = camera.farclip;
    }
    m_perspectiveFOV = std::clamp( camera.perspective_fov, 1.0f, 180.0f );
  }
}

Camera::operator gltfviewer_camera() const
{
  gltfviewer_camera camera = {};
  camera.projection = ( Projection::Orthographic == m_projection ) ? gltfviewer_camera_projection_orthographic : gltfviewer_camera_projection_perspective;
  camera.preset = m_preset;
  const auto& m = m_matrix.GetValues();
  size_t k = 0;
  for ( size_t i = 0; i < 4; i++ ) {
    for ( size_t j = 0; j < 4; j++, k++ ) {
      camera.matrix[ k ] = m[ j ][ i ];
    }
  }
  camera.farclip = m_farclip;
  camera.nearclip = m_nearclip;
  camera.perspective_fov = GetFOVDegrees();
  camera.orthographic_width = m_orthographicWidth;
  camera.orthographic_height = m_orthographicHeight;
  return camera;
}

Matrix Camera::LookAt( const Vec3& from, const Vec3& to, const Vec3& up )
{
  Matrix matrix;
  auto& m = matrix.GetValues();

  auto forward = from - to;
  forward.Normalise();
  auto right = up.CrossProduct( forward );
  right.Normalise();
  const auto u = forward.CrossProduct( right );

  m[ 0 ][ 0 ] = right.x;
  m[ 1 ][ 0 ] = right.y;
  m[ 2 ][ 0 ] = right.z; 
  
  m[ 0 ][ 1 ] = u.x;
  m[ 1 ][ 1 ] = u.y;
  m[ 2 ][ 1 ] = u.z;

  m[ 0 ][ 2 ] = forward.x;
  m[ 1 ][ 2 ] = forward.y;
  m[ 2 ][ 2 ] = forward.z;

  m[ 0 ][ 3 ] = from.x;
  m[ 1 ][ 3 ] = from.y;
  m[ 2 ][ 3 ] = from.z; 

  return matrix;
}

static float ToHorizontalFieldOfView( const float verticalFieldOfView, const float viewAspect )
{
  if ( verticalFieldOfView >= M_PI_2 )
    return M_PI_2;

  return 2 * atanf( tanf( verticalFieldOfView / 2 ) * viewAspect );
}

static float ToVerticalFieldOfView( const float horizontalFieldOfView, const float viewAspect )
{
  if ( viewAspect <= 0 )
    return horizontalFieldOfView;

  if ( horizontalFieldOfView >= M_PI_2 )
    return M_PI_2;

  return 2 * atanf( tanf( horizontalFieldOfView / 2 ) / viewAspect );
}

void Camera::CalculatePresetViewPosition( const Model& model, const uint32_t viewWidth, const uint32_t viewHeight )
{
  if ( gltfviewer_camera_preset_none != m_preset ) {
    const float viewAspect = ( viewHeight > 0 ) ? ( static_cast<float>( viewWidth ) / viewHeight ) : 1.0f;
    const auto [ minBounds, maxBounds ] = model.GetBounds();
    const Vec3 sceneCentre = { ( minBounds.x + maxBounds.x ) / 2, ( minBounds.y + maxBounds.y ) / 2, ( minBounds.z + maxBounds.z ) / 2 };
    switch ( m_preset ) {
      case gltfviewer_camera_preset_front : 
      case gltfviewer_camera_preset_back : {
        const float sceneWidth = maxBounds.x - minBounds.x;
        const float sceneHeight = maxBounds.y - minBounds.y;
        const float sceneAspect = ( sceneHeight > 0 ) ? ( sceneWidth / sceneHeight ) : 1.0f;
        float distance = 0;
        if ( sceneAspect > viewAspect ) {
          const float halfExtent = sceneWidth / 2;
          distance = halfExtent / tanf( ToHorizontalFieldOfView( GetFOVRadians(), viewAspect ) / 2 );
        } else {
          const float halfExtent = sceneHeight / 2;
          distance = halfExtent / tanf( GetFOVRadians() / 2 );
        }
        Vec3 to = sceneCentre;
        to.z = ( gltfviewer_camera_preset_front == m_preset ) ? maxBounds.z : minBounds.z;
        Vec3 from = to;
        from.z += distance * ( ( gltfviewer_camera_preset_front == m_preset ) ? 1.0f : -1.0f );
        m_matrix = LookAt( from, to, { 0, 1.0f, 0 } );
        break;
      }

      case gltfviewer_camera_preset_left :
      case gltfviewer_camera_preset_right : {
        const float sceneWidth = maxBounds.z - minBounds.z;
        const float sceneHeight = maxBounds.y - minBounds.y;
        const float sceneAspect = ( sceneHeight > 0 ) ? ( sceneWidth / sceneHeight ) : 1.0f;
        float distance = 0;
        if ( sceneAspect > viewAspect ) {
          const float halfExtent = sceneWidth / 2;
          distance = halfExtent / tanf( ToHorizontalFieldOfView( GetFOVRadians(), viewAspect ) / 2 );
        } else {
          const float halfExtent = sceneHeight / 2;
          distance = halfExtent / tanf( GetFOVRadians() / 2 );
        }
        Vec3 to = sceneCentre;
        to.x = ( gltfviewer_camera_preset_left == m_preset ) ? maxBounds.x : minBounds.x;
        Vec3 from = to;
        from.x += distance * ( ( gltfviewer_camera_preset_left == m_preset ) ? 1.0f : -1.0f );
        m_matrix = LookAt( from, to, { 0, 1.0f, 0 } );
        break;
      }

      case gltfviewer_camera_preset_top : 
      case gltfviewer_camera_preset_bottom : {
        const float sceneWidth = maxBounds.x - minBounds.x;
        const float sceneHeight = maxBounds.z - minBounds.z;
        const float sceneAspect = ( sceneHeight > 0 ) ? ( sceneWidth / sceneHeight ) : 1.0f;
        float distance = 0;
        if ( sceneAspect > viewAspect ) {
          const float halfExtent = sceneWidth / 2;
          distance = halfExtent / tanf( ToHorizontalFieldOfView( GetFOVRadians(), viewAspect ) / 2 );
        } else {
          const float halfExtent = sceneHeight / 2;
          distance = halfExtent / tanf( GetFOVRadians() / 2 );
        }
        Vec3 to = sceneCentre;
        to.y = ( gltfviewer_camera_preset_top == m_preset ) ? maxBounds.y : minBounds.y;
        Vec3 from = to;
        from.y += distance * ( ( gltfviewer_camera_preset_top == m_preset ) ? 1.0f : -1.0f );
        m_matrix = LookAt( from, to, { 0, 0, -1.0f } );
        break;
      }
      default:
        break;
    }
  }
}


} // namespace gltfviewer
