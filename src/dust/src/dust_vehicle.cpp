#include <dust_vehicle.h>

#include <dust_entity.h>

namespace dust {
    const char * ToString( VehicleChassisType type ) {
        switch ( type ) {
            case VEHICLE_CHASSIS_TYPE_BUGGY:
                return "Buggy";
            case VEHICLE_CHASSIS_TYPE_TRUCK:
                return "Truck";
            case VEHICLE_CHASSIS_TYPE_APC:
                return "APC";
            case VEHICLE_CHASSIS_TYPE_TANK:
                return "Tank";
            case VEHICLE_CHASSIS_TYPE_CRAWLER:
                return "Crawler";
        }

        return "Unknown";
    }

    void DriveVehicle( PhysicsWorld & physicsWorld, Entity & entity, f32 throttle, f32 steer, f32 deltaTime ) {
        if ( entity.type != ENTITY_TYPE_VEHICLE || !entity.rigidBody.IsValid() ) {
            return;
        }

        VehicleData & vehicle = entity.vehicle;
        if ( vehicle.handle.IsValid() == false ) {
            return;
        }

        physicsWorld.SetVehicleInput( vehicle.handle, throttle, steer, 0, 0 );
    }

    void DriveVehicleToward( PhysicsWorld & physicsWorld, Entity & entity, glm::vec3 targetPosition, f32 deltaTime ) {
        // Distance at which throttle starts ramping down instead of driving
        // in at full speed and overshooting/orbiting the target.
        constexpr f32 slowRadius = 15.0f;
        // Full steering lock is used once misaligned by this much or more;
        // scales down smoothly below it so the vehicle doesn't twitch once
        // it's basically already facing the target.
        constexpr f32 fullSteerAngle = 30.0f * 3.14159265f / 180.0f;

        glm::vec3 toTarget = targetPosition - entity.position;
        toTarget.y = 0.0f;
        f32 distance = glm::length( toTarget );
        if ( distance < 0.01f ) {
            return;
        }
        glm::vec3 toTargetDir = toTarget / distance;

        glm::vec3 forward = entity.rotation * glm::vec3( 0.0f, 0.0f, 1.0f );
        forward.y = 0.0f;
        f32 forwardLength = glm::length( forward );
        if ( forwardLength < 0.01f ) {
            return;
        }
        forward /= forwardLength;

        glm::vec3 up = entity.rotation * glm::vec3( 0.0f, 1.0f, 0.0f );

        // Signed angle from `forward` to `toTargetDir` about `up`, matching
        // SetVehicleInput's steer convention (positive = right, i.e. toward
        // cross(forward, up) - see DustCamera's flatRight for the same
        // convention). cross(toTargetDir, forward), not the other order:
        // that's what makes a target to the right produce positive angle.
        f32 angle = glm::atan( glm::dot( up, glm::cross( toTargetDir, forward ) ), glm::dot( forward, toTargetDir ) );

        f32 steer = glm::clamp( angle / fullSteerAngle, -1.0f, 1.0f );
        f32 throttle = glm::clamp( distance / slowRadius, 0.25f, 1.0f );

        DriveVehicle( physicsWorld, entity, throttle, steer, deltaTime );
    }
}
