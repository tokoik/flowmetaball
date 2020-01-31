#version 410 core
#extension GL_ARB_shading_language_420pack : enable

//
// 矩形の描画
//

// 材質
uniform vec4 kamb = vec4(0.2, 0.5, 0.7, 1.0);
uniform vec4 kdiff = vec4(0.2, 0.5, 0.7, 0.0);
uniform vec4 kspec = vec4(0.0, 0.0, 0.0, 0.0);            // 雲鏡面反射ないから値0.0
float kshi = 50.0;

// 光源
layout (std140, binding = 0) uniform Light
{
  vec4 lamb;
  vec4 ldiff;
  vec4 lspec;
  vec4 lpos;
};

// 法線ベクトルの変換行列
uniform mat4 mn;

// テクスチャ
uniform sampler2D image[2];

// 閾値
uniform float threshold;

// テクスチャ座標
in vec2 texcoord;

// フラグメントの色
layout (location = 0) out vec4 fc;

void main()
{
  // 現在のスライスのテクスチャをサンプリングする
	vec4 c = texture(image[0], texcoord);

	// ポテンシャルが閾値未満ならフラグメントを捨てる
  float w1 = c.w - threshold;
	if (w1 <= 0.0) discard;

  // ひとつ前のスライスのテクスチャをサンプリングする
	vec4 d = texture(image[1], texcoord);

  // ひとつ前のスライスのポテンシャルの方が大きければフラグメントを捨てる
  float w2 = c.w - d.w;
  if (w2 <= 0.0) discard;

  // ポテンシャルが閾値と一致する位置
  float t = w1 / w2;

  // 陰影付け
	vec3 v = -vec3(0.0, 0.0, 1.0);
	vec3 l = normalize(vec3(lpos));
	vec3 n = normalize(mat3(mn) * mix(vec3(c), vec3(d), t));
	vec3 h = normalize(l - v);
  float idiff = max(dot(n, l), 0.0);
  // vec4 idiff = max(dot(n, l), 0.0) * kdiff * ldiff + kamb * lamb;
  vec4 ispec = pow(max(dot(n, h), 0.0), kshi) * kspec * lspec;

  // 閾値(トゥーンシェーディング)
  float edge1t = 0.1;

  // step(a, x):aは閾値・xはチェックされる値
  // 閾値未満の場合は0.0, それ以外は1.0を返す
  float Ikichi1t = step(edge1t, idiff);

  if(Ikichi1t == 0.0)
  {
    // 灰色
    fc = vec4(0.6, 0.66, 0.69, c * 0.3);

    // 水色
    // fc = vec4(0.67, 0.79, 0.87, c * 0.3);
  }
  else
  {
    // 白色
    fc = vec4(1.0, 1.0, 1.0, c * 0.3);
  }
  
  // 色
  //fc = vec4(1.0, 1.0, 1.0, c * 0.3);

  //fc = idiff + ispec;

  //fc = vec4(n * 0.5 + 0.5, 1.0);
}
