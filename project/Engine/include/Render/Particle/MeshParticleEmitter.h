#pragma once
#include "Particle/IParticleEmitter.h"
#include "Math/VertexData.h"
#include <vector>

class MeshParticleEmitter : public IParticleEmitter {
public:
    MeshParticleEmitter() = default;
    ~MeshParticleEmitter() override = default;

    /// <summary>
    /// 平面(Plane)モデルとして初期化します
    /// </summary>
    /// <param name="textureFilePath">テクスチャのファイルパス</param>
    void InitializeAsPlane(const std::string& textureFilePath);
    
    /// <summary>
    /// ドーナツ型(Ring)モデルとして初期化します
    /// </summary>
    /// <param name="textureFilePath">テクスチャのファイルパス</param>
    /// <param name="divide">分割数（頂点の滑らかさ。例: 64）</param>
    /// <param name="outerRadius">外側の半径（例: 1.0f）</param>
    /// <param name="innerRadius">内側の穴の半径（例: 0.2f）</param>
    void InitializeAsRing(const std::string& textureFilePath, 
        uint32_t divide, float outerRadius, float innerRadius);
        
    /// <summary>
    /// 円柱(Cylinder)モデルとして初期化します
    /// </summary>
    /// <param name="textureFilePath">テクスチャのファイルパス</param>
    /// <param name="divide">分割数（頂点の滑らかさ。例: 32）</param>
    /// <param name="topRadius">上面の半径（例: 1.0f）</param>
    /// <param name="bottomRadius">底面の半径（例: 1.0f）</param>
    /// <param name="height">円柱の高さ（例: 3.0f）</param>
    void InitializeAsCylinder(const std::string& textureFilePath, 
        uint32_t divide, float topRadius, float bottomRadius, float height);

    void Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix) override;

private:
    void CreateVertexDataFromPlane();
    void CreateVertexDataFromVertices(const std::vector<VertexData>& vertices);
};
