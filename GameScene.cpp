#include "GameScene.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "InputManager.h"
#include "SpriteRenderer.h"
#include "ModelRenderer.h"
#include "PhysicsSystem.h"
#include "AudioSystem.h"

GameScene::GameScene()
    : Scene()
    , m_playerPosition(640.0f, 360.0f)
    , m_playerSpeed(200.0f)
    , m_score(0)
    , m_gameOver(false)
    , m_gameClear(false)
    , m_timer(0.0f)
{
}

GameScene::~GameScene()
{
    Shutdown();
}

bool GameScene::Initialize()
{
    if (!Scene::Initialize()) {
        return false;
    }

    // カメラの作成
    m_camera = std::make_shared<Camera>();
    m_camera->SetPosition(DirectX::XMFLOAT3(0.0f, 2.0f, -5.0f));
    m_camera->SetTarget(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));

    // 衝突イベントリスナーの登録
    GetPhysicsSystem()->AddCollisionListener("GameScene",
        [this](const CollisionEvent& event) {
            OnCollision(event);
        }
    );

    return true;
}

void GameScene::Shutdown()
{
    // 衝突イベントリスナーの削除
    if (GetPhysicsSystem()) {
        GetPhysicsSystem()->RemoveCollisionListener("GameScene");
    }

    // すべてのオブジェクトをクリア
    m_sprites.clear();
    m_colliders.clear();
    m_rigidbodies.clear();
    m_audioSources.clear();

    m_playerSprite = nullptr;
    m_playerCollider = nullptr;
    m_playerRigidbody = nullptr;

    m_enemySprites.clear();
    m_enemyColliders.clear();

    m_bgmSource = nullptr;
    m_seHitSource = nullptr;
    m_seJumpSource = nullptr;

    m_backgroundSprite = nullptr;
    m_titleSprite = nullptr;
    m_scoreSprite = nullptr;

    m_camera = nullptr;

    Scene::Shutdown();
}

bool GameScene::Load()
{
    if (!Scene::Load()) {
        return false;
    }

    // アセットの読み込み
    if (!LoadAssets()) {
        return false;
    }

    // オブジェクトの初期化
    InitializeObjects();

    return true;
}

void GameScene::Unload()
{
    // BGMの停止
    if (m_bgmSource) {
        m_bgmSource->Stop();
    }

    Scene::Unload();
}

void GameScene::Update(float deltaTime)
{
    Scene::Update(deltaTime);

    // シーンが一時停止中なら更新しない
    if (m_isPaused) {
        return;
    }

    // ゲームオーバーまたはゲームクリア時は入力処理をスキップ
    if (!m_gameOver && !m_gameClear) {
        // 入力処理
        ProcessInput(deltaTime);

        // タイマー更新
        m_timer += deltaTime;
    }

    // プレイヤーの位置を更新
    if (m_playerSprite) {
        m_playerSprite->position = m_playerPosition;
    }

    // 敵の更新
    for (size_t i = 0; i < m_enemySprites.size(); ++i) {
        // 敵の移動
        m_enemyPositions[i].x += m_enemyVelocities[i].x * deltaTime;
        m_enemyPositions[i].y += m_enemyVelocities[i].y * deltaTime;

        // 画面の端で反射
        float screenWidth = static_cast<float>(GetEngine()->GetRenderer()->GetViewport().Width);
        float screenHeight = static_cast<float>(GetEngine()->GetRenderer()->GetViewport().Height);

        if (m_enemyPositions[i].x < 0 || m_enemyPositions[i].x > screenWidth) {
            m_enemyVelocities[i].x *= -1;
        }

        if (m_enemyPositions[i].y < 0 || m_enemyPositions[i].y > screenHeight) {
            m_enemyVelocities[i].y *= -1;
        }

        // スプライトの位置を更新
        m_enemySprites[i]->position = m_enemyPositions[i];
    }
}

