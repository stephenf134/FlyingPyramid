#ifndef MYANIMATION_OBJECTMANAGER_HPP
#define MYANIMATION_OBJECTMANAGER_HPP

#include <array>
#include <cstdint>
#include <vector>
#include <cmath>
#include <bgfx/bgfx.h>
#include <bx/math.h>

namespace MyAnimation {
    struct Vertex
    {
        float x;
        float y;
        float z;

        uint32_t abgr;

        Vertex();
        Vertex(bx::Vec3 location, std::uint32_t abgr);
    };
    class SpatialObject {
        bgfx::VertexBufferHandle vtxBufHandle_ = BGFX_INVALID_HANDLE;
        bgfx::IndexBufferHandle indBufHandle_ = BGFX_INVALID_HANDLE;
        std::vector<Vertex> vtxVec_;
        std::vector<std::uint16_t> indVec_;
    public:
        SpatialObject();
        ~SpatialObject();
        SpatialObject(const SpatialObject&) = delete;
        SpatialObject& operator=(const SpatialObject&) = delete;
        SpatialObject(SpatialObject&& other) noexcept;
        SpatialObject& operator=(SpatialObject&& other) noexcept;
        static SpatialObject MakeSquarePyramid(bgfx::VertexLayout layout, bx::Vec3 origin, std::array<std::uint32_t, 5> colors, float width, float height);
        void useVtxBuf();
        void useIndBuf();
        void useBufObjs();
        void bgfxDestroyBufObjs() noexcept;
    };
    class ObjectManager {
        std::vector<SpatialObject> objectVec_;
        bgfx::VertexLayout vertexLayout_;
    public:
        ObjectManager();
        ~ObjectManager();

        void bgfxDestroyBufObjs(std::size_t index);

        void addSquarePyramid(bx::Vec3 origin, std::array<std::uint32_t, 5> colors, float width, float height);
        void bgfxUseBufObjs(std::size_t index);
    };
}

#endif //MYANIMATION_OBJECTMANAGER_HPP
