#include <Novice.h>
#include <cmath>
#include <corecrt_math.h>
#include <cstdint>
#include <imgui.h>

const char kWindowTitle[] = "LE2D_02_アオヤギ_ガクト_確認課題";

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

struct Vector3 {
    float x, y, z;
};

struct Matrix4x4 {
    float m[4][4];
};

struct Sphere {
    Vector3 center;
    float radius;
};

Matrix4x4 MakeViewProjectionMatrix(const Vector3& cameraTranslate, const Vector3& cameraRotate)
{
    // 単位行列で初期化
    Matrix4x4 view = {};
    for (int i = 0; i < 4; ++i)
        view.m[i][i] = 1.0f;

    // 回転行列（Y→X→Zの順で回転）
    float cosY = cosf(-cameraRotate.y);
    float sinY = sinf(-cameraRotate.y);
    float cosX = cosf(-cameraRotate.x);
    float sinX = sinf(-cameraRotate.x);
    float cosZ = cosf(-cameraRotate.z);
    float sinZ = sinf(-cameraRotate.z);

    //（Y軸回転）
    Matrix4x4 rotY = {};
    rotY.m[0][0] = cosY;
    rotY.m[0][2] = sinY;
    rotY.m[1][1] = 1.0f;
    rotY.m[2][0] = -sinY;
    rotY.m[2][2] = cosY;
    rotY.m[3][3] = 1.0f;

    //（X軸回転）
    Matrix4x4 rotX = {};
    rotX.m[0][0] = 1.0f;
    rotX.m[1][1] = cosX;
    rotX.m[1][2] = -sinX;
    rotX.m[2][1] = sinX;
    rotX.m[2][2] = cosX;
    rotX.m[3][3] = 1.0f;

    //（Z軸回転）
    Matrix4x4 rotZ = {};
    rotZ.m[0][0] = cosZ;
    rotZ.m[0][1] = -sinZ;
    rotZ.m[1][0] = sinZ;
    rotZ.m[1][1] = cosZ;
    rotZ.m[2][2] = 1.0f;
    rotZ.m[3][3] = 1.0f;

    // 回転合成（Z→X→Yの順）
    auto Multiply = [](const Matrix4x4& a, const Matrix4x4& b) {
        Matrix4x4 r = {};
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                for (int k = 0; k < 4; ++k)
                    r.m[i][j] += a.m[i][k] * b.m[k][j];
        return r;
    };
    Matrix4x4 rot = Multiply(Multiply(rotZ, rotX), rotY);

    // ビュー行列 = 回転 * 平行移動
    view = rot;
    view.m[0][3] = -(cameraTranslate.x * view.m[0][0] + cameraTranslate.y * view.m[0][1] + cameraTranslate.z * view.m[0][2]);
    view.m[1][3] = -(cameraTranslate.x * view.m[1][0] + cameraTranslate.y * view.m[1][1] + cameraTranslate.z * view.m[1][2]);
    view.m[2][3] = -(cameraTranslate.x * view.m[2][0] + cameraTranslate.y * view.m[2][1] + cameraTranslate.z * view.m[2][2]);
    view.m[3][3] = 1.0f;

    // プロジェクション行列
    Matrix4x4 proj = {};
    float fovY = 60.0f * (M_PI / 180.0f);
    float aspect = 1280.0f / 720.0f;
    float nearZ = 0.1f;
    float farZ = 100.0f;
    float f = 1.0f / tanf(fovY / 2.0f);
    proj.m[0][0] = f / aspect;
    proj.m[1][1] = f;
    proj.m[2][2] = (farZ + nearZ) / (nearZ - farZ);
    proj.m[2][3] = (2.0f * farZ * nearZ) / (nearZ - farZ);
    proj.m[3][2] = -1.0f;
    proj.m[3][3] = 0.0f;

    Matrix4x4 viewProj = Multiply(proj, view);
    return viewProj;
}