void GameScene::Render()
{
    // スプライトバッチの開始
    GetSpriteRenderer()->Begin();

    // 背景の描画
    if (m_backgroundSprite) {
        GetSpriteRenderer()->Draw(m_backgroundSprite);
    }

    // 敵の描画
    for (auto& enemySprite : m_enemySprites) {
        GetSpriteRenderer()->Draw(enemySprite);
    }

    // プレイヤーの描画
    if (m_playerSprite) {
        GetSpriteRenderer()->Draw(m_playerSprite);
    }

    // UIの描画
    if (m_titleSprite) {
        GetSpriteRenderer()->Draw(m_titleSprite);
    }

    if (m_scoreSprite) {
        GetSpriteRenderer()->Draw(m_scoreSprite);
    }

    // スプライトバッチの終了
    GetSpriteRenderer()->End();

    // 3Dモデルの描画（ここでは使用していない）
    // if (GetModelRenderer()->GetCamera()) {
    //     GetModelRenderer()->Begin();
    //     // 3Dモデルの描画処理...
    //     GetModelRenderer()->End();
    // }
}

void GameScene::OnActivated()
{
    Scene::OnActivated();

    // BGMの再生
    if (m_bgmSource) {
        m_bgmSource->Play();
    }
}

void GameScene::OnDeactivated()
{
    // BGMの停止
    if (m_bgmSource) {
        m_bgmSource->Stop();
    }

    Scene::OnDeactivated();
}

void GameScene::OnCollision(const CollisionEvent& event)
{
    // プレイヤーと敵の衝突判定
    if (event.type == CollisionEventType::Enter) {
        if (event.colliderA == m_playerCollider) {
            // プレイヤーと何かが衝突
            for (auto& enemyCollider : m_enemyColliders) {
                if (event.colliderB == enemyCollider) {
                    // プレイヤーと敵が衝突
                    m_gameOver = true;

                    // ヒット効果音の再生
                    if (m_seHitSource) {
                        m_seHitSource->Play();
                    }

                    break;
                }
            }
        }
        else if (event.colliderB == m_playerCollider) {
            // 何かとプレイヤーが衝突
            for (auto& enemyCollider : m_enemyColliders) {
                if (event.colliderA == enemyCollider) {
                    // 敵とプレイヤーが衝突
                    m_gameOver = true;

                    // ヒット効果音の再生
                    if (m_seHitSource) {
                        m_seHitSource->Play();
                    }

                    break;
                }
            }
        }
    }
}

