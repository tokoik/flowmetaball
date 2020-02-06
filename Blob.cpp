//
// 粒子群オブジェクト
//
#include "Blob.h"

// レンダーターゲット
constexpr GLenum sliceBufs[] = { GL_COLOR_ATTACHMENT0 };

// レンダーターゲットの初期値
constexpr GLfloat zero[] = { 0.0f, 0.0f, 0.f, 0.0f };

// コンストラクタ
Blob::Blob(const Particles &particles)
  : particles(particles)
  , drawShader(ggLoadShader("point.vert", "point.frag"))
  , mpLoc(glGetUniformLocation(drawShader, "mp"))
  , mvLoc(glGetUniformLocation(drawShader, "mv"))
  , updateShader(ggLoadComputeShader("update.comp"))
  , potentialShader("potential.vert", "potential.frag")
  , zSliceLoc(glGetUniformLocation(potentialShader.get(), "zSlice"))
  , radiusLoc(glGetUniformLocation(potentialShader.get(), "radius"))
  , sizeLoc(glGetUniformLocation(potentialShader.get(), "size"))
  , rectangleShader(ggLoadShader("rectangle.vert", "rectangle.frag"))
  , lightlLoc(glGetUniformBlockIndex(rectangleShader, "Light"))
#if DEPTH
  // スクリーン座標系での奥行き値
  , zClipLoc(glGetUniformLocation(rectangleShader, "zClip"))
#endif
  , mnLoc(glGetUniformLocation(rectangleShader, "mn"))
  , imageLoc(glGetUniformLocation(rectangleShader, "image"))
  , thresholdLoc(glGetUniformLocation(rectangleShader, "threshold"))
  , springLoc(glGetUniformLocation(updateShader, "spring"))
  , attenuationLoc(glGetUniformLocation(updateShader, "attenuation"))
  , timestepLoc(glGetUniformLocation(updateShader, "timestep"))
  , rangeLoc(glGetUniformLocation(updateShader, "range"))
  , equilibriumLoc(glGetUniformLocation(updateShader, "equilibrium"))
  , weightLoc(glGetUniformLocation(updateShader, "weight"))
  , sphereLoc(glGetUniformLocation(updateShader, "sphere"))
{
  // uniform block の場所を 0 番の結合ポイントに結びつける
  glUniformBlockBinding(rectangleShader, lightlLoc, 0);

  //
  // 粒子群
  //

  // 頂点配列オブジェクトを作成する
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // 頂点バッファオブジェクトを作成する
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(Particle), NULL, GL_STATIC_DRAW);

  // 結合されている頂点バッファオブジェクトを in 変数から参照できるようにする
  glVertexAttribPointer(0, std::tuple_size<Vector>::value, GL_FLOAT, GL_FALSE,
    sizeof(Particle), &static_cast<const Particle *>(0)->position);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, std::tuple_size<Vector>::value, GL_FLOAT, GL_FALSE,
    sizeof(Particle), &static_cast<const Particle *>(0)->velocity);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, std::tuple_size<Vector>::value, GL_FLOAT, GL_FALSE,
    sizeof(Particle), &static_cast<const Particle *>(0)->force);
  glEnableVertexAttribArray(2);

  // 頂点バッファオブジェクトにデータを格納する
  initialize(particles);

#if SORT
  // 点群のインデックスの頂点バッファオブジェクト
  GLuint ibo;
  glGenBuffers(1, &ibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, particles.size() * sizeof(GLuint), NULL, GL_STREAM_DRAW);
#endif

  // 頂点配列オブジェクトを標準に戻す
  glBindVertexArray(0);

  //
  // スライス
  //

  // スライス用のフレームバッファオブジェクトを作成する
  glGenFramebuffers(2, sliceFbo);

  // スライス用のフレームバッファのレンダーターゲットに使うテクスチャ
  glGenTextures(2, sliceColor);

  for (int i = 0; i < 2; ++i)
  {
    // レンダーターゲットのテクスチャを準備する
    glBindTexture(GL_TEXTURE_2D, sliceColor[i]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, sliceWidth, sliceHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // フレームバッファオブジェクトに組み込む
    glBindFramebuffer(GL_FRAMEBUFFER, sliceFbo[i]);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, sliceColor[i], 0);

    // カラーバッファを消去する
    glDrawBuffers(1, sliceBufs);
    glClearBufferfv(GL_COLOR, 0, zero);
  }

  // 通常のフレームバッファに戻す
  glBindFramebuffer(GL_FRAMEBUFFER, 0);


}

