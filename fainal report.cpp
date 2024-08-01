#include <iostream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <conio.h>
#include <windows.h>
#include <iterator>

static const int FIELD_WIDTH = 30;
static const int FIELD_HEIGHT = 30;
static const int FPS = 30; // フレームレート (FPS)
static const int ITEM_SPAWN_INTERVAL = 50; // アイテム生成のフレーム間隔

class GameObject {
public:
    int x, y;
    char shape;
    int color;

    GameObject(int startX, int startY, char startShape, int startColor)
        : x(startX), y(startY), shape(startShape), color(startColor) {}

    virtual void update() = 0;
    virtual bool isOffScreen() const {
        return (y >= FIELD_HEIGHT);
    }
    virtual bool isActive() const = 0;
    virtual void setActive(bool active) = 0;
};

class Player : public GameObject {
public:
    Player(int startX, int startY)
        : GameObject(startX, startY, 'A', 7) {} // 白色

    void update() override {
        if (_kbhit()) {
            switch (_getch()) {
            case 'w': y = (y > 0) ? y - 1 : y; break;
            case 's': y = (y < FIELD_HEIGHT - 1) ? y + 1 : y; break;
            case 'a': x = (x > 0) ? x - 1 : x; break;
            case 'd': x = (x < FIELD_WIDTH - 1) ? x + 1 : x; break;
            }
        }
    }

    bool isActive() const override { return true; }
    void setActive(bool active) override {}
};

class Enemy : public GameObject {
public:
    Enemy(int startX, int startY)
        : GameObject(startX, startY, 'E', 4) {} // 赤色

    void update() override {
        y += 1;
        if (y >= FIELD_HEIGHT) {
            y = 0;
            x = rand() % FIELD_WIDTH;
        }
    }

    bool isActive() const override { return true; }
    void setActive(bool active) override {}
};

class Item : public GameObject {
private:
    int updateInterval;  // 更新間隔
    int updateCounter;   // カウンタ
    bool active;         // アイテムのアクティブ状態

public:
    Item(int startX, int startY, int interval = 10)
        : GameObject(startX, startY, 'I', 14), updateInterval(interval), updateCounter(0), active(true) {} // 青色

    void update() override {
        if (!active) return;

        updateCounter++;

        // 更新間隔に達したら位置を更新
        if (updateCounter >= updateInterval) {
            y += 1;
            if (y >= FIELD_HEIGHT) {
                y = FIELD_HEIGHT; // 画面下に固定
            }
            updateCounter = 0; // カウンタリセット
        }
    }

    bool isOffScreen() const override {
        return (y >= FIELD_HEIGHT);
    }

    bool isActive() const override { return active; }
    void setActive(bool newActive) override { active = newActive; }
};

class Game {
    std::vector<GameObject*> objects;
    char field[FIELD_HEIGHT][FIELD_WIDTH];
    int itemSpawnCounter; // アイテム生成カウンタ
    bool gameOverFlag;    // ゲームオーバーフラグ
    int score;            // スコア

    void clearField() {
        for (int i = 0; i < FIELD_HEIGHT; ++i) {
            for (int j = 0; j < FIELD_WIDTH; ++j) {
                field[i][j] = ' ';
            }
        }
    }

    void drawField() {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        COORD cursorPos;

        for (int i = 0; i < FIELD_HEIGHT; ++i) {
            cursorPos.X = 0;
            cursorPos.Y = i;
            SetConsoleCursorPosition(hConsole, cursorPos);

            for (int j = 0; j < FIELD_WIDTH; ++j) {
                bool isColored = false;
                for (auto it = objects.begin(); it != objects.end(); ++it) {
                    GameObject* obj = *it;
                    if (obj->isActive() && obj->y == i && obj->x == j) {
                        SetConsoleTextAttribute(hConsole, obj->color);
                        std::cout << obj->shape;
                        isColored = true;
                        break;
                    }
                }
                if (!isColored) {
                    SetConsoleTextAttribute(hConsole, 7); // デフォルトの色に戻す
                    std::cout << field[i][j];
                }
            }
        }

        SetConsoleTextAttribute(hConsole, 7); // デフォルトの色に戻す
    }

    void spawnItem() {
        objects.push_back(new Item(rand() % FIELD_WIDTH, 0)); // 新しいアイテムを画面上部に生成
    }

public:
    Game() : itemSpawnCounter(0), gameOverFlag(false), score(0) {
        srand(static_cast<unsigned>(time(0)));
        objects.push_back(new Player(FIELD_WIDTH / 2, FIELD_HEIGHT - 1));
        for (int i = 0; i < 5; ++i) {
            objects.push_back(new Enemy(rand() % FIELD_WIDTH, rand() % FIELD_HEIGHT));
        }
        spawnItem(); // 初期のアイテム生成
    }

    ~Game() {
        for (auto obj : objects) {
            delete obj;
        }
    }

    void update() {
        for (auto it = objects.begin(); it != objects.end(); ++it) {
            (*it)->update();
        }

        // プレイヤーとアイテムの衝突をチェック
        Player* player = dynamic_cast<Player*>(objects[0]);
        for (auto it = objects.begin(); it != objects.end(); ++it) {
            if (auto item = dynamic_cast<Item*>(*it)) {
                if (item->isActive() && player->x == item->x && player->y == item->y) {
                    item->setActive(false); // アイテムが消える
                    score++; // スコアを加算
                }
            }
        }

        // アイテムの生成カウンタを更新
        itemSpawnCounter++;
        if (itemSpawnCounter >= ITEM_SPAWN_INTERVAL) {
            itemSpawnCounter = 0;
            spawnItem(); // 新しいアイテムを生成
        }

        // ゲームオーバー状態のチェック
        gameOverFlag = isGameOver();
    }

    bool isGameOver() {
        bool itemOffScreen = false;

        for (auto it = objects.begin(); it != objects.end(); ++it) {
            GameObject* obj = *it;
            if (!obj->isActive()) continue;

            // プレイヤーと敵が同じ位置にいるかチェック
            Player* player = dynamic_cast<Player*>(objects[0]);
            if (player && player->x == obj->x && player->y == obj->y) {
                if (dynamic_cast<Enemy*>(obj)) {
                    return true; // プレイヤーが敵に当たった場合
                }
            }

            // アイテムが画面下に達したかチェック
            if (auto item = dynamic_cast<Item*>(obj)) {
                if (item->isOffScreen()) {
                    itemOffScreen = true;
                }
            }
        }
        return itemOffScreen;
    }

    void run() {
        DWORD frameTime = 1000 / FPS;
        DWORD lastFrame = GetTickCount();

        while (!gameOverFlag) {
            DWORD currentTime = GetTickCount();
            if (currentTime - lastFrame >= frameTime) {
                lastFrame = currentTime;
                update();
                clearField();
                drawField();
            }
        }

        // ゲームオーバー表示
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        COORD cursorPos = { 0, FIELD_HEIGHT };
        SetConsoleCursorPosition(hConsole, cursorPos);
        SetConsoleTextAttribute(hConsole, 7); // デフォルトの色に戻す
        std::cout << "Game Over! Score: " << score << std::endl;
        Sleep(2000); // 2秒待機してから終了
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}