void GameScene::ProcessInput(float deltaTime)
{
    InputManager* inputManager = GetInputManager();
    if (!inputManager) {
        return;
    }

    // キーボード入力によるプレイヤー移動
    DirectX::XMFLOAT2 moveDir(0.0f, 0.0f);

    if (inputManager->IsKeyDown('W') || inputManager->IsKeyDown(VK_UP)) {
        moveDir.y -= 1.0f;
    }

    if (inputManager->IsKeyDown('S') || inputManager->IsKeyDown(VK_DOWN)) {
        moveDir.y += 1.0f;
    }

    if (inputManager->IsKeyDown('A') || inputManager->IsKeyDown(VK_LEFT)) {
        moveDir.x -= 1.0f;
    }

    if (inputManager->IsKeyDown('D') || inputManager->IsKeyDown(VK_RIGHT)) {
        moveDir.x += 1.0f;
    }

    // 移動量の正規化
    float length = sqrtf(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
    if (length > 0.0f) {
        moveDir.x /= length;
        moveDir.y /= length;
    }

    // プレイヤーの位置を更新
    m_playerPosition.x += moveDir.x * m_playerSpeed * deltaTime;
    m_playerPosition.y += moveDir.y * m_playerSpeed * deltaTime;

    // 画面内に制限
    float screenWidth = static_cast<float>(GetEngine()->GetRenderer()->GetViewport().Width);
    float screenHeight = static_cast<float>(GetEngine()->GetRenderer()->GetViewport().Height);

    m_playerPosition.x = std::max(0.0f, std::min(m_playerPosition.x, screenWidth));
    m_playerPosition.y = std::max(0.0f, std::min(m_playerPosition.y, screenHeight));

    // スペースキーでジャンプ（例）
    if (inputManager->IsKeyPressed(VK_SPACE)) {
        // ジャンプ効果音の再生
        if (m_seJumpSource) {
            m_seJumpSource->Play();
        }

        // 物理シミュレーションを使ったジャンプ処理
        if (m_playerRigidbody) {
            m_playerRigidbody->ApplyImpulse(DirectX::XMFLOAT3(0.0f, -10.0f, 0.0f));
        }
    }

    // ESCキーでゲーム終了
    if (inputManager->IsKeyPressed(VK_ESCAPE)) {
        // ここでシーン遷移などの処理
        if (GetSceneManager()) {
            GetSceneManager()->ChangeScene("TitleScene");
        }
    }

    // ゲームパッド入力（例）
    if (inputManager->IsGamepadConnected()) {
        DirectX::XMFLOAT2 leftStick = inputManager->GetGamepadLeftStick();

        // 左スティックによるプレイヤー移動
        m_playerPosition.x += leftStick.x * m_playerSpeed * deltaTime;
        m_playerPosition.y += leftStick.y * m_playerSpeed * deltaTime;

        // Aボタンでジャンプ
        if (inputManager->IsGamepadButtonPressed(InputManager::GamepadButton::A)) {
            // ジャンプ効果音の再生
            if (m_seJumpSource) {
                m_seJumpSource->Play();
            }
        }
    }
}

bool GameScene::LoadAssets()
{
    ResourceManager* resourceManager = GetResourceManager();
    if (!resourceManager) {
        return false;
    }

    // プレイヤースプライトの作成
    m_playerSprite = GetSpriteRenderer()->CreateSprite(L"Assets/Textures/Player.png");
    if (!m_playerSprite) {
        return false;
    }

    // 背景スプライトの作成
    m_backgroundSprite = GetSpriteRenderer()->CreateSprite(L"Assets/Textures/Background.png");
    if (!m_backgroundSprite) {
        return false;
    }

    // タイトルスプライトの作成
    m_titleSprite = GetSpriteRenderer()->CreateSprite(L"Assets/Textures/Title.png");
    if (!m_titleSprite) {
        return false;
    }

    // スコアスプライトの作成
    m_scoreSprite = GetSpriteRenderer()->CreateSprite(L"Assets/Textures/Score.png");
    if (!m_scoreSprite) {
        return false;
    }

    // 敵スプライトの作成（5体の敵）
    for (int i = 0; i < 5; ++i) {
        std::shared_ptr<Sprite> enemySprite = GetSpriteRenderer()->CreateSprite(L"Assets/Textures/Enemy.png");
        if (!enemySprite) {
            return false;
        }

        m_enemySprites.push_back(enemySprite);
    }

    // サウンドの読み込み
    AudioSystem* audioSystem = GetAudioSystem();
    if (audioSystem) {
        // BGM
        m_bgmSource = audioSystem->CreateAudioSource(L"Assets/Sounds/BGM.wav");
        if (m_bgmSource) {
            m_bgmSource->SetLoop(true);
            m_bgmSource->SetVolume(0.5f);
        }

        // ヒット効果音
        m_seHitSource = audioSystem->CreateAudioSource(L"Assets/Sounds/Hit.wav");

        // ジャンプ効果音
        m_seJumpSource = audioSystem->CreateAudioSource(L"Assets/Sounds/Jump.wav");
    }

    return true;
}

void GameScene::InitializeObjects()
{
    // 画面サイズの取得
    float screenWidth = static_cast<float>(GetEngine()->GetRenderer()->GetViewport().Width);
    float screenHeight = static_cast<float>(GetEngine()->GetRenderer()->GetViewport().Height);

    // プレイヤーの初期設定
    m_playerPosition = DirectX::XMFLOAT2(screenWidth * 0.5f, screenHeight * 0.5f);
    m_playerSprite->position = m_playerPosition;
    m_playerSprite->size = DirectX::XMFLOAT2(64.0f, 64.0f);
    m_playerSprite->origin = DirectX::XMFLOAT2(0.5f, 0.5f);

    // プレイヤーのコライダー作成
    m_playerCollider = std::make_shared<SphereCollider>(32.0f);
    m_playerCollider->SetPosition(DirectX::XMFLOAT3(m_playerPosition.x, m_playerPosition.y, 0.0f));
    m_playerCollider->SetCategory(CollisionCategory::Player);
    m_playerCollider->SetMask(static_cast<CollisionCategory>(
        static_cast<int>(CollisionCategory::Enemy) |
        static_cast<int>(CollisionCategory::Projectile)
        ));

    // プレイヤーの剛体作成
    m_playerRigidbody = std::make_shared<Rigidbody>();
    m_playerRigidbody->SetMass(1.0f);
    m_playerRigidbody->SetCollider(m_playerCollider);

    // 物理システムに追加
    GetPhysicsSystem()->AddCollider(m_playerCollider);
    GetPhysicsSystem()->AddRigidbody(m_playerRigidbody);

    // 敵の初期設定
    for (size_t i = 0; i < m_enemySprites.size(); ++i) {
        // 敵の位置をランダムに設定
        DirectX::XMFLOAT2 position(
            static_cast<float>(rand() % static_cast<int>(screenWidth)),
            static_cast<float>(rand() % static_cast<int>(screenHeight))
        );

        // 敵の速度をランダムに設定
        DirectX::XMFLOAT2 velocity(
            (static_cast<float>(rand() % 200) - 100.0f),
            (static_cast<float>(rand() % 200) - 100.0f)
        );

        m_enemyPositions.push_back(position);
        m_enemyVelocities.push_back(velocity);

        // 敵スプライトの設定
        m_enemySprites[i]->position = position;
        m_enemySprites[i]->size = DirectX::XMFLOAT2(48.0f, 48.0f);
        m_enemySprites[i]->origin = DirectX::XMFLOAT2(0.5f, 0.5f);

        // 敵のコライダー作成
        std::shared_ptr<SphereCollider> enemyCollider = std::make_shared<SphereCollider>(24.0f);
        enemyCollider->SetPosition(DirectX::XMFLOAT3(position.x, position.y, 0.0f));
        enemyCollider->SetCategory(CollisionCategory::Enemy);
        enemyCollider->SetMask(static_cast<CollisionCategory>(
            static_cast<int>(CollisionCategory::Player) |
            static_cast<int>(CollisionCategory::Projectile)
            ));

        m_enemyColliders.push_back(enemyCollider);

        // 物理システムに追加
        GetPhysicsSystem()->AddCollider(enemyCollider);
    }

    // 背景スプライトの設定
    if (m_backgroundSprite) {
        m_backgroundSprite->position = DirectX::XMFLOAT2(screenWidth * 0.5f, screenHeight * 0.5f);
        m_backgroundSprite->size = DirectX::XMFLOAT2(screenWidth, screenHeight);
        m_backgroundSprite->origin = DirectX::XMFLOAT2(0.5f, 0.5f);
        m_backgroundSprite->depth = 1.0f; // 背景を一番後ろに描画
    }

    // タイトルスプライトの設定
    if (m_titleSprite) {
        m_titleSprite->position = DirectX::XMFLOAT2(screenWidth * 0.5f, 50.0f);
        m_titleSprite->size = DirectX::XMFLOAT2(400.0f, 100.0f);
        m_titleSprite->origin = DirectX::XMFLOAT2(0.5f, 0.5f);
    }

    // スコアスプライトの設定
    if (m_scoreSprite) {
        m_scoreSprite->position = DirectX::XMFLOAT2(100.0f, 50.0f);
        m_scoreSprite->size = DirectX::XMFLOAT2(200.0f, 50.0f);
        m_scoreSprite->origin = DirectX::XMFLOAT2(0.0f, 0.5f);
    }

    // 3Dカメラの設定
    if (m_camera) {
        m_camera->SetPosition(DirectX::XMFLOAT3(0.0f, 2.0f, -5.0f));
        m_camera->SetTarget(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));
        m_camera->SetUp(DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f));
        m_camera->SetAspectRatio(screenWidth / screenHeight);

        // モデルレンダラーにカメラを設定
        GetModelRenderer()->SetCamera(m_camera);
    }

    // ゲーム状態の初期化
    m_score = 0;
    m_gameOver = false;
    m_gameClear = false;
    m_timer = 0.0f;
}