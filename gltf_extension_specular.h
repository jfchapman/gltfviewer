#pragma once

#include <GLTFSDK/ExtensionHandlers.h>

#include <optional>

namespace gltfviewer {

struct KHR_materials_specular : Microsoft::glTF::Extension, Microsoft::glTF::glTFProperty
{
    static constexpr const char* Name = "KHR_materials_specular";

    float m_specularFactor = 1.0f;
    std::optional<Microsoft::glTF::TextureInfo> m_specularTexture;

    Microsoft::glTF::Color3 m_specularColorFactor = { 1.0f, 1.0f, 1.0f };
    std::optional<Microsoft::glTF::TextureInfo> m_specularColorTexture;

    std::unique_ptr<Extension> Clone() const override;
    bool IsEqual( const Extension& rhs ) const override;
};

std::unique_ptr<Microsoft::glTF::Extension> Deserialize_KHR_materials_specular( const std::string& json, const Microsoft::glTF::ExtensionDeserializer& extensionDeserializer );

} // namespace gltfviewer
