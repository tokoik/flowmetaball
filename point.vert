#version 430 core

// 頂点属性
layout (location = 0) in vec4 position;             // 現在の位置

// 変換行列
uniform mat4 mp;                                    // 投影変換行列
uniform mat4 mv;                                    // モデルビュー変換行列

void main()
{
  // モデルビュー変換
  vec4 p = mv * position;                           // 視点座標系の頂点の位置

  // 投影変換
  gl_Position = mp * p;
}
