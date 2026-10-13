//
// Created by Giovanni Bollati on 06/03/26.
//

#ifndef EVOLVING_PLANETS_PHYSICSSYSTEM_HPP
#define EVOLVING_PLANETS_PHYSICSSYSTEM_HPP

#include <btBulletDynamicsCommon.h>
#include <unordered_map>
#include "Engine/ECS/Components.hpp"
#include "Engine/ECS/World.hpp"
#include "Engine/ECS/Systems.hpp"


struct ResetRigidBodies : public ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        auto rigidbodies = world.query<RigidBodyComponent>();
        for (auto rb : rigidbodies)
        {
            auto& rbComponent = world.getComponent<RigidBodyComponent>(rb);
            rbComponent.force = glm::vec3(0.0f);
            rbComponent.torque = glm::vec3(0.0f);
        }
    }
    [[nodiscard]] std::string name() const override { return "Reset Rigidbodies"; }
};

struct PhysicsSystem : public ISystem
{
    std::shared_ptr<GravityAdapter::GravityComputer> gravityComputer;

    // callback: get rigidbodies, and apply the forces
    static void PhysicsPreTickCallback(btDynamicsWorld* btworld, btScalar timeStep) {
        auto world = static_cast<World*>(btworld->getWorldUserInfo());
        if (not world) return;

        auto e_rbs = world->query<RigidBodyComponent>();
        for (auto e_rb: e_rbs)
        {
            auto& rb = world->getComponent<RigidBodyComponent>(e_rb);
            rb.body->applyCentralForce(btVector3(rb.force.x, rb.force.y, rb.force.z));
            rb.body->applyTorque(btVector3(rb.torque.x, rb.torque.y, rb.torque.z));
            rb.body->setAngularVelocity(btVector3(rb.torque.x, rb.torque.y, rb.torque.z));
        }
    }

