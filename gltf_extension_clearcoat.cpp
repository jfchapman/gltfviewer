#include "gltf_extension_clearcoat.h"

#include <nlohmann/json.hpp>

namespace gltfviewer {

std::unique_ptr<Microsoft::glTF::Extension> KHR_materials_clearcoat::Clone() const
{
    return std::make_unique<KHR_materials_clearcoat>( *this );
}

bool KHR_materials_clearcoat::IsEqual( const Microsoft::glTF::Extension& rhs ) const
{
  const KHR_materials_clearcoat* extension = dynamic_cast<const KHR_materials_clearcoat*>( &rhs );
  return 
    ( nullptr != extension ) &&
    ( m_clearcoatFactor == extension->m_clearcoatFactor ) &&
    ( m_clearcoatTexture == extension->m_clearcoatTexture ) &&
    ( m_clearcoatRoughnessFactor == extension->m_clearcoatRoughnessFactor ) &&
    ( m_clearcoatRoughnessTexture == extension->m_clearcoatRoughnessTexture );
}

std::unique_ptr<Microsoft::glTF::Extension> Deserialize_KHR_materials_clearcoat( const std::string& json, const Microsoft::glTF::ExtensionDeserializer& extensionDeserializer )
{
  try {
    auto extension = std::make_unique<KHR_materials_clearcoat>();

    const auto document = nlohmann::json::parse( json );
    if ( auto it = document.find( "clearcoatFactor" ); ( document.end() != it ) && it->is_number() ) {
      extension->m_clearcoatFactor = *it;
    }
    if ( auto it = document.find( "clearcoatRoughnessFactor" ); ( document.end() != it ) && it->is_number() ) {
      extension->m_clearcoatRoughnessFactor = *it;
    }
    if ( auto it = document.find( "clearcoatTexture" ); ( document.end() != it ) && it->is_object() ) {
      extension->m_clearcoatTexture = std::make_optional<Microsoft::glTF::TextureInfo>();
      if ( auto index = it->find( "index" ); ( it->end() != index ) && index->is_number_unsigned() ) {
        const uint32_t texIndex = *index;
        extension->m_clearcoatTexture->textureId = std::to_string( texIndex );
      }
      if ( auto texCoord = it->find( "texCoord" ); ( it->end() != texCoord ) && texCoord->is_number_unsigned() ) {
        extension->m_clearcoatTexture->texCoord = *texCoord;
      }
    }
    if ( auto it = document.find( "clearcoatRoughnessTexture" ); ( document.end() != it ) && it->is_object() ) {
      extension->m_clearcoatRoughnessTexture = std::make_optional<Microsoft::glTF::TextureInfo>();
      if ( auto index = it->find( "index" ); ( it->end() != index ) && index->is_number_unsigned() ) {
        const uint32_t texIndex = *index;
        extension->m_clearcoatRoughnessTexture->textureId = std::to_string( texIndex );
      }
      if ( auto texCoord = it->find( "texCoord" ); ( it->end() != texCoord ) && texCoord->is_number_unsigned() ) {
        extension->m_clearcoatRoughnessTexture->texCoord = *texCoord;
      }
    }
    return extension;
  } catch ( const nlohmann::json::exception& ) {
    return nullptr;
  }
}

} // namespace gltfviewer