// デストラクタ
Blob::~Blob()
{
  // 頂点配列オブジェクトを削除する
  glDeleteBuffers(1, &vao);

  // 頂点バッファオブジェクトを削除する
  glDeleteBuffers(1, &vbo);

#if SORT
  // 点群のインデックスの頂点バッファオブジェクトを削除する
  GLuint ibo;
  glDeleteBuffers(1, &ibo);
#endif

  // スライス用のフレームバッファオブジェクトを削除する
  glDeleteFramebuffers(2, sliceFbo);

  // スライス用のフレームバッファオブジェクトのレンダーターゲットのテクスチャを削除する
  glDeleteTextures(2, sliceColor);
}

// 初期化
void Blob::initialize(const Particles &particles) const
{
  // 頂点バッファオブジェクトを結合する
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  //**200128
  // シェーダストレージバッファオブジェクトを 0 番の結合ポイントに結合する
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vbo);

  //  頂点バッファオブジェクトにデータを格納する
  Particle *p(static_cast<Particle *>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY)));
  for (auto particle : particles)
  {
    p->position = particle.position;
    p->velocity = particle.velocity;
    p->force = particle.force;

    ++p;
  }
  glUnmapBuffer(GL_ARRAY_BUFFER);
}

// 描画
void Blob::draw(const GgMatrix &mp, const GgMatrix &mv) const
{
  // 描画する頂点配列オブジェクトを指定する
  glBindVertexArray(vao);

  // 点のシェーダプログラムの使用開始
  glUseProgram(drawShader);

  // uniform 変数を設定する
  glUniformMatrix4fv(mpLoc, 1, GL_FALSE, mp.get());
  glUniformMatrix4fv(mvLoc, 1, GL_FALSE, mv.get());

  // 点で描画する
  glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(particles.size()));
}

// メタボールで描画
void Blob::drawMetaball(const GgMatrix &mp, const GgMatrix &mv,
  const GgApplication::Window &window, const GgSimpleShader::LightBuffer &light) const
{

  // 法線変換行列
  const GgMatrix mn(mv.normal());

  // メタボールの半径
  const GLfloat sphereRadius(0.1f);
  //const GLfloat sphereRadius(window.getWheel(0) * 0.01f + 0.2f);

#if SORT
  //
  // バケットソート
  //

  // バケット
  static std::array<std::vector<GLuint>, sliceCount> bucket;

  // バケットのクリア
  for (int bucketNum = 0; bucketNum < sliceCount; ++bucketNum)
    bucket[bucketNum].clear();

  // すべてのメタボールの中心について
  for (size_t pointNum = 0; pointNum < particles.size(); ++pointNum)
  {
    // メタボールの中心の視点座標系の z 値
    const GLfloat zw(mv.get(2) * particles[pointNum].position[0]
      + mv.get(6) * particles[pointNum].position[1]
      + mv.get(10) * particles[pointNum].position[2] + mv.get(14));

    // メタボールの前端のスクリーン座標系における z 値
    const GLfloat zsf(mp.get(10) + mp.get(14) / (zw + sphereRadius));

    // zsf の符号を反転してメタボールの前端の位置におけるバケット番号を求める
    int bucketFront(static_cast<int>(ceil((0.5f - zsf * 0.5f) * sliceCount - 0.5f)));
    if (bucketFront < 0) bucketFront = 0;

    // メタボールの後端のスクリーン座標系における z 値
    const GLfloat zsr(mp.get(10) + mp.get(14) / (zw - sphereRadius));

    // zsr の符号を反転してメタボールの後端の位置におけるバケット番号を求める
    int bucketBack(static_cast<int>(floor((0.5f - zsr * 0.5f) * sliceCount - 0.5f)));
    if (bucketBack >= sliceCount) bucketBack = sliceCount - 1;

    // バケットソート
    for (int bucketNum = bucketFront; bucketNum <= bucketBack; ++bucketNum)
      bucket[bucketNum].emplace_back(static_cast<GLuint>(pointNum));
  }
#endif

#if DEPTH
  // 標示用のカラーバッファとデプスバッファを消去する
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#else
  // 標示用のカラーバッファを消去する
  glClear(GL_COLOR_BUFFER_BIT);
#endif

  // テクスチャを割り当てる
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, sliceColor[0]);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, sliceColor[1]);

  // 描画するスライス
  int f(0);

  // 各スライスについて
  for (int sliceNum = 0; sliceNum < sliceCount; ++sliceNum)
  {
    // スクリーン座標系におけるスライスの位置
    const GLfloat zClip(static_cast<GLfloat>(sliceNum * 2 + 1) / static_cast<GLfloat>(sliceCount) - 1.0f);

    // 視点座標系のスライスの位置
    const GLfloat zSlice(mp.get(14) / (zClip * mp.get(11) - mp.get(10)));

    // FBO に描く
    glBindFramebuffer(GL_FRAMEBUFFER, sliceFbo[f]);
    glDrawBuffers(1, sliceBufs);
    glClearBufferfv(GL_COLOR, 0, zero);

    // ビューポートをスライス用の FBO のサイズに設定する
    glViewport(0, 0, sliceWidth, sliceHeight);

    // メタボールを描くシェーダを指定する
    potentialShader.use();

    // フレームバッファのサイズを設定する
    glUniform2f(sizeLoc, static_cast<GLfloat>(sliceWidth), static_cast<GLfloat>(sliceHeight));

    // 変換行列を設定する
    potentialShader.loadMatrix(mp, mv);

    // スライスの位置を設定する
    glUniform1f(zSliceLoc, zSlice);

    // メタボールの半径を設定する
    glUniform1f(radiusLoc, sphereRadius);

#if DEPTH
    // メタボールの描画時にはデプスバッファは使わない
    glDisable(GL_DEPTH_TEST);
#endif

    // カラーバッファへの加算を有効にする
    glEnable(GL_BLEND);

    // 描画
    glBindVertexArray(vao);

#if SORT
    // バケットソートするときはインデックスを使って描画する
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, bucket[sliceNum].size() * sizeof(GLuint), bucket[sliceNum].data());
    glDrawElements(GL_POINTS, static_cast<GLsizei>(bucket[sliceNum].size()), GL_UNSIGNED_INT, NULL);
