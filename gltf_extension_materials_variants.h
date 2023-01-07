#pragma once

#include <GLTFSDK/ExtensionHandlers.h>

#include <map>

namespace gltfviewer {

struct KHR_materials_variants : Microsoft::glTF::Extension, Microsoft::glTF::glTFProperty
{
    static constexpr const char* Name = "KHR_materials_variants";

    // Material variant names (top level).
    std::vector<std::string> m_variants;

    // Maps a variant index to a material ID (primitive instance level).
    std::map<uint32_t, std::string> m_mappings;

    std::unique_ptr<Extension> Clone() const override;
    bool IsEqual( const Extension& rhs ) const override;
};

std::unique_ptr<Microsoft::glTF::Extension> Deserialize_KHR_materials_variants( const std::string& json, const Microsoft::glTF::ExtensionDeserializer& extensionDeserializer );

} // namespace gltfviewer
