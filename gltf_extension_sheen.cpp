#include "gltf_extension_sheen.h"

#include <nlohmann/json.hpp>

namespace gltfviewer {

std::unique_ptr<Microsoft::glTF::Extension> KHR_materials_sheen::Clone() const
{
    return std::make_unique<KHR_materials_sheen>( *this );
}

bool KHR_materials_sheen::IsEqual( const Microsoft::glTF::Extension& rhs ) const
{
  const KHR_materials_sheen* extension = dynamic_cast<const KHR_materials_sheen*>( &rhs );
  return 
    ( nullptr != extension ) &&
    ( m_sheenColorFactor == extension->m_sheenColorFactor ) &&
    ( m_sheenColorTexture == extension->m_sheenColorTexture ) &&
    ( m_sheenRoughnessFactor == extension->m_sheenRoughnessFactor ) &&
    ( m_sheenRoughnessTexture == extension->m_sheenRoughnessTexture );
}

std::unique_ptr<Microsoft::glTF::Extension> Deserialize_KHR_materials_sheen( const std::string& json, const Microsoft::glTF::ExtensionDeserializer& extensionDeserializer )
{
  try {
    auto extension = std::make_unique<KHR_materials_sheen>();

    const auto document = nlohmann::json::parse( json );
    if ( auto it = document.find( "sheenColorFactor" ); ( document.end() != it ) && it->is_array() && ( 3 == it->size() ) ) {
      if ( it->at( 0 ).is_number() && it->at( 1 ).is_number() && it->at( 2 ).is_number() ) {
        const float r = it->at( 0 );
        const float g = it->at( 1 );
        const float b = it->at( 2 );
        extension->m_sheenColorFactor = { r, g, b };
      }
    }
    if ( auto it = document.find( "sheenRoughnessFactor" ); ( document.end() != it ) && it->is_number() ) {
      extension->m_sheenRoughnessFactor = *it;
    }
    if ( auto it = document.find( "sheenColorTexture" ); ( document.end() != it ) && it->is_object() ) {
      extension->m_sheenColorTexture = std::make_optional<Microsoft::glTF::TextureInfo>();
      if ( auto index = it->find( "index" ); ( it->end() != index ) && index->is_number_unsigned() ) {
        const uint32_t texIndex = *index;
        extension->m_sheenColorTexture->textureId = std::to_string( texIndex );
      }
      if ( auto texCoord = it->find( "texCoord" ); ( it->end() != texCoord ) && texCoord->is_number_unsigned() ) {
        extension->m_sheenColorTexture->texCoord = *texCoord;
      }
    }
    if ( auto it = document.find( "sheenRoughnessTexture" ); ( document.end() != it ) && it->is_object() ) {
      extension->m_sheenRoughnessTexture = std::make_optional<Microsoft::glTF::TextureInfo>();
      if ( auto index = it->find( "index" ); ( it->end() != index ) && index->is_number_unsigned() ) {
        const uint32_t texIndex = *index;
        extension->m_sheenRoughnessTexture->textureId = std::to_string( texIndex );
      }
      if ( auto texCoord = it->find( "texCoord" ); ( it->end() != texCoord ) && texCoord->is_number_unsigned() ) {
        extension->m_sheenRoughnessTexture->texCoord = *texCoord;
      }
    }
    return extension;
  } catch ( const nlohmann::json::exception& ) {
    return nullptr;
  }
}

} // namespace gltfviewer
