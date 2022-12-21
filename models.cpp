#include "models.h"

static gltfviewer_handle nextHandle = 100;

namespace gltfviewer {

Models::Models()
{
}

Models::~Models()
{
}

gltfviewer_handle Models::AddModel( std::unique_ptr<Model> model )
{
  std::lock_guard<std::mutex> lock( m_mutex );
  const auto it = m_models.insert( { nextHandle++, std::move( model ) } ).first;
  if ( m_models.end() != it ) {
    return it->first;
  }

  return gltfviewer_error;
}

void Models::FreeModel( const gltfviewer_handle model_handle )
{
  std::lock_guard<std::mutex> lock( m_mutex );
  m_models.erase( model_handle );
}

Model* const Models::FindModel( const gltfviewer_handle model_handle )
{
  const auto it = m_models.find( model_handle );
  if ( m_models.end() == it )
    return nullptr;

  return it->second.get();
}


} // namespace gltfviewer