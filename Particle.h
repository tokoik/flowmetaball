#pragma once
#include "gg.h"
using namespace gg;

// 標準ライブラリ
#include <array>
#include <vector>

//
// ベクトルデータ
//
using Vector = std::array<GLfloat, 4>;

//
// 粒子データ
//
struct Particle
{
  // 位置
  Vector position;

  // 速度
  Vector velocity;

  // 力（引力・斥力）
  Vector force;

  // デフォルトコンストラクタ
  Particle()
  {}

  // 引数で初期化を行うコンストラクタ
  Particle(float px, float py, float pz,
    float vx = 0.0f, float vy = 0.0f, float vz = 0.0f,
    float fx = 0.0f, float fy = 0.0f, float fz = 0.0f)
    : position{ px, py, pz, 1.0f }, velocity{ vx, vy, vz, 0.0f },
    force{ fx, fy, fz, 0.0}
  {}

  // デストラクタ
  ~Particle()
  {}
};

//
// 粒子群データ
//
using Particles = std::vector<Particle>;
