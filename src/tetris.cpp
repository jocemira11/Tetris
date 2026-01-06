#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <string>
#include <cstdlib>
#include <iostream>
#include <algorithm>

// Classic Tetris minimal implementation

struct Piece {
    int type;
    int rotation;
    int x;
    int y;
};

struct Effect {
    sf::Vector2f pos;
    sf::Color color;
    float life;
};

enum GameState { MENU, PLAYING, GAME_OVER };

int main()
{
    const int screenWidth = 400;
    const int screenHeight = 520;

    const int fieldWidth = 10;
    const int fieldHeight = 20;
    const int blockSize = 24;
    const int offsetX = 50;
    const int offsetY = (screenHeight - fieldHeight * blockSize) / 2;

    std::vector<int> field(fieldWidth * fieldHeight, 0);

    // Tetromino definitions (4x4)
    std::string tetromino[11];
    tetromino[0] = "..X...X...X...X."; // I
    tetromino[1] = "..X..XX...X....."; // T
    tetromino[2] = ".X..XX..X......."; // S
    tetromino[3] = "..X..XX..X......"; // Z
    tetromino[4] = ".XX..XX........."; // O
    tetromino[5] = ".X...X...XX....."; // L
    tetromino[6] = "..X...X..XX....."; // J
    tetromino[7] = "..X..XX..X......"; // Frozen (same as Z for simplicity)
    tetromino[8] = ".XX..XX........."; // Electrical (same as O)
    tetromino[9] = "..X..XX...X....."; // Fire (same as T)
    tetromino[10] = "..X...X...X...X."; // Ghost (same as I)

    auto rotate = [](int px, int py, int r) {
        switch (r % 4) {
        case 0: return py * 4 + px;
        case 1: return 12 + py - (px * 4);
        case 2: return 15 - py * 4 - px;
        case 3: return 3 - py + (px * 4);
        }
        return 0;
    };

    auto doesPieceFit = [&](int tetrominoIndex, int rotation, int posX, int posY) {
        for (int px = 0; px < 4; px++)
            for (int py = 0; py < 4; py++) {
                int pi = rotate(px, py, rotation);
                int fi = (posY + py) * fieldWidth + (posX + px);

                if (posX + px >= 0 && posX + px < fieldWidth && posY + py >= 0 && posY + py < fieldHeight) {
                    if (tetromino[tetrominoIndex][pi] == 'X' && field[fi] != 0)
                        return false;
                } else {
                    if (tetromino[tetrominoIndex][pi] == 'X')
                        return false;
                }
            }
        return true;
    };

    sf::RenderWindow window(sf::VideoMode(screenWidth, screenHeight), "Tetris");
    window.setFramerateLimit(60);

    // Stars for space background
    std::vector<sf::Vector2f> stars;
    for (int i = 0; i < 50; i++) {
        stars.push_back(sf::Vector2f(rand() % screenWidth, rand() % screenHeight));
    }

    // Background music
    sf::Music music;
    if (music.openFromFile("assets/music/space.ogg")) {
        music.setLoop(true);
        music.play();
    }

    // block rectangles
    sf::RectangleShape block(sf::Vector2f(blockSize - 1, blockSize - 1));

    // Colors for pieces
    sf::Color colors[12] = {
        sf::Color::Black,
        sf::Color::Cyan,       // I
        sf::Color(128, 0, 128), // T
        sf::Color::Green,      // S
        sf::Color::Red,        // Z
        sf::Color::Yellow,     // O
        sf::Color(255, 165, 0), // L
        sf::Color::Blue,       // J
        sf::Color::Magenta,    // Frozen
        sf::Color::Yellow,     // Electrical
        sf::Color::Red,        // Fire
        sf::Color::Green       // Ghost
    };

    // Load font
    sf::Font font;
    bool fontLoaded = font.loadFromFile("assets/fonts/Minecraft.ttf");
    if (!fontLoaded) {
        std::cerr << "Warning: font not loaded. Expected assets/fonts/Minecraft.ttf\n";
    }

    // Button for menu and game over
    sf::RectangleShape button(sf::Vector2f(100, 40));
    button.setFillColor(sf::Color(0, 100, 200)); // Blue
    button.setOutlineColor(sf::Color::Cyan);
    button.setOutlineThickness(2);

    sf::Text buttonText;
    if (fontLoaded) {
        buttonText.setFont(font);
        buttonText.setCharacterSize(20);
        buttonText.setFillColor(sf::Color::White);
    }

    // Pause button
    sf::RectangleShape pauseButton(sf::Vector2f(80, 30));
    pauseButton.setFillColor(sf::Color(0, 150, 255)); // Light blue
    pauseButton.setOutlineColor(sf::Color::Cyan);
    pauseButton.setOutlineThickness(2);

    sf::Text pauseText;
    if (fontLoaded) {
        pauseText.setFont(font);
        pauseText.setCharacterSize(18);
        pauseText.setFillColor(sf::Color::White);
    }

    // Game variables
    GameState state = MENU;
    Piece currentPiece;
    int score = 0;
    int linesCleared = 0;
    int level = 1;
    float speed = 0.5f;
    float speedCounter = 0.0f;
    bool isPaused = false;
    std::vector<Effect> effects;

    // Special pieces
    int pieceCounter = 0;
    bool isFrozen = false;
    float freezeTimer = 0.0f;
    const float freezeDuration = 3.0f; // seconds
    int ghostShadowY = 0;

    // Input timing
    float moveDelay = 0.12f;
    float moveTimer = 0.0f;
    bool rotatePrev = false;
    bool spacePrev = false;

    sf::Clock clock;

    auto resetGame = [&]() {
        field.assign(fieldWidth * fieldHeight, 0);
        currentPiece.type = rand() % 7;
        currentPiece.rotation = 0;
        currentPiece.x = fieldWidth / 2 - 2;
        currentPiece.y = 0;
        score = 0;
        linesCleared = 0;
        level = 1;
        speedCounter = 0.0f;
        pieceCounter = 0;
        isFrozen = false;
        freezeTimer = 0.0f;
        isPaused = false;
        effects.clear();
        state = PLAYING;
    };

    srand((unsigned)time(NULL));

    // Game loop
    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        speedCounter += deltaTime;
        moveTimer += deltaTime;

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) window.close();

            // Mouse click for buttons
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                if (state == MENU) {
                    sf::FloatRect buttonBounds = button.getGlobalBounds();
                    if (buttonBounds.contains(mousePos.x, mousePos.y)) {
                        resetGame();
                    }
                } else if (state == GAME_OVER) {
                    sf::FloatRect buttonBounds = button.getGlobalBounds();
                    if (buttonBounds.contains(mousePos.x, mousePos.y)) {
                        resetGame();
                    }
                } else if (state == PLAYING) {
                    sf::FloatRect pauseBounds = pauseButton.getGlobalBounds();
                    if (pauseBounds.contains(mousePos.x, mousePos.y)) {
                        isPaused = !isPaused;
                    }
                }
            }
        }

        // Handle freeze
        if (state == PLAYING && isFrozen) {
            freezeTimer -= deltaTime;
            if (freezeTimer <= 0) {
                isFrozen = false;
            }
        }

        // Update effects
        for (auto& e : effects) {
            e.life -= deltaTime;
        }
        effects.erase(std::remove_if(effects.begin(), effects.end(), [](const Effect& e) { return e.life <= 0; }), effects.end());

        // Calculate ghost shadow
        if (state == PLAYING && currentPiece.type == 10) { // Ghost
            ghostShadowY = currentPiece.y;
            while (doesPieceFit(currentPiece.type, currentPiece.rotation, currentPiece.x, ghostShadowY + 1)) {
                ghostShadowY++;
            }
        }

        // Real-time input handling (allows holding keys)
        if (state == PLAYING && !isFrozen && !isPaused) {
            if (moveTimer >= moveDelay) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                    if (doesPieceFit(currentPiece.type, currentPiece.rotation, currentPiece.x - 1, currentPiece.y)) currentPiece.x -= 1;
                    moveTimer = 0.0f;
                } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
                    if (doesPieceFit(currentPiece.type, currentPiece.rotation, currentPiece.x + 1, currentPiece.y)) currentPiece.x += 1;
                    moveTimer = 0.0f;
                }
            }

            // Rotation: detect edge (press)
            bool rotateNow = sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
            if (rotateNow && !rotatePrev) {
                if (doesPieceFit(currentPiece.type, currentPiece.rotation + 1, currentPiece.x, currentPiece.y)) currentPiece.rotation += 1;
            }
            rotatePrev = rotateNow;

            // Soft drop (hold Down)
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
                if (doesPieceFit(currentPiece.type, currentPiece.rotation, currentPiece.x, currentPiece.y + 1)) currentPiece.y += 1;
            }

            // Hard drop (Space) detect edge
            bool spaceNow = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);
            if (spaceNow && !spacePrev) {
                while (doesPieceFit(currentPiece.type, currentPiece.rotation, currentPiece.x, currentPiece.y + 1)) currentPiece.y++;
                speedCounter = speed; // force lock next update
            }
            spacePrev = spaceNow;
        }

        // Gravity
        if (state == PLAYING && !isFrozen && !isPaused && speedCounter >= speed) {
            if (doesPieceFit(currentPiece.type, currentPiece.rotation, currentPiece.x, currentPiece.y + 1)) {
                currentPiece.y += 1;
            } else {
                // Lock piece
                for (int px = 0; px < 4; px++) {
                    for (int py = 0; py < 4; py++) {
                        int pi = rotate(px, py, currentPiece.rotation);
                        if (tetromino[currentPiece.type][pi] == 'X') {
                            int fi = (currentPiece.y + py) * fieldWidth + (currentPiece.x + px);
                            if (fi >= 0 && fi < (int)field.size())
                                field[fi] = currentPiece.type + 1;
                        }
                    }
                }

                // Special effects
                if (currentPiece.type == 7) { // Frozen
                    isFrozen = true;
                    freezeTimer = freezeDuration;
                } else if (currentPiece.type == 8) { // Electrical
                    // Clear two rows where the piece landed
                    int row1 = currentPiece.y;
                    int row2 = currentPiece.y - 1;
                    if (row1 >= 0 && row1 < fieldHeight) {
                        for (int x = 0; x < fieldWidth; x++) {
                            field[row1 * fieldWidth + x] = 0;
                        }
                        score += 100; // Points for clearing a row
                        // Add spark effects
                        for (int x = 0; x < fieldWidth; x++) {
                            effects.push_back({{x * blockSize + offsetX + blockSize / 2, row1 * blockSize + offsetY + blockSize / 2}, sf::Color::Yellow, 0.5f});
                        }
                    }
                    if (row2 >= 0 && row2 < fieldHeight) {
                        for (int x = 0; x < fieldWidth; x++) {
                            field[row2 * fieldWidth + x] = 0;
                        }
                        score += 100; // Points for clearing a row
                        // Add spark effects
                        for (int x = 0; x < fieldWidth; x++) {
                            effects.push_back({{x * blockSize + offsetX + blockSize / 2, row2 * blockSize + offsetY + blockSize / 2}, sf::Color::Yellow, 0.5f});
                        }
                    }
                } else if (currentPiece.type == 9) { // Fire
                    // Explode blocks around the piece
                    int blocksCleared = 0;
                    for (int ex = -2; ex <= 2; ex++) {
                        for (int ey = -2; ey <= 2; ey++) {
                            int nx = currentPiece.x + ex;
                            int ny = currentPiece.y + ey;
                            if (nx >= 0 && nx < fieldWidth && ny >= 0 && ny < fieldHeight && field[ny * fieldWidth + nx] != 0) {
                                field[ny * fieldWidth + nx] = 0;
                                blocksCleared++;
                                // Add fire effects
                                effects.push_back({{nx * blockSize + offsetX + blockSize / 2, ny * blockSize + offsetY + blockSize / 2}, sf::Color::Red, 0.5f});
                            }
                        }
                    }
                    score += blocksCleared * 10; // Points for each block cleared
                }

                // Check lines
                for (int py = 0; py < 4; py++) {
                    int y = currentPiece.y + py;
                    if (y >= 0 && y < fieldHeight) {
                        bool line = true;
                        for (int x = 0; x < fieldWidth; x++) {
                            if (field[y * fieldWidth + x] == 0) { line = false; break; }
                        }
                        if (line) {
                            // remove line
                            for (int ty = y; ty > 0; ty--) {
                                for (int x = 0; x < fieldWidth; x++) {
                                    field[ty * fieldWidth + x] = field[(ty - 1) * fieldWidth + x];
                                }
                            }
                            for (int x = 0; x < fieldWidth; x++) {
                                field[x] = 0;
                            }
                            score += 100;
                            linesCleared++;
                            level = linesCleared / 10 + 1;
                            speed = 0.5f / (level * 0.5f + 0.5f);
                            // Add line clear effects
                            for (int x = 0; x < fieldWidth; x++) {
                                effects.push_back({{x * blockSize + offsetX + blockSize / 2, y * blockSize + offsetY + blockSize / 2}, sf::Color::White, 0.5f});
                            }                    }
                }            }
                // Next piece
                pieceCounter++;
                if (pieceCounter % 3 == 0) {
                    // Special piece
                    currentPiece.type = 7 + (rand() % 4); // 7-10
                } else {
                    currentPiece.type = rand() % 7; // 0-6
                }
                currentPiece.rotation = 0;
                currentPiece.x = fieldWidth / 2 - 2;
                currentPiece.y = 0;

                if (!doesPieceFit(currentPiece.type, currentPiece.rotation, currentPiece.x, currentPiece.y)) state = GAME_OVER;
            }

            speedCounter = 0.0f;
        }

        // Render
        window.clear(sf::Color(0, 0, 20)); // Dark blue space background

        // Draw stars
        for (auto& star : stars) {
            sf::CircleShape s(1);
            s.setPosition(star);
            s.setFillColor(sf::Color::White);
            window.draw(s);
        }

        if (state == MENU) {
            if (fontLoaded) {
                sf::Text title("TETRIS", font, 40);
                title.setPosition(100, 80);
                title.setFillColor(sf::Color::Cyan);
                window.draw(title);

                sf::Text controls("Controls:\nA/D or Left/Right: Move\nS or Down: Soft Drop\nW or Up: Rotate\nSpace: Hard Drop\nP: Pause\n\nSpecial Pieces:\nFrozen (Magenta): Freezes time briefly\nElectrical (Yellow): Clears random lines\nFire (Red): Explodes nearby blocks\nGhost (Green): Passes through blocks", font, 20);
                controls.setPosition(20, 180);
                controls.setFillColor(sf::Color(200, 200, 255)); // Light blue
                window.draw(controls);

                button.setPosition(150, 450);
                buttonText.setString("Start");
                buttonText.setPosition(170, 460);
                window.draw(button);
                window.draw(buttonText);
            }
        } else {
            // Draw field
            for (int x = 0; x < fieldWidth; x++) {
                for (int y = 0; y < fieldHeight; y++) {
                    if (field[y * fieldWidth + x] != 0) {
                        sf::RectangleShape block(sf::Vector2f(blockSize, blockSize));
                        block.setPosition(x * blockSize + offsetX, y * blockSize + offsetY);
                        block.setFillColor(colors[field[y * fieldWidth + x]]);
                        window.draw(block);
                    }
                }
            }

            if (state == PLAYING) {
                // Draw current piece
                for (int px = 0; px < 4; px++) {
                    for (int py = 0; py < 4; py++) {
                        int pi = rotate(px, py, currentPiece.rotation);
                        if (tetromino[currentPiece.type][pi] == 'X') {
                            int x = currentPiece.x + px;
                            int y = currentPiece.y + py;
                            sf::RectangleShape block(sf::Vector2f(blockSize, blockSize));
                            block.setPosition(x * blockSize + offsetX, y * blockSize + offsetY);
                            block.setFillColor(colors[currentPiece.type + 1]);
                            window.draw(block);
                        }
                    }
                }

                // Draw ghost piece
                if (currentPiece.type == 10) { // Ghost
                    for (int px = 0; px < 4; px++) {
                        for (int py = 0; py < 4; py++) {
                            int pi = rotate(px, py, currentPiece.rotation);
                            if (tetromino[currentPiece.type][pi] == 'X') {
                                int x = currentPiece.x + px;
                                int y = ghostShadowY + py;
                                sf::RectangleShape block(sf::Vector2f(blockSize, blockSize));
                                block.setPosition(x * blockSize + offsetX, y * blockSize + offsetY);
                                block.setFillColor(sf::Color(255, 255, 255, 100)); // Semi-transparent white
                                window.draw(block);
                            }
                        }
                    }
                }

                // Draw pause button
                pauseButton.setPosition(300, 200);
                pauseText.setString(isPaused ? "Resume" : "Pause");
                pauseText.setPosition(305, 205);
                window.draw(pauseButton);
                window.draw(pauseText);

                // Draw paused text
                if (isPaused) {
                    sf::Text pausedText("PAUSED", font, 50);
                    pausedText.setPosition(150, 200);
                    pausedText.setFillColor(sf::Color::Yellow);
                    window.draw(pausedText);
                }
            }

            // Draw effects
            for (const auto& e : effects) {
                sf::CircleShape c(3);
                c.setPosition(e.pos - sf::Vector2f(3, 3));
                c.setFillColor(e.color);
                window.draw(c);
            }

            // Draw border
            sf::RectangleShape border(sf::Vector2f(fieldWidth * blockSize, fieldHeight * blockSize));
            border.setPosition(offsetX, offsetY);
            border.setFillColor(sf::Color::Transparent);
            border.setOutlineThickness(2);
            border.setOutlineColor(sf::Color::White);
            window.draw(border);

            // Draw grid
            for (int i = 1; i < fieldWidth; i++) {
                sf::RectangleShape line(sf::Vector2f(1, fieldHeight * blockSize));
                line.setPosition(i * blockSize + offsetX, offsetY);
                line.setFillColor(sf::Color(100, 100, 100));
                window.draw(line);
            }
            for (int i = 1; i < fieldHeight; i++) {
                sf::RectangleShape line(sf::Vector2f(fieldWidth * blockSize, 1));
                line.setPosition(offsetX, i * blockSize + offsetY);
                line.setFillColor(sf::Color(100, 100, 100));
                window.draw(line);
            }

            // Draw score
            if (fontLoaded) {
                sf::Text scoreText("Score: " + std::to_string(score), font, 20);
                scoreText.setPosition(300, 50);
                scoreText.setFillColor(sf::Color::White);
                window.draw(scoreText);

                sf::Text linesText("Lines: " + std::to_string(linesCleared), font, 20);
                linesText.setPosition(300, 80);
                linesText.setFillColor(sf::Color::White);
                window.draw(linesText);

                sf::Text levelText("Level: " + std::to_string(linesCleared / 10 + 1), font, 20);
                levelText.setPosition(300, 110);
                levelText.setFillColor(sf::Color::White);
                window.draw(levelText);

                if (state == PLAYING && currentPiece.type >= 7) {
                    std::string name;
                    if (currentPiece.type == 7) name = "Frozen";
                    else if (currentPiece.type == 8) name = "Electrical";
                    else if (currentPiece.type == 9) name = "Fire";
                    else name = "Ghost";
                    sf::Text specialLabel("Special:", font, 20);
                    specialLabel.setPosition(300, 140);
                    specialLabel.setFillColor(sf::Color::White);
                    window.draw(specialLabel);
                    sf::Text specialName(name, font, 20);
                    specialName.setPosition(300, 170);
                    specialName.setFillColor(sf::Color::White);
                    window.draw(specialName);
                }
            }

            if (state == GAME_OVER) {
                if (fontLoaded) {
                    sf::Text gameOverText("GAME OVER", font, 40);
                    gameOverText.setPosition(125, 250);
                    gameOverText.setFillColor(sf::Color::Red);
                    window.draw(gameOverText);

                    button.setPosition(150, 350);
                    buttonText.setString("Restart");
                    buttonText.setPosition(160, 360);
                    window.draw(button);
                    window.draw(buttonText);
                }
            }
        }

        window.display();
    }

    return 0;
}

