#pragma once
#include "Render/Model/Model.h"
#include "Render/Object3d/Object3d.h"
#include "Render/Model/SkinCluster.h"
#include "Math/MathUtil.h"
#include "Model/Skeleton.h"
#include <memory>
#include <string>

class Object3dRenderer;
class SrvManager;

class SkinnedObject {
public:
    SkinnedObject() = default;
    ~SkinnedObject() = default;

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="object3dRenderer">描画システム</param>
    /// <param name="srvManager">SRVマネージャー</param>
    /// <param name="directoryPath">リソースのディレクトリパス</param>
    /// <param name="filename">ファイル名</param>
    void Initialize(Object3dRenderer* object3dRenderer, SrvManager* srvManager, const std::string& directoryPath, const std::string& filename);

    /// <summary>
    /// 更新
    /// </summary>
    /// <param name="deltaTime">フレームの経過時間（秒）</param>
    void Update(float deltaTime);

    /// <summary>
    /// 描画
    /// </summary>
    void Draw();

public:
    // ゲッター
    Object3d* GetObject3d() const { return object3d_.get(); }
    const Skeleton& GetSkeleton() const { return skeleton_; }
    Model* GetModel() const { return model_.get(); }

private:
    std::unique_ptr<Model> model_ = nullptr;
    std::unique_ptr<Object3d> object3d_ = nullptr;
    
    Animation animation_;
    float animationTime_ = 0.0f;
    
    Skeleton skeleton_;
    SkinCluster skinCluster_;
};
