#pragma once

#include <GLTFSDK/ExtensionHandlers.h>

#include <optional>

namespace gltfviewer {

struct KHR_materials_sheen : Microsoft::glTF::Extension, Microsoft::glTF::glTFProperty
{
    static constexpr const char* Name = "KHR_materials_sheen";

    Microsoft::glTF::Color3 m_sheenColorFactor = { 0.0f, 0.0f, 0.0f };
    std::optional<Microsoft::glTF::TextureInfo> m_sheenColorTexture;

    float m_sheenRoughnessFactor = 0.0f;
    std::optional<Microsoft::glTF::TextureInfo> m_sheenRoughnessTexture;

    std::unique_ptr<Extension> Clone() const override;
    bool IsEqual( const Extension& rhs ) const override;
};

std::unique_ptr<Microsoft::glTF::Extension> Deserialize_KHR_materials_sheen( const std::string& json, const Microsoft::glTF::ExtensionDeserializer& extensionDeserializer );

} // namespace gltfviewer
