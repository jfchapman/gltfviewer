#include "gltf_extension_volume.h"

#include <nlohmann/json.hpp>

namespace gltfviewer {

std::unique_ptr<Microsoft::glTF::Extension> KHR_materials_volume::Clone() const
{
    return std::make_unique<KHR_materials_volume>( *this );
}

bool KHR_materials_volume::IsEqual( const Microsoft::glTF::Extension& rhs ) const
{
  const KHR_materials_volume* extension = dynamic_cast<const KHR_materials_volume*>( &rhs );
  return ( nullptr != extension ) && ( m_attenutationDistance == extension->m_attenutationDistance ) && ( m_attenuationColor == extension->m_attenuationColor );
}

std::unique_ptr<Microsoft::glTF::Extension> Deserialize_KHR_materials_volume( const std::string& json, const Microsoft::glTF::ExtensionDeserializer& extensionDeserializer )
{
  try {
    auto extension = std::make_unique<KHR_materials_volume>();
    const auto document = nlohmann::json::parse( json );
    if ( auto it = document.find( "attenuationDistance" ); ( document.end() != it ) && it->is_number() ) {
      extension->m_attenutationDistance = *it;
    }
    if ( auto it = document.find( "attenuationColor" ); ( document.end() != it ) && it->is_array() && ( 3 == it->size() ) ) {
      if ( it->at( 0 ).is_number() && it->at( 1 ).is_number() && it->at( 2 ).is_number() ) {
        const float r = it->at( 0 );
        const float g = it->at( 1 );
        const float b = it->at( 2 );
        extension->m_attenuationColor = { r, g, b };
      }
    }
    return extension;
  } catch ( const nlohmann::json::exception& ) {
    return nullptr;
  }
}

} // namespace gltfviewer
