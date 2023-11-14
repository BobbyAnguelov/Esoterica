#pragma once

#include "EntityComponent.h"
#include "Base/Math/BoundingVolumes.h"
#include "Base/Math/Transform.h"

//-------------------------------------------------------------------------

namespace EE
{
    #if EE_DEVELOPMENT_TOOLS
    namespace EntityModel
    {
        class EntityEditor;
    }
    #endif

    //-------------------------------------------------------------------------

    class EE_ENGINE_API SpatialEntityComponent : public EntityComponent
    {
        EE_ENTITY_COMPONENT( SpatialEntityComponent );

        friend class Entity;
        friend class EntityDebugView;
        friend EntityModel::Serializer;
        friend EntityModel::EntityMapEditor;
        friend EntityModel::EntityCollection;

        #if EE_DEVELOPMENT_TOOLS
        friend EntityModel::EntityEditor;
        #endif

        struct AttachmentSocketTransformResult
        {
            AttachmentSocketTransformResult( Matrix transform ) : m_transform( transform ) {}

            Matrix      m_transform;
            bool        m_wasFound = false;
        };

    public:

        // Spatial data
        //-------------------------------------------------------------------------

        // Are we the root component for this entity? 
        inline bool IsRootComponent() const { return m_pSpatialParent == nullptr || m_pSpatialParent->m_entityID != m_entityID; }

        inline Transform const& GetLocalTransform() const { return m_transform; }
        inline OBB const& GetLocalBounds() const { return m_bounds; }

        inline Transform const& GetWorldTransform() const { return m_worldTransform; }
        inline OBB const& GetWorldBounds() const { return m_worldBounds; }

        // Get world space position
        inline Vector const& GetPosition() const { return m_worldTransform.GetTranslation(); }

        // Get world space orientation
        inline Quaternion const& GetOrientation() const { return m_worldTransform.GetRotation(); }
        
        // Get world space forward vector
        inline Vector GetForwardVector() const { return m_worldTransform.GetForwardVector(); }

        // Get world space up vector
        inline Vector GetUpVector() const { return m_worldTransform.GetUpVector(); }

        // Get world space right vector
        inline Vector GetRightVector() const { return m_worldTransform.GetRightVector(); }

        // Call to update the local transform - this will also update the world transform for this component and all children
        inline void SetLocalTransform( Transform const& newTransform )
        {
            m_transform = newTransform;
            CalculateWorldTransform();
        }

        // Call to update the world transform - this will also updated the local transform for this component and all children's world transforms
        inline void SetWorldTransform( Transform const& newTransform )
        {
            SetWorldTransformDirectly( newTransform );
        }

        // Move the component by the specified delta transform
        inline void MoveByDelta( Transform const& deltaTransform )
        {
            EE_ASSERT( !deltaTransform.HasScale() );
            Transform const newWorldTransform = deltaTransform * GetWorldTransform();
            SetWorldTransform( newWorldTransform );
        }

        // Parenting
        //-------------------------------------------------------------------------

        // Do we have a spatial parent?
        inline bool HasSpatialParent() const { return m_pSpatialParent != nullptr; }
        
        // Do we have any spatial children
        inline bool HasChildren() const { return !m_spatialChildren.empty(); }

        // Get the spatial parent ID
        inline ComponentID GetSpatialParentID() const { EE_ASSERT( HasSpatialParent() ); return m_pSpatialParent->GetID(); }

        // Get the world transform of our spatial parent
        inline Transform const& GetSpatialParentWorldTransform() const { EE_ASSERT( HasSpatialParent() ); return m_pSpatialParent->GetWorldTransform(); }

        // How deep in the spatial hierarchy are we?
        int32_t GetSpatialHierarchyDepth( bool limitToCurrentEntity = true ) const;

        // Check if we are a spatial child a certain component
        bool IsSpatialChildOf( SpatialEntityComponent const* pPotentialParent ) const;

        // Get the socket ID that we want to be attached to
        inline StringID GetAttachmentSocketID() const { return m_parentAttachmentSocketID; }

        // Set the socket ID that we want to be attached to on the parent
        inline void SetAttachmentSocketID( StringID socketID ) { m_parentAttachmentSocketID = socketID; }

        // Returns the world transform for the specified attachment socket if it exists, if it doesnt this function returns the world transform
        // The search children parameter controls, whether to only search this component or to also search it's children
        Transform GetAttachmentSocketTransform( StringID socketID ) const;

        // Apply an translation offset to all children, needed in many cases to maintain relative offset when a spatial component's bound change
        void ApplyOffsetToAllChildren( Vector const& offset );

        // Local Scale
        //-------------------------------------------------------------------------

        // Do we support local scaling?
        virtual bool SupportsLocalScale() const { return false; }

        // Get the local scaling multiplier
        virtual Float3 const& GetLocalScale() const { return Float3::One; }

        // Conversion Functions
        //-------------------------------------------------------------------------

        // Convert a world transform to a component local transform
        inline Transform ConvertWorldTransformToLocalTransform( Transform const& worldTransform ) const { return Transform::Delta( m_worldTransform, worldTransform ); }

