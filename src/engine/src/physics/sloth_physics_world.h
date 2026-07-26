#pragma once

#include "core/sloth_defines.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <memory>

namespace sloth {

    enum class BodyMotionType : u8 {
        Static,    // Never moves; infinite mass.
        Kinematic, // Moved by the caller (SetPosition/SetRotation), not affected by forces.
        Dynamic,   // Fully simulated.
    };

    struct RigidBodyDesc {
        glm::vec3 position { 0.0f, 0.0f, 0.0f };
        glm::quat rotation { 1.0f, 0.0f, 0.0f, 0.0f }; // identity, glm order (w, x, y, z)
        BodyMotionType motionType = BodyMotionType::Dynamic;
        f32 friction = 0.5f;
        f32 restitution = 0.0f;
    };

    struct RigidBody {
        static constexpr u32 InvalidId = 0xFFFFFFFF; // matches JPH::BodyID::cInvalidBodyID

        u32 Id = InvalidId;

        bool IsValid() const { return Id != InvalidId; }
    };

    struct RayCastHit {
        RigidBody body;
        glm::vec3 point { 0.0f, 0.0f, 0.0f };
    };

    struct VehicleHandle {
        static constexpr u32 InvalidId = 0xFFFFFFFF;

        u32 Id = InvalidId;

        bool IsValid() const { return Id != InvalidId; }
    };

    struct CharacterHandle {
        static constexpr u32 InvalidId = 0xFFFFFFFF;

        u32 Id = InvalidId;

        bool IsValid() const { return Id != InvalidId; }
    };


    // Owns a Jolt physics simulation: the PhysicsSystem, job system, and
    // temp allocator. All Jolt types are hidden behind Impl (like Window
    // hides GLFWwindow) so nothing outside this .cpp needs Jolt's headers.
    //
    // Stepping: Update() runs a fixed-timestep accumulator internally, so
    // call it once per frame with the real (variable) frame deltaTime; it
    // will run zero, one, or several 1/60s sub-steps as needed.
    //
    // Readback: body transforms are only valid to read once Update() has
    // returned for that call — never read mid-step, and never call into
    // this class concurrently from multiple threads.
    //
    // Threading: Jolt parallelizes internally across the job system this
    // class owns. Callers don't need to (and shouldn't) manage their own
    // threads for physics.
    class PhysicsWorld {
      public:
        PhysicsWorld();
        ~PhysicsWorld();

        SL_NON_COPYABLE( PhysicsWorld );
        SL_NON_MOVABLE( PhysicsWorld );

        void Update( f32 deltaTime );

        RigidBody       CreateBoxBody( const glm::vec3 & halfExtents, const RigidBodyDesc & desc );
        RigidBody       CreateSphereBody( f32 radius, const RigidBodyDesc & desc );
        void            DestroyBody( RigidBody body );

        VehicleHandle   CreateVehicle( const glm::vec3 & position, const glm::vec3 & halfExtents, f32 wheelRad, f32 wheelWidth );
        void            DestroyVehicle( VehicleHandle vehicle );
        RigidBody       GetVehicleBody( VehicleHandle vehicle ) const;
        void            SetVehicleInput( VehicleHandle vehicle, f32 inForward, f32 inRight, f32 inBrake, f32 inHandBrake );
        glm::mat4       GetVehicleWheelTransform( VehicleHandle vehicle, i32 wheelIndex ) const;

        CharacterHandle CreatePlayerCharacter( const glm::vec3 & position, f32 height, f32 radius );
        void            DestroyPlayerCharacter( CharacterHandle character );
        bool            SetCharacterHeight( CharacterHandle character, f32 height, f32 radius );
        void            SetCharacterLinearVelocity( CharacterHandle character, const glm::vec3 & velocity );
        glm::vec3       GetCharacterLinearVelocity( CharacterHandle character ) const;
        void            SetCharacterPosition( CharacterHandle character, const glm::vec3 & position ); // Teleport - e.g. respawn. Does not refresh contacts; next ExtendedUpdate() re-resolves them.
        glm::vec3       GetCharacterPosition( CharacterHandle character ) const;
        glm::quat       GetCharacterRotation( CharacterHandle character ) const;
        bool            IsCharacterGrounded( CharacterHandle character ) const;

        bool            Raycast( const glm::vec3 & origin, const glm::vec3 & direction, f32 maxDistance, RayCastHit & outHit ) const;

        glm::vec3       GetPosition( RigidBody body ) const;
        glm::quat       GetRotation( RigidBody body ) const;

        void            SetLinearVelocity( RigidBody body, const glm::vec3 & velocity );
        glm::vec3       GetLinearVelocity( RigidBody body ) const;

        void            SetAngularVelocity( RigidBody body, const glm::vec3 & angularVelocity );
        glm::vec3       GetAngularVelocity( RigidBody body ) const;

        void            AddForce( RigidBody body, const glm::vec3 & force );
        void            AddTorque( RigidBody body, const glm::vec3 & torque );

        // Fraction     in [0,1) of a fixed step left over in the accumulator;
        f32             GetInterpolationAlpha() const;

      private:
        struct Impl;
        std::unique_ptr<Impl> impl;
    };

} // namespace sloth
