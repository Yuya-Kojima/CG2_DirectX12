#pragma once
#include <stdint.h>

// 衝突属性（自分が誰か）とマスク（誰と当たるか）をビットフラグで定義
enum CollisionAttribute : uint32_t {
  kCollisionAttributePlayer = 0b00000001,       // 1: 自機
  kCollisionAttributeEnemy = 0b00000010,        // 2: 敵
  kCollisionAttributePlayerBullet = 0b00000100, // 4: 自機の弾
  kCollisionAttributeEnemyBullet = 0b00001000,  // 8: 敵の弾
  kCollisionAttributeItem = 0b00010000,         // 16: アイテム

  // 全てと当たる
  kCollisionAttributeAll = 0xFFFFFFFF,
};
