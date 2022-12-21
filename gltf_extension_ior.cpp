#include "gltf_extension_ior.h"

#include <nlohmann/json.hpp>

namespace gltfviewer {

std::unique_ptr<Microsoft::glTF::Extension> KHR_materials_ior::Clone() const
{
    return std::make_unique<KHR_materials_ior>( *this );
}

bool KHR_materials_ior::IsEqual( const Microsoft::glTF::Extension& rhs ) const
{
  const KHR_materials_ior* extension = dynamic_cast<const KHR_materials_ior*>( &rhs );
  return ( nullptr != extension ) && ( m_ior == extension->m_ior );
}

std::unique_ptr<Microsoft::glTF::Extension> Deserialize_KHR_materials_ior( const std::string& json, const Microsoft::glTF::ExtensionDeserializer& extensionDeserializer )
{
  try {
    auto extension = std::make_unique<KHR_materials_ior>();
    const auto document = nlohmann::json::parse( json );
    if ( auto it = document.find( "ior" ); ( document.end() != it ) && it->is_number() ) {
      extension->m_ior = *it;
    }
    return extension;
  } catch ( const nlohmann::json::exception& ) {
    return nullptr;
  }
}

} // namespace gltfviewer
