//
// アプリケーション本体
//
#include "GgApplication.h"

// 粒子群オブジェクト
#include "Blob.h"

// 標準ライブラリ
#include <memory>
#include <random>

//
// カメラ
//

// 位置
constexpr GLfloat cameraPosition[] = { 0.0f, 0.0f, 5.0f };

// 目標点
constexpr GLfloat cameraTarget[] = { 0.0f, 0.0f, 0.0f };

// 上方向
constexpr GLfloat cameraUp[] = { 0.0f, 1.0f, 0.0f };

// 画角
constexpr GLfloat cameraFovy(1.0f);

// 前方面の位置
constexpr GLfloat cameraNear(1.0f);

// 後方面の位置
constexpr GLfloat cameraFar(10.0f);

//
// 光源
//

//   ambient 光源強度の環境光成分
//   diffuse 光源強度の拡散反射光成分
//   specular 光源強度の鏡面反射光成分
//   position 光源の位置
constexpr GgSimpleShader::Light lightData =
{
  { 0.1f, 0.1f, 0.1f, 1.0f },
  { 1.0f, 1.0f, 1.0f, 1.0f },
  { 1.0f, 1.0f, 1.0f, 1.0f },
  { 3.0f, 4.0f, 5.0f, 1.0f }
};

//
// 材質
//

//   ambient 環境光の反射係数
//   diffuse 拡散反射係数
//   specular 鏡面反射係数
//   shininess 輝き係数
constexpr GgSimpleShader::Material materialData =
{
  { 0.6f, 0.6f, 0.6f, 1.0f },
  { 0.6f, 0.6f, 0.6f, 0.0f },
  { 0.4f, 0.4f, 0.4f, 0.0f },
  30.0f
};

//
// 球
//

// 球の半径
constexpr float sphereRadius(0.5f);

// 球の分割数
constexpr int slices(32), stacks(16);

// 球の速度 (倍速の値)
constexpr float dSpeed(2.0f);

//
// 粒子群
//

// 生成する粒子群の数
constexpr int bCount(8);

// 生成する粒子群の中心位置の範囲
constexpr GLfloat bRange(0.8f);

// 一つの粒子群の粒子数
constexpr int pCount(2000);

// 一つの粒子群の中心からの距離の平均
constexpr GLfloat pMean(0.0f);

// 一つの粒子群の中心からの距離の標準偏差
constexpr GLfloat pDeviation(0.3f);

// アニメーションの繰り返し間隔
constexpr double interval(5.0);

//
// 粒子群の生成
//
//   paticles 粒子群の格納先
//   count 粒子群の粒子数
//   cx, cy, cz 粒子群の中心位置
//   rn メルセンヌツイスタ法による乱数
//   mean 粒子の粒子群の中心からの距離の平均値
//   deviation 粒子の粒子群の中心からの距離の標準偏差
//
void generateParticles(Particles &particles, int count,
  GLfloat cx, GLfloat cy, GLfloat cz,
  std::mt19937 &rn, GLfloat mean, GLfloat deviation)
{
  // 一様実数分布
  //   [0, 2) の値の範囲で等確率に実数を生成する
  std::uniform_real_distribution<GLfloat> uniform(0.0f, 2.0f);

  // 正規分布
  //   平均 mean、標準偏差 deviation で分布させる
  std::normal_distribution<GLfloat> normal(mean, deviation);

  // 格納先のメモリをあらかじめ確保しておく
  particles.reserve(particles.size() + count);

  // 原点中心に直径方向に正規分布する粒子群を発生する
  for (int i = 0; i < count; ++i)
  {
    // 緯度方向
    const GLfloat cp(uniform(rn) - 1.0f);
    const GLfloat sp(sqrt(1.0f - cp * cp));

    // 経度方向
    const GLfloat t(3.1415927f * uniform(rn));
    const GLfloat ct(cos(t)), st(sin(t));

    // 粒子の粒子群の中心からの距離 (半径)
    const GLfloat r(normal(rn));

    // 粒子を追加する
    //   (静止した粒子をランダムに配置する場合)
    particles.emplace_back(r * sp * ct + cx, r * sp * st + cy, r * cp + cz);
    //   (中心からランダムな方向に発射する場合)
    //particles.emplace_back(cx, cy, cz, r * sp * ct, r * sp * st, r * cp);
  }
}

