#pragma once

#include <GLTFSDK/ExtensionHandlers.h>

#include <optional>

namespace gltfviewer {

struct KHR_materials_clearcoat : Microsoft::glTF::Extension, Microsoft::glTF::glTFProperty
{
    static constexpr const char* Name = "KHR_materials_clearcoat";

    std::optional<float> m_clearcoatFactor;
    std::optional<Microsoft::glTF::TextureInfo> m_clearcoatTexture;
    std::optional<float> m_clearcoatRoughnessFactor;
    std::optional<Microsoft::glTF::TextureInfo> m_clearcoatRoughnessTexture;

    std::unique_ptr<Extension> Clone() const override;
    bool IsEqual( const Extension& rhs ) const override;
};

std::unique_ptr<Microsoft::glTF::Extension> Deserialize_KHR_materials_clearcoat( const std::string& json, const Microsoft::glTF::ExtensionDeserializer& extensionDeserializer );

} // namespace gltfviewer
