#pragma once
#include "Window.h"
#include "Particle.h"

//
// 実験用の設定
//

// メタボールをバケットソートする場合は 1
#define SORT 1

// デプスバッファを使う場合は 1
#define DEPTH 1

// 時間を計測する場合は 1
#define TIME 1

// glFinish() を実行する場合は 1
#define USE_GLFINISH 0

//
// スライス
//

// スライスの数
constexpr int sliceCount(128);

// スライス用の FBO のサイズ
constexpr GLsizei sliceWidth(128), sliceHeight(128);

//
// 粒子群オブジェクト
//
class Blob
{
  // 粒子群
  const Particles &particles;

  // 頂点配列オブジェクト名
  GLuint vao;

  // 頂点バッファオブジェクト名
  GLuint vbo;

  // 描画用のシェーダ
  const GLuint drawShader;

  // unform 変数の場所
  const GLint mpLoc, mvLoc;

  // 更新用のシェーダ
  const GLuint updateShader;

  // スライス用のフレームバッファオブジェクト
  GLuint sliceFbo[2];

  // スライス用のフレームバッファのレンダーターゲットに使うテクスチャ
  GLuint sliceColor[2];

#if SORT
  // 粒子のインデックスの頂点バッファオブジェクト名
  GLuint ibo;
#endif

  //
  // ポテンシャルの書き込み
  //

  // ポテンシャルを描くシェーダ
  const GgPointShader potentialShader;

  // スライスの位置
  const GLint zSliceLoc;

  // メタボールの半径
  const GLint radiusLoc;

  // フレームバッファのサイズ
  const GLint sizeLoc;

  //
  // 等値面表示
  //

  // 矩形を描くシェーダ
  const GLuint rectangleShader;

  // uniform block の場所を取得する
  const GLint lightlLoc;

#if DEPTH
  // スクリーン座標系での奥行き値
  const GLint zClipLoc;
#endif

  // 法線変換行列
  const GLint mnLoc;

  // テクスチャ
  const GLint imageLoc;

  // 閾値
  const GLint thresholdLoc;

  //
  // 粒子位置の更新
  //

  // ばねの自然長
  const GLint equilibriumLoc;

  // ばね定数
  const GLint springLoc;

  // 減衰率
  const GLint attenuationLoc;

  // 粒子の質量
  const GLint weightLoc;

  // 粒子の影響範囲
  const GLint rangeLoc;

  // タイムステップ
  const GLint timestepLoc;

  // 衝突させる球
  const GLint sphereLoc;

public:

  // コンストラクタ
  Blob(const Particles &particles);

  // デストラクタ
  virtual ~Blob();

  // コピーコンストラクタによるコピー禁止
  Blob(const Blob &blob) = delete;

  // 代入によるコピー禁止
  Blob &operator=(const Blob &blob) = delete;

  // 初期化
  void initialize(const Particles &particles) const;

  // 描画
  void draw(const GgMatrix &mp, const GgMatrix &mv) const;

  // メタボールで描画
  void drawMetaball(const GgMatrix &mp, const GgMatrix &mv,
    const Window &window, const GgSimpleShader::LightBuffer &light) const;

  // 更新
  void update(GLfloat x, GLfloat y, GLfloat z, GLfloat r,
    GLfloat equilibrium = 0.1f, GLfloat spring = 1.0f, GLfloat attenuation = 5.0f,
    GLfloat weight = 1.0f, GLfloat range = 0.5f, GLfloat timestep = 0.01666667f) const;

};
