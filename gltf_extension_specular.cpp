#include "gltf_extension_specular.h"

#include <nlohmann/json.hpp>

namespace gltfviewer {

std::unique_ptr<Microsoft::glTF::Extension> KHR_materials_specular::Clone() const
{
    return std::make_unique<KHR_materials_specular>( *this );
}

bool KHR_materials_specular::IsEqual( const Microsoft::glTF::Extension& rhs ) const
{
  const KHR_materials_specular* extension = dynamic_cast<const KHR_materials_specular*>( &rhs );
  return 
    ( nullptr != extension ) &&
    ( m_specularFactor == extension->m_specularFactor ) &&
    ( m_specularTexture == extension->m_specularTexture ) &&
    ( m_specularColorFactor == extension->m_specularColorFactor ) &&
    ( m_specularColorTexture == extension->m_specularColorTexture );
}

std::unique_ptr<Microsoft::glTF::Extension> Deserialize_KHR_materials_specular( const std::string& json, const Microsoft::glTF::ExtensionDeserializer& extensionDeserializer )
{
  try {
    auto extension = std::make_unique<KHR_materials_specular>();

    const auto document = nlohmann::json::parse( json );
    if ( auto it = document.find( "specularFactor" ); ( document.end() != it ) && it->is_number() ) {
      extension->m_specularFactor = *it;
    }
    if ( auto it = document.find( "specularColorFactor" ); ( document.end() != it ) && it->is_array() && ( 3 == it->size() ) ) {
      if ( it->at( 0 ).is_number() && it->at( 1 ).is_number() && it->at( 2 ).is_number() ) {
        const float r = it->at( 0 );
        const float g = it->at( 1 );
        const float b = it->at( 2 );
        extension->m_specularColorFactor = { r, g, b };
      }
    }
    if ( auto it = document.find( "specularTexture" ); ( document.end() != it ) && it->is_object() ) {
      extension->m_specularTexture = std::make_optional<Microsoft::glTF::TextureInfo>();
      if ( auto index = it->find( "index" ); ( it->end() != index ) && index->is_number_unsigned() ) {
        const uint32_t texIndex = *index;
        extension->m_specularTexture->textureId = std::to_string( texIndex );
      }
      if ( auto texCoord = it->find( "texCoord" ); ( it->end() != texCoord ) && texCoord->is_number_unsigned() ) {
        extension->m_specularTexture->texCoord = *texCoord;
      }
    }
    if ( auto it = document.find( "specularColorTexture" ); ( document.end() != it ) && it->is_object() ) {
      extension->m_specularColorTexture = std::make_optional<Microsoft::glTF::TextureInfo>();
      if ( auto index = it->find( "index" ); ( it->end() != index ) && index->is_number_unsigned() ) {
        const uint32_t texIndex = *index;
        extension->m_specularColorTexture->textureId = std::to_string( texIndex );
      }
      if ( auto texCoord = it->find( "texCoord" ); ( it->end() != texCoord ) && texCoord->is_number_unsigned() ) {
        extension->m_specularColorTexture->texCoord = *texCoord;
      }
    }
    return extension;
  } catch ( const nlohmann::json::exception& ) {
    return nullptr;
  }
}

} // namespace gltfviewer