Matrix4x4 MakeViewportForMatrix(float left, float top, float width, float height, float minDepth, float maxDepth)
{
    Matrix4x4 m = {};
    m.m[0][0] = width * 0.5f;
    m.m[1][1] = -height * 0.5f;
    m.m[2][2] = (maxDepth - minDepth);
    m.m[3][0] = left + width * 0.5f;
    m.m[3][1] = top + height * 0.5f;
    m.m[3][2] = minDepth;
    m.m[3][3] = 1.0f;
    return m;
}

Vector3 Transform(const Vector3& v, const Matrix4x4& m)
{
    float x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
    float y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
    float z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
    float w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];
    if (w != 0.0f) {
        x /= w;
        y /= w;
        z /= w;
    }
    return { x, y, z };
}

/*
void DrawGrid(const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix)
{
    const float kGridHalfWidth = 2.0f;
    const uint32_t kSubdivision = 10;
    const float kGridEvery = (kGridHalfWidth * 2.0f) / float(kSubdivision);
    // 奥から手前への線を順々にひいていく
    for (uint32_t xIndex = 0; xIndex <= kSubdivision; ++xIndex) {
        // 上の情報を使ってワールド座標計上の始点と終点を求める

        // スクリーン座標系まで返還をかける
        // 変換した座標を使って表示。色は薄い灰色(0xAAAAAAFF),原点は黒ぐらいがいいが、何でも良い
        Novice::DrawLine(
            );
    }
    for (uint32_t zIndex = 0; zIndex <= kSubdivision; ++zIndex) {
        // 奥から手前が左右に代わるだけ

    }
}
*/

void DrawGrid(const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix)
{
    const float kGridHalfWidth = 2.0f;
    const uint32_t kSubdivision = 10;
    const float kGridEvery = (kGridHalfWidth * 2.0f) / float(kSubdivision);

    // X方向（縦線）
    for (uint32_t xIndex = 0; xIndex <= kSubdivision; ++xIndex) {
        float x = -kGridHalfWidth + xIndex * kGridEvery;
        Vector3 start = { x, 0.0f, -kGridHalfWidth };
        Vector3 end = { x, 0.0f, kGridHalfWidth };

        start = Transform(start, viewProjectionMatrix);
        start = Transform(start, viewportMatrix);
        end = Transform(end, viewProjectionMatrix);
        end = Transform(end, viewportMatrix);

        Novice::DrawLine((int)start.x, (int)start.y, (int)end.x, (int)end.y, 0xAAAAAAFF);
    }

    // Z方向（横線）
    for (uint32_t zIndex = 0; zIndex <= kSubdivision; ++zIndex) {
        float z = -kGridHalfWidth + zIndex * kGridEvery;
        Vector3 start = { -kGridHalfWidth, 0.0f, z };
        Vector3 end = { kGridHalfWidth, 0.0f, z };

        start = Transform(start, viewProjectionMatrix);
        start = Transform(start, viewportMatrix);
        end = Transform(end, viewProjectionMatrix);
        end = Transform(end, viewportMatrix);

        Novice::DrawLine((int)start.x, (int)start.y, (int)end.x, (int)end.y, 0xAAAAAAFF);
    }
}

/*
void DrawSphere(const Sphere& sphere, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color)
{
    const uint32_t kSubdivision = ;// 分割数
    const float kLonEvery = ;　// 経度の分割間隔
    const float kLatEvery = ;　// 緯度の分割間隔
    // 緯度の分割間隔は-π/2~π/2
    for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
        float lat = -pI / 2.0f + latIndex * kLatEvery;
        // 経度の方向に分割0~2π
        for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
            float lon = lonIndex * kLonEvery;
            // would座標系でのa,b,cを求める
            Vector3 a ,b,c;
            // a,b,cをScreen座標系まで変換
            // a,b,cで線を引く
            Novice::DrawLine(
                );
            Novice::DrawLine(
                );
        }
    }
}
*/