    PhysicsSystem()
    {
        // INIT BULLET
        collisionConfiguration = new btDefaultCollisionConfiguration();
        dispatcher = new btCollisionDispatcher(collisionConfiguration);
        overlappingPairCache = new btDbvtBroadphase();
        solver = new btSequentialImpulseConstraintSolver();

        dynamicWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);
        dynamicWorld->setGravity(btVector3(0, 0, 0));
        dynamicWorld->setInternalTickCallback(PhysicsPreTickCallback, nullptr, true);
    }

    void update(World& world, const Context& ctx, float dt) override
    {
        // set my world to the dynamic world as world user info
        dynamicWorld->setWorldUserInfo(&world);
        // CLEAN DELETED RIGID BODIES
        for (auto it = activeBodies.begin(); it != activeBodies.end(); ) {
            if (!world.hasEntity(it->first) || !world.hasComponent<RigidBodyComponent>(it->first)) {
                dynamicWorld->removeRigidBody(it->second);
                delete it->second; // Dealloca il rigid body
                it = activeBodies.erase(it); // Rimuove dalla mappa
            } else {
                ++it;
            }
        }
        // GET ENTITIES
        auto entities = world.query<RigidBodyComponent, Transform>();

        // INITIALIZE NEW RIGID BODIES
        for (auto& entity : entities)
        {
            auto& rb = world.getComponent<RigidBodyComponent>(entity);
            auto& transform = world.getComponent<Transform>(entity);

            // initialize rigid body if not already done
            if (!rb.body)
            {
                // set collider
                switch (rb.colliderType) {
                case ColliderType::SPHERE:
                    rb.btCollider = new btSphereShape(rb.radius);
                    break;
                case ColliderType::BOX:
                    rb.btCollider = new btBoxShape(btVector3(rb.boxHalfExtents.x, rb.boxHalfExtents.y, rb.boxHalfExtents.z));
                    break;
                case ColliderType::MESH:
                    {
                        if (not world.hasComponent<MeshComponent>(entity))
                        {
                            throw runtime_error("MeshComponent not found for entity " + std::to_string(entity));
                            std::cout << "trying to build collider before mesh loading" << std::endl;
                        }
                        std::cout << "building collider for entity " << world.getComponent<NameComponent>(entity).name << std::endl;
                        auto mesh = world.getComponent<MeshComponent>(entity).mesh;
                        auto btMesh = new btTriangleMesh();

                        auto triangles = mesh->getTriangles();
                        for (int i = 0; i < mesh->getFacesData().size(); i += 3)
                        {
                            auto t1 = triangles[i];
                            auto t2 = triangles[i + 1];
                            auto t3 = triangles[i + 2];
                            btMesh->addTriangle(btVector3(t1.x, t1.y, t1.z), btVector3(t2.x, t2.y, t2.z), btVector3(t3.x, t3.y, t3.z));
                        }
                        rb.btCollider = new btConvexTriangleMeshShape(btMesh, true);
                        //delete btMesh;
                        break;
                    }
                case ColliderType::BVH:
                    {
                        if (not world.hasComponent<MeshComponent>(entity))
                        {
                            throw runtime_error("MeshComponent not found for entity " + std::to_string(entity));
                            std::cout << "trying to build collider before mesh loading" << std::endl;
                        }
                        std::cout << "building collider for entity " << world.getComponent<NameComponent>(entity).name << std::endl;
                        auto mesh = world.getComponent<MeshComponent>(entity).mesh;
                        auto btMesh = new btTriangleMesh();

                        auto triangles = mesh->getTriangles();
                        for (int i = 0; i < mesh->getFacesData().size(); i += 3)
                        {
                            auto t1 = triangles[i];
                            auto t2 = triangles[i + 1];
                            auto t3 = triangles[i + 2];
                            btMesh->addTriangle(btVector3(t1.x, t1.y, t1.z), btVector3(t2.x, t2.y, t2.z), btVector3(t3.x, t3.y, t3.z));
                        }
                        rb.btCollider = new btBvhTriangleMeshShape(btMesh, true);
                        //delete btMesh;
                        break;
                    }
                case ColliderType::HULL:
                    {
                        if (not world.hasComponent<MeshComponent>(entity))
                        {
                            throw runtime_error("MeshComponent not found for entity " + std::to_string(entity));
                            std::cout << "trying to build collider before mesh loading" << std::endl;
                        }
                        std::cout << "building collider for entity " << world.getComponent<NameComponent>(entity).name << std::endl;
                        auto mesh = world.getComponent<MeshComponent>(entity).mesh;

                        auto vertices = mesh->getVertices();
                        auto convexHullShape = new btConvexHullShape();

                        for (const auto& v : vertices)
                        {
                            convexHullShape->addPoint(btVector3(v.x, v.y, v.z), false);
                        }
                        convexHullShape->optimizeConvexHull();
                        convexHullShape->recalcLocalAabb();
                        rb.btCollider = convexHullShape;
                        break;
                    }
                }
                rb.btCollider->setMargin(0.05f);
                if (rb.mass > 0.0f) {
                    rb.btCollider->calculateLocalInertia(rb.mass, rb.localIntertia);
                }
                // set motion state
                rb.motionState = new btDefaultMotionState(btTransform(
                    btQuaternion(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w),
                    btVector3(transform.position.x, transform.position.y, transform.position.z)
                ));
                // set rigidbody
                auto rigidBodyInfo = btRigidBody::btRigidBodyConstructionInfo(rb.mass, rb.motionState, rb.btCollider, rb.localIntertia);
                rb.body = new btRigidBody(rigidBodyInfo);
                rb.body->setRestitution(rb.bounciness);
                rb.body->setFriction(rb.friction);

                if (rb.isKinematic)
                {
                    rb.body->setCollisionFlags(rb.body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
                    rb.body->setActivationState(DISABLE_DEACTIVATION);
                }

                dynamicWorld->addRigidBody(rb.body);
                activeBodies[entity] = rb.body;
            }
        }

        // UPDATE DIRTIES
        for (auto& entity : entities)
        {
            auto& rb = world.getComponent<RigidBodyComponent>(entity);
            auto& tf = world.getComponent<Transform>(entity);

            // update active
            if (rb.dirty)
            {
                if (rb.active) {
                rb.body->setCollisionFlags(rb.body->getCollisionFlags() & ~btCollisionObject::CF_NO_CONTACT_RESPONSE);
                rb.body->forceActivationState(ACTIVE_TAG);
                rb.body->activate(true);
                }
                else
                {
                    rb.body->setCollisionFlags(rb.body->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
                    rb.body->forceActivationState(DISABLE_SIMULATION);
                    rb.body->setLinearVelocity({0.0f, 0.0f, 0.0f});
                    rb.body->setAngularVelocity({0.0f, 0.0f, 0.0f});
                    rb.body->clearForces();
                }
            }
        }

        // TELEPORT AND UPDATE KINEMATICS
        for (auto& entity : entities) {
            auto& rb = world.getComponent<RigidBodyComponent>(entity);
            auto& tf = world.getComponent<Transform>(entity);

            if (rb.teleportRequested)
            {
                btTransform target;
                target.setOrigin(btVector3(rb.teleportPos.x, rb.teleportPos.y, rb.teleportPos.z));
                target.setRotation(btQuaternion(rb.teleportRot.x, rb.teleportRot.y, rb.teleportRot.z, rb.teleportRot.w));

                // 1. Sposta il corpo fisico
                rb.body->setWorldTransform(target);

                // 2. Fondamentale: aggiorna anche il MotionState
                rb.motionState->setWorldTransform(target);

                // 3. Azzera le forze residue (altrimenti l'oggetto "esplode" via)
                rb.body->setLinearVelocity(btVector3(0, 0, 0));
                rb.body->setAngularVelocity(btVector3(0, 0, 0));
                rb.body->clearForces();
                rb.body->activate(true);

                rb.teleportRequested = false;

                // Aggiorna subito anche il transform grafico per evitare lag di 1 frame
                tf.position = rb.teleportPos;
                tf.rotation = rb.teleportRot;
            }

            if (rb.isKinematic) {
                // SINCRONIZZA IL KINEMATICO: Transform -> Bullet
                btTransform target;
                target.setOrigin(btVector3(tf.position.x, tf.position.y, tf.position.z));
                target.setRotation(btQuaternion(tf.rotation.x, tf.rotation.y, tf.rotation.z, tf.rotation.w));

                // Per i kinematici si aggiorna il MotionState per calcolare le velocità di attrito
                rb.motionState->setWorldTransform(target);
            }
        }

        // update dynamic world
        if (dt > 0.0f) {
            dynamicWorld->stepSimulation(dt, 10, 1.0f / 60.0f);
        }

        for (auto& entity : entities) {
            auto& rb = world.getComponent<RigidBodyComponent>(entity);
            auto& tf = world.getComponent<Transform>(entity);

            // update transform from rigid body
            if (not rb.isKinematic and rb.mass > 0.0f)
            {
                btTransform btTrans;
                rb.body->getMotionState()->getWorldTransform(btTrans);
                tf.position = glm::vec3(btTrans.getOrigin().getX(), btTrans.getOrigin().getY(), btTrans.getOrigin().getZ());
                auto btRot = btTrans.getRotation();
                tf.rotation = glm::quat(btRot.getW(), btRot.getX(), btRot.getY(), btRot.getZ());
            }
        }

        // SET GRAVITY
        auto gravitySettings = world.query<GravitySettingsComponent>();
        if (gravitySettings.empty()) return;
        if (world.getComponent<GravitySettingsComponent>(gravitySettings[0]).enabled) {
            auto planets = world.query<PlanetConfigComponent>();
            if (planets.empty()) return;

            auto players = world.query<PlayerComponent>();
            if (players.empty()) return;

            auto player = players[0];

            if (not gravityComputer)
            {
                auto planet = planets[0];
                auto planetMesh = world.getComponent<MeshComponent>(planet).mesh;
                gravityComputer = std::make_shared<GravityAdapter::GravityComputer>(*planetMesh);
            }

            auto gravity = gravityComputer->getGravityGPU(world.getComponent<Transform>(player).position);
            gravity /= 800.0f;
            std::cout << "Gravity: " << glm::length(gravity) << std::endl;
            dynamicWorld->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
        }
        else
        {
            dynamicWorld->setGravity(btVector3(0, 0, 0));
        }
    }
    [[nodiscard]] std::string name() const override { return "PhysicsSystem"; }

    btCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btBroadphaseInterface* overlappingPairCache;
    btConstraintSolver* solver;
    btDynamicsWorld* dynamicWorld = nullptr;
    std::unordered_map<uint64_t, btRigidBody*> activeBodies;
};

#endif //EVOLVING_PLANETS_PHYSICSSYSTEM_HPP