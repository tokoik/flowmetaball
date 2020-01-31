#version 430 core

// フレームバッファに出力するデータ
layout (location = 0) out vec4 color;               // フラグメントの色

void main()
{
  // フラグメントの色の出力
  color = vec4(1.0);
}
