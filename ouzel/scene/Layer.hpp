// Copyright 2015-2019 Elviss Strazdins. All rights reserved.

#ifndef OUZEL_SCENE_LAYER_HPP
#define OUZEL_SCENE_LAYER_HPP

#include <cstdint>
#include <vector>
#include "scene/Actor.hpp"
#include "math/Vector.hpp"

namespace ouzel
{
    namespace scene
    {
        class Scene;
        class Camera;
        class Light;

        class Layer: public ActorContainer
        {
            friend Scene;
            friend Camera;
            friend Light;
        public:
            using Order = int32_t;

            Layer();
            ~Layer() override;

            virtual void draw();

            void addChild(Actor* actor) override;

            inline auto& getCameras() const noexcept { return cameras; }

            std::pair<Actor*, Vector3F> pickActor(const Vector2F& position, bool renderTargets = false) const;
            std::vector<std::pair<Actor*, Vector3F>> pickActors(const Vector2F& position, bool renderTargets = false) const;
            std::vector<Actor*> pickActors(const std::vector<Vector2F>& edges, bool renderTargets = false) const;

            inline auto getOrder() const noexcept { return order; }
            void setOrder(Order newOrder);

            inline auto getScene() const noexcept { return scene; }
            void removeFromScene();

        protected:
            void addCamera(Camera* camera);
            void removeCamera(Camera* camera);

            void addLight(Light* light);
            void removeLight(Light* light);

            virtual void recalculateProjection();
            void enter() override;

            Scene* scene = nullptr;

            std::vector<Camera*> cameras;
            std::vector<Light*> lights;

            Order order = 0;
        };
    } // namespace scene
} // namespace ouzel

#endif // OUZEL_SCENE_LAYER_HPP