        // Convert a world point to a component local point 
        inline Vector ConvertWorldPointToLocalPoint( Vector const& worldPoint ) const { return m_worldTransform.GetInverse().TransformPoint( worldPoint ); }

        // Convert a world direction to a component local direction 
        inline Vector ConvertWorldVectorToLocalVector( Vector const& worldVector ) const { return m_worldTransform.GetInverse().RotateVector( worldVector ); }

    protected:

        using EntityComponent::EntityComponent;

        virtual void Initialize() override;

        // This function should be called whenever the local scale is changed - this is left entirely up to the end user as we will have very few spatial components that support this
        virtual void OnLocalScaleChanged( Float3 const& newLocalScale ) {}

        // Calculate and return the local bounds for this component - This should not be called directly!
        virtual OBB CalculateLocalBounds() const 
        {
            EE_DEVELOPMENT_TOOLS_ONLY( EE_ASSERT( m_boundsValidationGuard ) );
            return OBB( Vector::Zero, Vector::Half );
        }

        // Updates the local and world bounds for this component
        void UpdateBounds()
        {
            EE_DEVELOPMENT_TOOLS_ONLY( m_boundsValidationGuard = true );
            m_bounds = CalculateLocalBounds();
            m_worldBounds = m_bounds.GetTransformed( m_worldTransform );
        }

        // Try to find and return the world space transform for the specified socket
        bool TryGetAttachmentSocketTransform( StringID socketID, Transform& outSocketWorldTransform ) const;

        // This should be implemented on each derived spatial component to find the transform of the socket if it exists
        // This function must always return a valid world transform in the outSocketTransform usually that of the component
        virtual bool TryFindAttachmentSocketTransform( StringID socketID, Transform& outSocketWorldTransform ) const;

        // This function should be implemented on any component that supports sockets, it will check if the specified socket exists on the component
        virtual bool HasSocket( StringID socketID ) const { return false; }

        // This function should be called whenever a socket is created/destroyed or its position is updated
        void NotifySocketsUpdated();

        // Called whenever the world transform is updated, try to avoid doing anything expensive in this function
        virtual void OnWorldTransformUpdated() {}

        // This function allows you to directly set the world transform for a component and skip the "OnWorldTransformUpdated" callback.
        // This must be used with care and so not be exposed externally.
        inline void SetWorldTransformDirectly( Transform newWorldTransform, bool triggerCallback = true )
        {
            // Only update the transform if we have a parent, if we dont have a parent it means we are the root transform
            if ( m_pSpatialParent != nullptr )
            {
                auto parentWorldTransform = m_pSpatialParent->GetAttachmentSocketTransform( m_parentAttachmentSocketID );
                m_worldTransform = newWorldTransform;
                m_transform = Transform::Delta( parentWorldTransform, m_worldTransform );
            }
            else
            {
                m_worldTransform = newWorldTransform;
                m_transform = newWorldTransform;
            }

            // Calculate world bounds
            m_worldBounds = m_bounds.GetTransformed( m_worldTransform );

            // Propagate the world transforms on the children - children will always have their callbacks fired!
            for ( auto pChild : m_spatialChildren )
            {
                pChild->CalculateWorldTransform();
            }

            // Should we fire the transform updated callback?
            if ( triggerCallback )
            {
                OnWorldTransformUpdated();
            }
        }

        #if EE_DEVELOPMENT_TOOLS
        virtual void PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited ) override;
        #endif

    private:

        // Called whenever the local transform is modified
        inline void CalculateWorldTransform( bool triggerCallback = true )
        {
            // Only update the transform if we have a parent, if we dont have a parent it means we are the root transform
            if ( m_pSpatialParent != nullptr )
            {
                auto parentWorldTransform = m_pSpatialParent->GetAttachmentSocketTransform( m_parentAttachmentSocketID );
                m_worldTransform = m_transform * parentWorldTransform;
            }
            else
            {
                m_worldTransform = m_transform;
            }

            // Calculate world bounds
            m_worldBounds = m_bounds.GetTransformed( m_worldTransform );

            // Propagate the world transforms on the children
            for ( auto pChild : m_spatialChildren )
            {
                pChild->CalculateWorldTransform( triggerCallback );
            }

            if ( triggerCallback )
            {
                OnWorldTransformUpdated();
            }
        }

    private:

        EE_REFLECT() Transform                                                 m_transform;                            // Local space transform
        OBB                                                                 m_bounds;                               // Local space bounding box
        Transform                                                           m_worldTransform;                       // World space transform (left uninitialized to catch initialization errors)
        OBB                                                                 m_worldBounds;                          // World space bounding box

        //-------------------------------------------------------------------------

        SpatialEntityComponent*                                             m_pSpatialParent = nullptr;             // The component we are attached to (spatial hierarchy is managed by the parent entity!)
        EE_REFLECT() StringID                                                  m_parentAttachmentSocketID;             // The socket we are attached to (can be invalid)
        TInlineVector<SpatialEntityComponent*, 2>                           m_spatialChildren;                      // All components that are attached to us. DO NOT EXPOSE THIS!!!

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        bool                                                                m_boundsValidationGuard = false;
        #endif
    };
}