#else
    // バケットソートしない場合は直接描画する
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(points.size()));
#endif

    // 通常のフレームバッファに描く
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, window.getWidth(), window.getHeight());

    // 矩形を描くシェーダを選択する
    glUseProgram(rectangleShader);

    // 光源のデータを設定する
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, light.getBuffer());

    // テクスチャユニットを指定する
    const GLint image[] = { f, 1 - f };
    glUniform1iv(imageLoc, 2, image);

    // 法線変換行列を設定する
    glUniformMatrix4fv(mnLoc, 1, GL_FALSE, mn.get());

    // 閾値を設定する
    glUniform1f(thresholdLoc, window.getWheelY() * 0.1f + 1.0f);

#if DEPTH
    // スライスのスクリーン座標系の奥行き値を設定する
    glUniform1f(zClipLoc, zClip);

    // スライスの描画時にはデプスバッファを使う
    glEnable(GL_DEPTH_TEST);
#endif

    // フレームバッファへの加算を無効にする
    glDisable(GL_BLEND);

    // 矩形を描画する
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // 描画するスライスを入れ替える
    f = 1 - f;
  }

}

// 更新
void Blob::update(GLfloat x, GLfloat y, GLfloat z, GLfloat r,
  GLfloat equilibrium, GLfloat spring, GLfloat attenuation,
  GLfloat weight, GLfloat range, GLfloat timestep) const
{
  // 更新用のシェーダプログラムの使用開始
  glUseProgram(updateShader);

  // ばねの自然長を設定する
  glUniform1f(equilibriumLoc, equilibrium);

  // ばね定数を設定する
  glUniform1f(springLoc, spring);

  // 空気抵抗を設定する
  glUniform1f(attenuationLoc, attenuation);

  // 粒子の質量を設定する
  glUniform1f(weightLoc, weight);

  // 粒子の影響範囲を設定する
  glUniform1f(rangeLoc, range);

  // タイムステップを設定する
  glUniform1f(timestepLoc, timestep);

  // 球のデータを設定する
  glUniform4f(sphereLoc, x, y, z, r);

  // 計算を実行する
  glDispatchCompute(static_cast<GLuint>(particles.size()), 1, 1);
}
