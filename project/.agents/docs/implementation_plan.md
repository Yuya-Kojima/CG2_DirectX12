# 作業計画書 Phase 1：RailCamera バンク（ロール）実装

## 目的
レールのカーブ時にカメラが自然に横傾き（バンク）するようにする。
パンツァードラグーンの「飛んでいる感覚」の再現が目標。

---

## 対象ファイル

| ファイル | 変更内容 |
|---|---|
| `Application/Camera/RailCamera.h` | `bankStrength_` メンバ変数の追加、`CalcTangent()` の宣言追加 |
| `Application/Camera/RailCamera.cpp` | `CalcTangent()` の実装、`Update()` のカメラ行列計算部分の改修 |

---

## 現状の把握（変更前）

`RailCamera::Update()` の現在の実装（抜粋）：
```cpp
// 進行方向からY軸の回転(ヨー)とX軸の回転(ピッチ)を求める
transform_.rotate.y = std::atan2(forward.x, forward.z);
float xzLen = std::sqrt(forward.x * forward.x + forward.z * forward.z);
transform_.rotate.x = std::atan2(-forward.y, xzLen) + 0.1f;

// カメラ行列の更新（Euler角で構築）
worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
```

**問題点：**
- `transform_.rotate.z`（ロール）が常に 0
- カメラの"上方向"が常にワールドY軸に固定されている
- Euler角でカメラ行列を作るとバンクを綺麗に表現しにくい

---

## 実装内容

### Step 1 : `RailCamera.h` にメンバ変数・メソッドを追加

```cpp
// bankStrength_ を追加（既存メンバ変数の近くに追記）
float bankStrength_ = 3.0f; // バンクの強さ（0で無効、大きいほど傾く）

// CalcTangent() の宣言を追加（CalcPosition() の宣言の近くに追記）
Vector3 CalcTangent(float t) const;
```

---

### Step 2 : `RailCamera.cpp` に `CalcTangent()` を実装

`CalcPosition()` 関数の直後に追記する：
```cpp
Vector3 RailCamera::CalcTangent(float t) const {
    float delta = 0.01f;
    float maxT = static_cast<float>(waypoints_.size() - 1);
    Vector3 p0 = CalcPosition(std::max(t - delta, 0.0f));
    Vector3 p1 = CalcPosition(std::min(t + delta, maxT));
    Vector3 tangent = { p1.x - p0.x, p1.y - p0.y, p1.z - p0.z };
    return SafeNormalize(tangent);
}
```

---

### Step 3 : `Update()` のカメラ行列計算を改修

**変更前（Euler角でバンクなし）：**
```cpp
transform_.rotate.y = std::atan2(forward.x, forward.z);
float xzLen = std::sqrt(forward.x * forward.x + forward.z * forward.z);
transform_.rotate.x = std::atan2(-forward.y, xzLen) + 0.1f;

worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
viewMatrix_ = Inverse(worldMatrix_);
```

**変更後（基底ベクトルからカメラ行列を直接構築）：**
```cpp
// --- バンク計算 ---
// 少し先の接線と現在の接線の差から旋回率を求める
float maxT = static_cast<float>(waypoints_.size() - 1);
Vector3 tangentNow  = CalcTangent(t_);
Vector3 tangentNext = CalcTangent(std::min(t_ + 0.05f, maxT));

Vector3 worldUp = { 0.0f, 1.0f, 0.0f };

// 右方向ベクトル（forward × worldUp）
Vector3 right = SafeNormalize(Cross(tangentNow, worldUp));

// 旋回による横方向の変化量 → バンク角
float bankAmount = Dot(tangentNext - tangentNow, right) * bankStrength_;
// バンクを加えたup方向（右に旋回 → 左に傾く）
Vector3 up = SafeNormalize(Cross(right, tangentNow));
// upをbankAmountぶんrightに向けて傾ける
up = SafeNormalize({
    up.x - right.x * bankAmount,
    up.y - right.y * bankAmount,
    up.z - right.z * bankAmount
});

// --- カメラ行列を基底ベクトルから直接構築 ---
// View行列（カメラの逆行列）を直接作る
Vector3 pos = transform_.translate;
viewMatrix_ = MakeLookAtMatrix(pos, pos + tangentNow, up);
projectionMatrix_ = MakePerspectiveFovMatrix(fov_, aspectRatio_, nearClip_, farClip_);
viewProjectionMatrix_ = Multiply(viewMatrix_, projectionMatrix_);

// worldMatrix_ も整合性のために更新（デバッグ用途等に使用）
worldMatrix_ = Inverse(viewMatrix_);
```

> **注意：** `MakeLookAtMatrix` が存在しない場合は `MathUtil` に追加するか、
> 基底ベクトルから手動でビュー行列を組み立てる。

---

### `MakeLookAtMatrix` が存在しない場合の実装

`MathUtil.h/.cpp` に追加：
```cpp
// pos: カメラ位置, target: 注視点, up: 上方向
Matrix4x4 MakeLookAtMatrix(const Vector3& pos, const Vector3& target, const Vector3& up) {
    Vector3 f = SafeNormalize({ target.x - pos.x, target.y - pos.y, target.z - pos.z }); // forward
    Vector3 r = SafeNormalize(Cross(up, f)); // right
    Vector3 u = Cross(f, r);                 // up（再計算）

    Matrix4x4 m = MakeIdentity4x4();
    // Row-major（右手系・DX12）の場合
    m.m[0][0] = r.x; m.m[0][1] = u.x; m.m[0][2] = f.x; m.m[0][3] = 0;
    m.m[1][0] = r.y; m.m[1][1] = u.y; m.m[1][2] = f.y; m.m[1][3] = 0;
    m.m[2][0] = r.z; m.m[2][1] = u.z; m.m[2][2] = f.z; m.m[2][3] = 0;
    m.m[3][0] = -Dot(r, pos); m.m[3][1] = -Dot(u, pos); m.m[3][2] = -Dot(f, pos); m.m[3][3] = 1;
    return m;
}
```
> **注意：** エンジンの行列レイアウト（Row-major / Column-major）や座標系（左手系/右手系）に合わせて調整が必要。

---

## 調整パラメータ

| パラメータ | 初期値 | 説明 |
|---|---|---|
| `bankStrength_` | `3.0f` | バンクの強さ。大きいほど激しく傾く。0で無効化。 |

ImGuiのエディタからリアルタイムで調整できるようにスライダーを追加するのが望ましい。

---

## 検証方法

1. ビルドして実行する
2. レールカメラが右旋回する区間で、画面が右に傾く（左方向の地平線が上がる）ことを確認する
3. `bankStrength_` を 0 にすると現状と同じ動作になることを確認する
4. 直線区間では傾きがほぼ 0 になることを確認する

---

## 完了条件
- [ ] `CalcTangent()` の実装
- [ ] `RailCamera::Update()` のビュー行列計算をバンク対応に改修
- [ ] `bankStrength_` でバンクの強弱を調整できる
- [ ] 既存のカメラシェイク機能が引き続き動作する
- [ ] エディタから `bankStrength_` をリアルタイム調整できる（任意）
