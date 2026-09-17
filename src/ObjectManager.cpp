#include "MyAnimation\ObjectManager.hpp"

#include <array>
#include <utility>

MyAnimation::Vertex::Vertex() {};

MyAnimation::Vertex::Vertex(bx::Vec3 location, std::uint32_t abgr) {
    this->x = location.x;
    this->y = location.y;
    this->z = location.z;
    this->abgr = abgr;
}

MyAnimation::SpatialObject::SpatialObject() {
    this->vtxVec_ = {};
};
MyAnimation::SpatialObject::~SpatialObject() {
    bgfxDestroyBufObjs();
}

MyAnimation::SpatialObject::SpatialObject(SpatialObject&& other) noexcept
    : vtxBufHandle_(std::exchange(other.vtxBufHandle_, {bgfx::kInvalidHandle})),
      indBufHandle_(std::exchange(other.indBufHandle_, {bgfx::kInvalidHandle})),
      vtxVec_(std::move(other.vtxVec_)),
      indVec_(std::move(other.indVec_)) {
}

MyAnimation::SpatialObject& MyAnimation::SpatialObject::operator=(SpatialObject&& other) noexcept {
    if (this != &other) {
        bgfxDestroyBufObjs();
        vtxVec_ = std::move(other.vtxVec_);
        indVec_ = std::move(other.indVec_);
        vtxBufHandle_ = std::exchange(other.vtxBufHandle_, {bgfx::kInvalidHandle});
        indBufHandle_ = std::exchange(other.indBufHandle_, {bgfx::kInvalidHandle});
    }
    return *this;
}

MyAnimation::SpatialObject MyAnimation::SpatialObject::MakeSquarePyramid(bgfx::VertexLayout layout, bx::Vec3 origin, std::array<std::uint32_t, 5> colors, float width, float height) {
    auto obj = MyAnimation::SpatialObject();

    // Vertices
    obj.vtxVec_.push_back(MyAnimation::Vertex(origin, colors.at(0)));
    obj.vtxVec_.push_back(MyAnimation::Vertex(bx::Vec3(origin.x + width, origin.y, origin.z), colors.at(1)));
    obj.vtxVec_.push_back(MyAnimation::Vertex(bx::Vec3(origin.x + width, origin.y, origin.z + width), colors.at(2)));
    obj.vtxVec_.push_back(MyAnimation::Vertex(bx::Vec3(origin.x, origin.y, origin.z + width), colors.at(3)));
    obj.vtxVec_.push_back(MyAnimation::Vertex(bx::Vec3(origin.x + width / 2.0f, origin.y + height, origin.z + width / 2.0f), colors.at(4)));

    // Indices
    std::uint16_t to_insert[] = {
        /* First triangle of square base of pyramid */
        0, 1, 2,
        /* second one */
        0, 2, 3,
        /* 4 triangle faces */
        0, 4, 1,
        1, 4, 2,
        0, 3, 4,
        2, 4, 3
    };
    obj.indVec_.insert(obj.indVec_.end(),
        std::begin(to_insert), std::end(to_insert));

    // Buffer Objects
    obj.vtxBufHandle_ = bgfx::createVertexBuffer(
        bgfx::copy(
            obj.vtxVec_.data(), static_cast<std::uint32_t>(sizeof(Vertex) * obj.vtxVec_.size())), layout);
    obj.indBufHandle_ = bgfx::createIndexBuffer(
        bgfx::copy(
            obj.indVec_.data(), static_cast<std::uint32_t>(sizeof(std::uint16_t) * obj.indVec_.size())));

    return obj;
}

void MyAnimation::SpatialObject::useVtxBuf() {
    bgfx::setVertexBuffer(0, this->vtxBufHandle_);
}
void MyAnimation::SpatialObject::useIndBuf() {
    bgfx::setIndexBuffer(this->indBufHandle_);
}
void MyAnimation::SpatialObject::useBufObjs() {
    this->useVtxBuf();
    this->useIndBuf();
}

MyAnimation::ObjectManager::ObjectManager() {
    vertexLayout_ = bgfx::VertexLayout();
    vertexLayout_.begin()
    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
    .end();
}
MyAnimation::ObjectManager::~ObjectManager() {
    this->objectVec_.clear();
    this->vertexLayout_ = {};
}

void MyAnimation::ObjectManager::addSquarePyramid(bx::Vec3 origin, std::array<std::uint32_t, 5> colors, float width, float height) {
    auto newObj = MyAnimation::SpatialObject::MakeSquarePyramid(
        this->vertexLayout_, origin, colors, width, height);
    this->objectVec_.push_back(std::move(newObj));
}

void MyAnimation::SpatialObject::bgfxDestroyBufObjs() noexcept {
    if (bgfx::isValid(vtxBufHandle_)) {
        bgfx::destroy(vtxBufHandle_);
        vtxBufHandle_ = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(indBufHandle_)) {
        bgfx::destroy(indBufHandle_);
        indBufHandle_ = BGFX_INVALID_HANDLE;
    }
}

void MyAnimation::ObjectManager::bgfxDestroyBufObjs(std::size_t index) {
    this->objectVec_.at(index).bgfxDestroyBufObjs();
}

void MyAnimation::ObjectManager::bgfxUseBufObjs(std::size_t index) {
    this->objectVec_.at(index).useBufObjs();
}