void DrawSphere(const Sphere& sphere, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color)
{
    const uint32_t kSubdivision = 16;
    const float kLatEvery = M_PI / float(kSubdivision); // 緯度：-π/2 ～ π/2
    const float kLonEvery = 2.0f * M_PI / float(kSubdivision); // 経度：0 ～ 2π

    for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
        float lat = -M_PI / 2.0f + kLatEvery * latIndex;
        for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
            float lon = lonIndex * kLonEvery;

            // 各点の球面座標 → ワールド座標変換
            Vector3 a {
                sphere.center.x + sphere.radius * cosf(lat) * cosf(lon),
                sphere.center.y + sphere.radius * sinf(lat),
                sphere.center.z + sphere.radius * cosf(lat) * sinf(lon)
            };
            Vector3 b {
                sphere.center.x + sphere.radius * cosf(lat + kLatEvery) * cosf(lon),
                sphere.center.y + sphere.radius * sinf(lat + kLatEvery),
                sphere.center.z + sphere.radius * cosf(lat + kLatEvery) * sinf(lon)
            };
            Vector3 c {
                sphere.center.x + sphere.radius * cosf(lat) * cosf(lon + kLonEvery),
                sphere.center.y + sphere.radius * sinf(lat),
                sphere.center.z + sphere.radius * cosf(lat) * sinf(lon + kLonEvery)
            };

            // スクリーン座標に変換
            a = Transform(a, viewProjectionMatrix);
            a = Transform(a, viewportMatrix);
            b = Transform(b, viewProjectionMatrix);
            b = Transform(b, viewportMatrix);
            c = Transform(c, viewProjectionMatrix);
            c = Transform(c, viewportMatrix);

            // 線で描画
            Novice::DrawLine((int)a.x, (int)a.y, (int)b.x, (int)b.y, color);
            Novice::DrawLine((int)a.x, (int)a.y, (int)c.x, (int)c.y, color);
        }
    }
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{

    // ライブラリの初期化
    Novice::Initialize(kWindowTitle, 1280, 720);

    // キー入力結果を受け取る箱
    char keys[256] = { 0 };
    char preKeys[256] = { 0 };
    Vector3 cameraTranslate { 0.0f, 1.9f, -6.49f };
    Vector3 cameraRotate { 0.26f, 0.0f, 0.0f };
    Sphere sphere = { { 0.0f, 0.0f, 0.0f }, 0.5f };
    Matrix4x4 viewportMatrix = MakeViewportForMatrix(
        0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f);
    Matrix4x4 viewProjectionMatrix = MakeViewProjectionMatrix(cameraTranslate, cameraRotate);

    // ウィンドウの×ボタンが押されるまでループ
    while (Novice::ProcessMessage() == 0) {
        // フレームの開始
        Novice::BeginFrame();

        // キー入力を受け取る
        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        ///
        /// ↓更新処理ここから
        ///

        // 行列を毎フレーム更新
        viewProjectionMatrix = MakeViewProjectionMatrix(cameraTranslate, cameraRotate);

        ///
        /// ↑更新処理ここまで
        ///

        ///
        /// ↓描画処理ここから
        ///

        ImGui::Begin("Window");
        ImGui::DragFloat3("CameraTranslate", &cameraTranslate.x, 0.01f);
        ImGui::DragFloat3("CameraRotate", &cameraRotate.x, 0.01f);
        ImGui::DragFloat3("SphereCenter", &sphere.center.x, 0.01f);
        ImGui::DragFloat("SphereRadius", &sphere.radius, 0.01f);
        ImGui::End();

        DrawSphere(
            sphere,
            viewProjectionMatrix,
            viewportMatrix,
            BLACK);

        DrawGrid(
            viewProjectionMatrix,
            viewportMatrix);

        ///
        /// ↑描画処理ここまで
        ///

        // フレームの終了
        Novice::EndFrame();

        // ESCキーが押されたらループを抜ける
        if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
            break;
        }
    }

    // ライブラリの終了
    Novice::Finalize();
    return 0;
}