//
// アプリケーションの実行
//
void GgApplication::run()
{
  // ウィンドウを作成する
  Window window("FlowMetaball");

  //
  // 粒子群オブジェクトの作成
  //

  // メルセンヌツイスタ法による乱数の系列を設定する
  //   (ハードウェア乱数を種に使って実行ごとに初期形状を変える場合)
  //std::random_device seed;
  //std::mt19937 rn(seed());
  //   (定数にして毎回同じ初期形状にする場合)
  std::mt19937 rn(54321);

  // 一様実数分布
  //   [-1.0, 1.0) の値の範囲で等確率に実数を生成する
  std::uniform_real_distribution<GLfloat> center(-bRange, bRange);

  // 粒子群データの初期値
  Particles initial;

  // 発生する粒子群の数だけ繰り返す
  for (int i = 0; i < bCount; ++i)
  {
    // 点の玉中心位置
    const GLfloat cx(center(rn)), cy(center(rn)), cz(center(rn));

    // 中心からの距離に対して密度が正規分布に従う点の玉を生成する
    generateParticles(initial, pCount, cx, cy, cz, rn, pMean, pDeviation);
  }

  // 粒子群オブジェクトを作成する
  std::unique_ptr<const Blob> blob(new Blob(initial));

  //
  // 位置更新用のシェーダ
  //
  const GLuint updateShader(ggLoadComputeShader("update.comp"));

  // 図形描画用のシェーダ
  const GgSimpleShader simpleShader("simple.vert", "simple.frag");

  // 図形描画用の光源
  const GgSimpleShader::LightBuffer light(lightData);

  //
  // 物体
  //

  // 図形データ
  //std::unique_ptr<const GgSimpleObj> object(new GgSimpleObj("AC_1038.obj", true));
  //std::unique_ptr<const GgSimpleObj> object(new GgSimpleObj("bunny.obj", true);
  //std::unique_ptr<const GgSimpleObj> object(new GgSimpleObj("box.obj", true);

  // 球を生成する
  std::unique_ptr<const GgElements> object(ggElementsSphere(sphereRadius, slices, stacks));

  // 球の材質
  const GgSimpleShader::MaterialBuffer material(materialData);

  //
  // 描画の設定
  //

  // ビュー変換行列
  const GgMatrix view(ggLookat(cameraPosition, cameraTarget, cameraUp));

  // 背面カリングを有効にする
  glEnable(GL_CULL_FACE);

  // デプスバッファを有効にする
  glEnable(GL_DEPTH_TEST);

  // 背景色を指定する
  glClearColor(0.1f, 0.2f, 0.3f, 0.0f);

  // 点のサイズはシェーダから変更する
  glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

  /*
  ** レンダリング結果のブレンド
  **
  **   glBlendFunc(GL_ONE, GL_ZERO);                       // 上書き（デフォルト）
  **   glBlendFunc(GL_ZERO, GL_ONE);                       // 描かない
  **   glBlendFunc(GL_ONE, GL_ONE);                        // 加算
  **   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // 通常のアルファブレンデング
  **   glBlendColor(0.01f, 0.01f, 0.01f, 0.0f);            // 加算する定数
  **   glBlendFunc(GL_CONSTANT_COLOR, GL_ONE);             // 定数を加算
  */

  // フレームバッファに加算する
  /**/
  // ポイントスプライトの設定
  // アルファブレンディングを設定する
  //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBlendFunc(GL_ONE, GL_ONE);
  glBlendEquation(GL_FUNC_ADD);

#if !DEPTH
  // デプスバッファは使わない
  glDisable(GL_DEPTH_TEST);
#endif

  // 時計をリセットする
  glfwSetTime(0.0);

  //
  // 描画
  //
  while (window)
  {
    // ウィンドウを消去する
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 透視投影変換行列を求める
    const GgMatrix projection(ggPerspective(cameraFovy, window.getAspect(), cameraNear, cameraFar));

    // モデル変換行列を求める
    const GgMatrix model(window.getTrackball());

    // モデルビュー変換行列を求める
    const GgMatrix modelview(view * model);

    // 粒子群オブジェクトを描画する
    //blob->draw(projection, modelview);

    //メタボールの描画
    blob->drawMetaball(projection, modelview, window, light);

    //
    // 球描画
    //

    // 初期位置
    float sphereX = 0.0f;
    float sphereY = 0.0f;
    float sphereZ = 3.0f - static_cast<float>(glfwGetTime()) * dSpeed;

    // 雲がちぎれる表現                    球の中心座標 x        y        z
    const GgMatrix animation(modelview * ggTranslate(sphereX, sphereY, sphereZ));

    // オブジェクトのシェーダプログラムの使用開始
    simpleShader.use(projection, animation, light);

    // アルファブレンディングを無効にして図形を描画する
    glDisable(GL_BLEND);
    material.select();
    object->draw();

    // 粒子群オブジェクトを更新する
    blob->update(sphereX, sphereY, sphereZ, sphereRadius);

    // カラーバッファを入れ替える
    window.swapBuffers();

    // 定期的に粒子群オブジェクトをリセットする
    if (glfwGetTime() > interval)
    {
      // 粒子を捨てる
      initial.clear();

      // 発生する粒子群の数だけ繰り返す
      for (int i = 0; i < bCount; ++i)
      {
        // 点の玉中心位置
        const GLfloat cx(center(rn)), cy(center(rn)), cz(center(rn));

        // 中心からの距離に対して密度が正規分布に従う点の玉を生成する
        generateParticles(initial, pCount, cx, cy, cz, rn, pMean, pDeviation);
      }

      blob->initialize(initial);
      glfwSetTime(0.0);
    }
  }
}
