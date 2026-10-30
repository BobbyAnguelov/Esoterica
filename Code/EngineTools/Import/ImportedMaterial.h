#pragma once
#include "ImportedData.h"
#include "Base/Types/Color.h"
#include "Base/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    struct EE_ENGINETOOLS_API Material
    {
    public:

        struct Property
        {
        public:

            inline bool IsSet() const { return m_isValueSet || HasTexture(); }
            inline bool HasTexture() const { return m_resolvedTexturePath.IsValid(); }
            inline bool HasResolvedTexture() const { return m_rawTexturePathStr.empty() || m_resolvedTexturePath.IsValid(); }

        public:

            Color                           m_value = 0xFFFFFFFF;
            bool                            m_isValueSet = false;
            String                          m_rawTexturePathStr;
            FileSystem::Path                m_resolvedTexturePath;
        };

    public:

        StringID                            m_ID;

        Property                            m_albedo;
        Property                            m_normal;
        Property                            m_metalness;
        Property                            m_roughness;
        Property                            m_occlusion;
        Property                            m_displacement;
        Property                            m_emissive;

        float                               m_emissiveStrength = 1.0f;
        float                               m_opacity = 1.0f;

        bool                                m_hasAlpha = false;
        bool                                m_isTwoSided = false;

        // Only metallic/roughness workflow is supported, will print a log warning and use default matte texture otherwise
        bool                                m_isSpecularGlossiness = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API Materials : public ImportedData
    {
    public:

        virtual bool IsValid() const override { return true; }

        int32_t GetNumMaterials() const { return (int32_t) m_data.size(); }

        // Resolve all raw texture paths and perform any necessary finalization steps
        void Finalize( FileSystem::Path const& sourceDataDirectory, FileSystem::Path const& overrideSearchPathDirectory = FileSystem::Path() );

        inline void SetSourcePath( FileSystem::Path const& path ) { m_sourcePath = path; }

        //-------------------------------------------------------------------------

        template <typename... Args>
        decltype( auto ) emplace_back( Args&&... args )
        {
            return m_data.emplace_back( std::forward<Args>( args )... );
        }

        EE_FORCE_INLINE size_t size() const { return m_data.size(); }
        EE_FORCE_INLINE TVector<Material>::iterator begin() { return m_data.begin(); }
        EE_FORCE_INLINE TVector<Material>::iterator end() { return m_data.end(); }
        EE_FORCE_INLINE TVector<Material>::const_iterator begin() const { return m_data.begin(); }
        EE_FORCE_INLINE TVector<Material>::const_iterator end() const { return m_data.end(); }
        EE_FORCE_INLINE Material const& operator[]( size_t idx ) const { return m_data[idx]; }
        EE_FORCE_INLINE Material& operator[]( size_t idx ) { return m_data[idx]; }

    public:

        TVector<Material>                   m_data;
    };
}