#include "stdafx.h"
#include "Render/Items/ItemPreviewBounds.h"

#include "Render/Items/ItemDisplay.h"
#include "Render/Models/ZzzBMD.h"
#include "Engine/Object/w_ObjectInfo.h"
#include "Camera/CameraProjection.h"
#include "Camera/CameraState.h"
#include "Core/Utilities/Log/MuLogger.h"
#include "Core/Text/Utf8.h"
#include "Render/Textures/ZzzTexture.h"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

extern CameraState g_Camera;
extern float BoneScale;

namespace Render::Items::Preview
{
namespace
{
using Vec3 = std::array<float, 3>;

float Dot(const Vec3& a, const Vec3& b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// A model's skinned vertices at its authored display scale and rest rotation, origin at zero: what
// RenderObjectScreen hands the renderer, minus the placement.
struct ModelGeometry
{
    std::vector<Vec3> points;
    Vec3 displayOffset{};
    int drawnMeshCount = 0;
    int totalMeshCount = 0;
};

// World-space size of one UI logical pixel along the screen's right and down axes, at the depth items
// are actually drawn at (see ItemDepthFraction). Probed from CameraProjection::ScreenToWorldRay itself
// so it is, by construction, in the same units the rest of the UI pass positions things in.
struct ScreenBasis
{
    Vec3 right{};
    Vec3 down{};
    float rightLengthSq = 0.f;
    float downLengthSq = 0.f;
};

// The silhouette's bounds in UI logical pixels, relative to where the model's origin projects, at the
// model's authored display scale (i.e. a RenderItem3DScaled multiplier of 1). offsetX/Y is where the
// authored display.offset moves that origin: world-space, so it does not shrink with the multiplier.
struct ScreenExtents
{
    float minX = 0.f;
    float maxX = 0.f;
    float minY = 0.f;
    float maxY = 0.f;
    float offsetX = 0.f;
    float offsetY = 0.f;
};

struct CachedModel
{
    ModelGeometry geometry;
    ScreenBasis basis;
    ScreenExtents extents;
    bool extentsValid = false;
};

// A one-pixel probe would lose most of its precision to ScreenToWorldRay truncating its input to int.
// RenderObjectScreen passes HideSkin=true to RenderPartObject for every item preview, so skin/hair
// textures baked into armor models (the body part the piece is cut from) are not drawn and must not
// widen the measured silhouette.
constexpr bool PreviewHidesSkin = true;

constexpr int ProbeDeltaPixels = 100;
constexpr float BasisRelativeTolerance = 1e-4f;
constexpr float MinExtentPixels = 1e-3f;

ScreenBasis MeasureScreenBasis(float depthFraction)
{
    vec3_t origin, rightProbe, downProbe;
    CameraProjection::ScreenToWorldRay(g_Camera, 0, 0, origin, false);
    CameraProjection::ScreenToWorldRay(g_Camera, ProbeDeltaPixels, 0, rightProbe, false);
    CameraProjection::ScreenToWorldRay(g_Camera, 0, ProbeDeltaPixels, downProbe, false);

    // ScreenToWorldRay returns the point on the far plane; the item is drawn depthFraction of the way
    // from the camera to it, so one pixel spans depthFraction as much world space there.
    const float perPixel = depthFraction / static_cast<float>(ProbeDeltaPixels);
    ScreenBasis basis;
    for (int axis = 0; axis < 3; ++axis)
    {
        basis.right[axis] = (rightProbe[axis] - origin[axis]) * perPixel;
        basis.down[axis] = (downProbe[axis] - origin[axis]) * perPixel;
    }
    basis.rightLengthSq = Dot(basis.right, basis.right);
    basis.downLengthSq = Dot(basis.down, basis.down);
    return basis;
}

bool IsUsable(const ScreenBasis& basis)
{
    return basis.rightLengthSq > 0.f && basis.downLengthSq > 0.f;
}

bool IsCloseEnough(float a, float b)
{
    return std::fabs(a - b) <= BasisRelativeTolerance * std::max(std::fabs(a), std::fabs(b)) + FLT_EPSILON;
}

bool IsSameBasis(const ScreenBasis& a, const ScreenBasis& b)
{
    for (int axis = 0; axis < 3; ++axis)
    {
        if (!IsCloseEnough(a.right[axis], b.right[axis]) || !IsCloseEnough(a.down[axis], b.down[axis]))
        {
            return false;
        }
    }
    return true;
}

// Poses drawnType's attach skeleton at rest (armor is drawn on the character body - see
// Display::IsDrawnOnCharacterSkeleton) and skins drawnType's own mesh against it: the same two-step
// path RenderObjectScreen/RenderPartObject use. BoneTransform and VertexTransform are the engine's
// shared skinning scratch buffers; this reads the result before anything else touches them.
void SkinPreviewMesh(int drawnType, const Display::InventoryDisplay& display)
{
    vec3_t restAngle;
    Vector(display.rotation[0], display.rotation[1], display.rotation[2], restAngle);
    vec3_t headAngle;
    Vector(0.f, 0.f, 0.f, headAngle);

    const int skeletonType = Display::IsDrawnOnCharacterSkeleton(drawnType) ? MODEL_PLAYER : drawnType;
    BMD& skeleton = Models[skeletonType];
    skeleton.BodyHeight = display.bodyHeight;
    skeleton.CurrentAction = 0;
    skeleton.Animation(BoneTransform, 0.f, 0.f, 0, restAngle, headAngle, false, false);

    BMD& mesh = Models[drawnType];
    mesh.BodyScale = display.scale;
    vec3_t origin;
    Vector(0.f, 0.f, 0.f, origin);
    VectorCopy(origin, mesh.BodyOrigin);

    // RenderPartObject resets BoneScale to 1 for every ordinary (unselected) draw; Transform snapshots
    // the global, so measure under the same value and leave it as found.
    const float previousBoneScale = BoneScale;
    BoneScale = 1.f;
    vec3_t dummyMin, dummyMax;
    Vector(0.f, 0.f, 0.f, dummyMin);
    Vector(0.f, 0.f, 0.f, dummyMax);
    OBB_t dummyObb{};
    mesh.Transform(BoneTransform, dummyMin, dummyMax, &dummyObb, /*Translate=*/true);
    mesh.EnsureCpuVertices();
    BoneScale = previousBoneScale;
}

ModelGeometry GatherGeometry(int drawnType, const Display::InventoryDisplay& display)
{
    SkinPreviewMesh(drawnType, display);

    const BMD& mesh = Models[drawnType];
    ModelGeometry geometry;
    geometry.displayOffset = {display.offset[0], display.offset[1], display.offset[2]};
    geometry.totalMeshCount = mesh.NumMeshs;
    for (int m = 0; m < mesh.NumMeshs; ++m)
    {
        if (!mesh.IsMeshDrawn(m, PreviewHidesSkin))
        {
            continue;
        }

        ++geometry.drawnMeshCount;
        const int vertexCount = mesh.Meshs[m].NumVertices;
        for (int v = 0; v < vertexCount; ++v)
        {
            const float* p = VertexTransform[m][v];
            geometry.points.push_back({p[0], p[1], p[2]});
        }
    }
    return geometry;
}

ScreenExtents ProjectToScreen(const ModelGeometry& geometry, const ScreenBasis& basis)
{
    ScreenExtents extents;
    bool first = true;
    for (const Vec3& point : geometry.points)
    {
        const float x = Dot(point, basis.right) / basis.rightLengthSq;
        const float y = Dot(point, basis.down) / basis.downLengthSq;
        if (first)
        {
            extents.minX = extents.maxX = x;
            extents.minY = extents.maxY = y;
            first = false;
            continue;
        }
        extents.minX = std::min(extents.minX, x);
        extents.maxX = std::max(extents.maxX, x);
        extents.minY = std::min(extents.minY, y);
        extents.maxY = std::max(extents.maxY, y);
    }
    extents.offsetX = Dot(geometry.displayOffset, basis.right) / basis.rightLengthSq;
    extents.offsetY = Dot(geometry.displayOffset, basis.down) / basis.downLengthSq;
    return extents;
}

// The expensive step (a full mesh skin and vertex copy) runs once per distinct drawn model and is
// cached: item models don't change at runtime. Per-call work after that is three ray probes and four
// multiplies (docs/CODING_RULES.md rule 12). An unmeasurable (not yet loaded) model is not cached, so
// it is retried; that retry is cheap because such a model has no meshes to skin.
CachedModel* FindOrBuildModel(int drawnType, const Display::InventoryDisplay& display)
{
    static std::unordered_map<int, CachedModel> cache;

    const auto found = cache.find(drawnType);
    if (found != cache.end())
    {
        return &found->second;
    }

    ModelGeometry geometry = GatherGeometry(drawnType, display);
    if (geometry.points.empty())
    {
        return nullptr;
    }

    CachedModel& entry = cache[drawnType];
    entry.geometry = std::move(geometry);
    return &entry;
}

// The screen axes depend on the camera, which the UI pass can reconfigure (window resize, layout mode),
// so the cached extents are only reused while the measured basis is unchanged.
void RefreshExtents(CachedModel& model, const ScreenBasis& basis)
{
    if (model.extentsValid && IsSameBasis(model.basis, basis))
    {
        return;
    }
    model.basis = basis;
    model.extents = ProjectToScreen(model.geometry, basis);
    model.extentsValid = true;
}

std::optional<Placement> FitExtents(const ScreenExtents& extents, const Display::Anchor& anchor, float slotWidth,
                                    float slotHeight, float fill)
{
    const float extentWidth = extents.maxX - extents.minX;
    const float extentHeight = extents.maxY - extents.minY;

    float scale = FLT_MAX;
    if (extentWidth > MinExtentPixels)
    {
        scale = std::min(scale, slotWidth * fill / extentWidth);
    }
    if (extentHeight > MinExtentPixels)
    {
        scale = std::min(scale, slotHeight * fill / extentHeight);
    }
    if (scale == FLT_MAX)
    {
        return std::nullopt;
    }

    // The silhouette's centre sits at (origin + display offset + scale * silhouette midpoint); the
    // origin is where RenderItem3D puts the slot's authored anchor. Shift the slot so that centre
    // lands on the slot's own centre.
    const float midX = (extents.minX + extents.maxX) * 0.5f;
    const float midY = (extents.minY + extents.maxY) * 0.5f;
    Placement placement;
    placement.scale = scale;
    placement.shiftX = slotWidth * 0.5f - slotWidth * anchor.x - extents.offsetX - scale * midX;
    placement.shiftY = slotHeight * 0.5f - slotHeight * anchor.y - extents.offsetY - scale * midY;
    return placement;
}

// Per-mesh breakdown logged next to the first fit of a model, to explain a preview that shows only part of
// an item: which meshes the renderer draws, their textures and skin/hair flags, and where each sits.
void LogMeshBreakdown(int drawnType)
{
    const BMD& mesh = Models[drawnType];
    const auto logger = mu::log::Get("render");
    for (int m = 0; m < mesh.NumMeshs; ++m)
    {
        const Mesh_t& part = mesh.Meshs[m];
        const int textureIndex = mesh.IndexTexture[part.Texture];
        std::string textureName = "<hidden>";
        bool isSkin = false;
        bool isHair = false;
        if (textureIndex != BITMAP_HIDE)
        {
            if (const auto texture = Bitmaps.GetTexture(textureIndex))
            {
                mu_wchar_to_utf8(texture->FileName, textureName);
                isSkin = texture->IsSkin;
                isHair = texture->IsHair;
            }
        }

        Vec3 low = {FLT_MAX, FLT_MAX, FLT_MAX};
        Vec3 high = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
        for (int v = 0; v < part.NumVertices; ++v)
        {
            for (int axis = 0; axis < 3; ++axis)
            {
                low[axis] = std::min(low[axis], VertexTransform[m][v][axis]);
                high[axis] = std::max(high[axis], VertexTransform[m][v][axis]);
            }
        }
        logger->info("ItemPreview:   model={} mesh={} verts={} tris={} drawn={} skin={} hair={} texture={} "
                     "bounds=[{:.1f},{:.1f}]x[{:.1f},{:.1f}]x[{:.1f},{:.1f}]",
                     drawnType, m, part.NumVertices, part.NumTriangles, mesh.IsMeshDrawn(m, PreviewHidesSkin), isSkin,
                     isHair, textureName, low[0], high[0], low[1], high[1], low[2], high[2]);
    }
}

void LogFirstFit(int drawnType, const Display::InventoryDisplay& display, const CachedModel& model,
                 const Placement& placement)
{
    static std::unordered_set<int> logged;
    if (!logged.insert(drawnType).second)
    {
        return;
    }

    const auto logger = mu::log::Get("render");
    logger->info(
        "ItemPreview: model={} meshes={}/{} dispScale={:.5f} points={} px/world={:.2f}x{:.2f} "
        "extents=[{:.2f},{:.2f}]x[{:.2f},{:.2f}] offsetPx=({:.2f},{:.2f}) => scale={:.4f} shift=({:.2f},{:.2f})",
        drawnType, model.geometry.drawnMeshCount, model.geometry.totalMeshCount, display.scale,
        model.geometry.points.size(), 1.f / std::sqrt(model.basis.rightLengthSq),
        1.f / std::sqrt(model.basis.downLengthSq), model.extents.minX, model.extents.maxX, model.extents.minY,
        model.extents.maxY, model.extents.offsetX, model.extents.offsetY, placement.scale, placement.shiftX,
        placement.shiftY);
    LogMeshBreakdown(drawnType);
    // The logger only flushes on errors; this fires once per model, so flush it to make the numbers readable
    // while the game is still running.
    logger->flush();
}
} // namespace

std::optional<Placement> ComputePlacement(int itemType, int level, float slotWidth, float slotHeight, float fill,
                                          float depthFraction)
{
    const int inventoryModel = Display::GetInventoryModel(itemType);
    const int drawnType = Display::GetDrawnModel(inventoryModel, level);

    const Display::InventoryDisplay display = Display::GetInventoryDisplay(drawnType);
    if (display.scale <= 0.f)
    {
        return std::nullopt;
    }

    CachedModel* model = FindOrBuildModel(drawnType, display);
    if (model == nullptr)
    {
        return std::nullopt;
    }

    const ScreenBasis basis = MeasureScreenBasis(depthFraction);
    if (!IsUsable(basis))
    {
        return std::nullopt;
    }
    RefreshExtents(*model, basis);

    const std::optional<Placement> placement =
        FitExtents(model->extents, Display::GetInventoryAnchor(itemType, level), slotWidth, slotHeight,
                   std::clamp(fill, 0.1f, 1.0f));
    if (placement)
    {
        LogFirstFit(drawnType, display, *model, *placement);
    }
    return placement;
}
} // namespace Render::Items::Preview
