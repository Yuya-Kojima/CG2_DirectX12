# CG5 ポストエフェクト評価課題

## 評価課題の確認方法

ゲーム実行後、画面左上のImGuiメニュー「CG5 Assessment」から以下のボタンを押すことで、各エフェクトの動作を確認できます。

Take Damage (Test Effect): 被弾演出（Vignetting, Radial Blur, Flash, Shockwave）をテストします。
Instant Death (Test GameOver): ゲームオーバー演出（Gaussian Filter, Dissolve）をテストします。

## 実装したポストエフェクト一覧

### 【必須項目】
*   **Grayscale**: スペースキー長押しのロックオン時の演出として実装

### 【加点項目】
*   **GaussianFilter**: ゲームオーバー時の背景のぼかし演出として実装
*   **DepthBasedOutline**: ゲーム全体のトゥーンレンダリング表現として常時適用
*   **Vignetting**: プレイヤー被弾時のダメージ表現として実装
*   **Radial Blur**: プレイヤー被弾時の衝撃表現として実装
*   **Dissolve**: ゲームオーバー時の画面消滅演出として実装
*   **Random**: シェーダーのシード値（Time）として渡し、ノイズ等のアニメーションに利用

### 【独自実装】
*   **Shockwave**: プレイヤー被弾時に空間が歪む衝撃波エフェクトを実装
