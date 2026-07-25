#pragma once

#include <dust_item.h>

#include <physics/sloth_physics_world.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace dust {
    struct Entity;

    enum VehicleChassisType {
        VEHICLE_CHASSIS_TYPE_BUGGY,
        VEHICLE_CHASSIS_TYPE_TRUCK,
        VEHICLE_CHASSIS_TYPE_APC,
        VEHICLE_CHASSIS_TYPE_TANK,
        VEHICLE_CHASSIS_TYPE_CRAWLER, // Aircraft carrier but on wheels
    };

    struct VehicleChassisDefinition {
        VehicleChassisType chassisType;
        union {
            struct {
                InventoryItemType engineSlot;
                InventoryItemType tireSlot;
                InventoryItemType turretSlot;
                InventoryItemType powerSlot;
                InventoryItemType generalSlot1;
                InventoryItemType generalSlot2;
            } buggy;
            struct {
                InventoryItemType engineSlot;
                InventoryItemType tireSlot;
                InventoryItemType turretSlot;
                InventoryItemType powerSlot;
                InventoryItemType generalSlot1;
                InventoryItemType generalSlot2;
                InventoryItemType generalSlot3;
                InventoryItemType generalSlot4;
            } truck;
        };
    };

    struct VehicleData {
        VehicleHandle               handle;
        glm::vec3                   halfExtents = { 0.9f, 0.35f, 1.7f };
        f32                         wheelRadius = 0.45f;
        f32                         wheelWidth = 0.3f;
        VehicleChassisDefinition    definition;

        // Wheel transforms relative to the chassis body, refreshed each
        // frame from the physics wheel state (suspension/steer/roll) by
        // DustWorld::SyncPhysicsTransforms(). Order matches CreateVehicle's
        // wheel order: left front, right front, left rear, right rear.
        glm::mat4                   wheelLocalTransforms[4];
    };

    void DriveVehicle( PhysicsWorld & physicsWorld, Entity & entity, f32 throttle, f32 steer, f32 deltaTime );
    void DriveVehicleToward( PhysicsWorld & physicsWorld, Entity & entity, glm::vec3 targetPosition, f32 deltaTime );


}
