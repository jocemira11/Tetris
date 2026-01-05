#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <cstdlib>
#include <iostream>

// Classic Tetris minimal implementation

enum GameState { MENU, PLAYING, GAME_OVER };

int main()
{
    const int screenWidth = 400;
    const int screenHeight = 520;

    const int fieldWidth = 10;
    const int fieldHeight = 20;
    const int blockSize = 24;

    std::vector<int> field(fieldWidth * fieldHeight, 0);

    // Tetromino definitions (4x4)
    std::string tetromino[7];
    tetromino[0] = "..X...X...X...X."; // I
    tetromino[1] = "..X..XX...X....."; // T
    tetromino[2] = ".X..XX..X......."; // S
    tetromino[3] = "..X..XX..X......"; // Z
    tetromino[4] = ".XX..XX........."; // O
    tetromino[5] = ".X...X...XX....."; // L
    tetromino[6] = "..X...X..XX....."; // J

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

    // block rectangles
    sf::RectangleShape block(sf::Vector2f(blockSize - 1, blockSize - 1));

    // Colors for pieces
    sf::Color colors[8] = {
        sf::Color::Black,
        sf::Color::Cyan,
        sf::Color(128, 0, 128),
        sf::Color::Green,
        sf::Color::Red,
        sf::Color::Yellow,
        sf::Color(255, 165, 0),
        sf::Color::Blue
    };

    // Load font
    sf::Font font;
    bool fontLoaded = font.loadFromFile("assets/fonts/Minecraft.ttf");
    if (!fontLoaded) {
        std::cerr << "Warning: font not loaded. Expected assets/fonts/Minecraft.ttf\n";
    }

    // Button for menu and game over
    sf::RectangleShape button(sf::Vector2f(100, 40));
    button.setFillColor(sf::Color::Green);
    button.setOutlineColor(sf::Color::White);
    button.setOutlineThickness(2);

    sf::Text buttonText;
    if (fontLoaded) {
        buttonText.setFont(font);
        buttonText.setCharacterSize(20);
        buttonText.setFillColor(sf::Color::White);
    }

    // Game variables
    GameState state = MENU;
    int currentPiece = 0;
    int currentRotation = 0;
    int currentX = fieldWidth / 2 - 2;
    int currentY = 0;
    int score = 0;
    float speed = 0.5f;
    float speedCounter = 0.0f;

    // Input timing
    float moveDelay = 0.12f;
    float moveTimer = 0.0f;
    bool rotatePrev = false;
    bool spacePrev = false;

    sf::Clock clock;

    auto resetGame = [&]() {
        field.assign(fieldWidth * fieldHeight, 0);
        currentPiece = rand() % 7;
        currentRotation = 0;
        currentX = fieldWidth / 2 - 2;
        currentY = 0;
        score = 0;
        speedCounter = 0.0f;
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
                    // Check if click on Play button
                    sf::FloatRect buttonBounds = button.getGlobalBounds();
                    if (buttonBounds.contains(mousePos.x, mousePos.y)) {
                        resetGame();
                    }
                } else if (state == GAME_OVER) {
                    // Check if click on Restart button
                    sf::FloatRect buttonBounds = button.getGlobalBounds();
                    if (buttonBounds.contains(mousePos.x, mousePos.y)) {
                        resetGame();
                    }
                }
            }
        }

        if (state == PLAYING) {
            // Real-time input handling (allows holding keys)
            if (moveTimer >= moveDelay) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                    if (doesPieceFit(currentPiece, currentRotation, currentX - 1, currentY)) currentX -= 1;
                    moveTimer = 0.0f;
                } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
                    if (doesPieceFit(currentPiece, currentRotation, currentX + 1, currentY)) currentX += 1;
                    moveTimer = 0.0f;
                }
            }

            // Rotation: detect edge (press)
            bool rotateNow = sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
            if (rotateNow && !rotatePrev) {
                if (doesPieceFit(currentPiece, currentRotation + 1, currentX, currentY)) currentRotation += 1;
            }
            rotatePrev = rotateNow;

            // Soft drop (hold Down)
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
                if (doesPieceFit(currentPiece, currentRotation, currentX, currentY + 1)) currentY += 1;
            }

            // Hard drop (Space) detect edge
            bool spaceNow = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);
            if (spaceNow && !spacePrev) {
                while (doesPieceFit(currentPiece, currentRotation, currentX, currentY + 1)) currentY++;
                speedCounter = speed; // force lock next update
            }
            spacePrev = spaceNow;

            // Gravity
            if (speedCounter >= speed) {
                if (doesPieceFit(currentPiece, currentRotation, currentX, currentY + 1)) {
                    currentY += 1;
                } else {
                    // Lock piece
                    for (int px = 0; px < 4; px++)
                        for (int py = 0; py < 4; py++) {
                            int pi = rotate(px, py, currentRotation);
                            if (tetromino[currentPiece][pi] == 'X') {
                                int fi = (currentY + py) * fieldWidth + (currentX + px);
                                if (fi >= 0 && fi < (int)field.size())
                                    field[fi] = currentPiece + 1;
                            }
                        }

                    // Check lines
                    for (int py = 0; py < 4; py++) {
                        int y = currentY + py;
                        if (y >= 0 && y < fieldHeight) {
                            bool line = true;
                            for (int x = 0; x < fieldWidth; x++)
                                if (field[y * fieldWidth + x] == 0) { line = false; break; }
                            if (line) {
                                // remove line
                                for (int ty = y; ty > 0; ty--)
                                    for (int x = 0; x < fieldWidth; x++)
                                        field[ty * fieldWidth + x] = field[(ty - 1) * fieldWidth + x];
                                for (int x = 0; x < fieldWidth; x++)
                                    field[x] = 0;
                                score += 100;
                            }
                        }
                    }

                    // Next piece
                    currentPiece = rand() % 7;
                    currentRotation = 0;
                    currentX = fieldWidth / 2 - 2;
                    currentY = 0;

                    if (!doesPieceFit(currentPiece, currentRotation, currentX, currentY)) state = GAME_OVER;
                }

                speedCounter = 0.0f;
            }
        }

        // Render
        window.clear(sf::Color(50, 50, 50));

        if (state == MENU) {
            // Draw menu
            if (fontLoaded) {
                sf::Text title("TETRIS", font, 40);
                title.setFillColor(sf::Color::White);
                title.setPosition(screenWidth / 2 - 80, 50);
                window.draw(title);

                sf::Text controls("Controles:\nIzq/Der: mover\nArriba: rotar\nAbajo: soft drop\nEspacio: hard drop\nEsc: salir", font, 18);
                controls.setFillColor(sf::Color::White);
                controls.setPosition(50, 150);
                window.draw(controls);

                button.setPosition(screenWidth / 2 - 50, 350);
                buttonText.setString("PLAY");
                buttonText.setPosition(screenWidth / 2 - 25, 360);
                window.draw(button);
                window.draw(buttonText);
            }
        } else if (state == PLAYING || state == GAME_OVER) {
            // Draw field
            for (int x = 0; x < fieldWidth; x++)
                for (int y = 0; y < fieldHeight; y++) {
                    int v = field[y * fieldWidth + x];
                    block.setPosition(20 + x * blockSize, 20 + y * blockSize);
                    block.setFillColor(colors[v]);
                    window.draw(block);
                }

            if (state == PLAYING) {
                // Draw current piece
                for (int px = 0; px < 4; px++)
                    for (int py = 0; py < 4; py++) {
                        int pi = rotate(px, py, currentRotation);
                        if (tetromino[currentPiece][pi] == 'X') {
                            block.setPosition(20 + (currentX + px) * blockSize, 20 + (currentY + py) * blockSize);
                            block.setFillColor(colors[currentPiece + 1]);
                            window.draw(block);
                        }
                    }
            }

            // Draw grid border
            sf::RectangleShape border(sf::Vector2f(fieldWidth * blockSize + 2, fieldHeight * blockSize + 2));
            border.setPosition(19, 19);
            border.setFillColor(sf::Color::Transparent);
            border.setOutlineThickness(1);
            border.setOutlineColor(sf::Color::White);
            window.draw(border);

            // Draw score
            if (fontLoaded) {
                sf::Text text;
                text.setFont(font);
                text.setCharacterSize(18);
                text.setFillColor(sf::Color::White);
                text.setString("Score: " + std::to_string(score));
                text.setPosition(20 + fieldWidth * blockSize + 10, 40);
                window.draw(text);
            }

            if (state == GAME_OVER) {
                if (fontLoaded) {
                    sf::Text go;
                    go.setFont(font);
                    go.setCharacterSize(28);
                    go.setFillColor(sf::Color::Red);
                    go.setString("GAME OVER");
                    go.setPosition(40, screenHeight / 2 - 50);
                    window.draw(go);

                    button.setPosition(screenWidth / 2 - 50, screenHeight / 2 + 20);
                    buttonText.setString("RESTART");
                    buttonText.setPosition(screenWidth / 2 - 40, screenHeight / 2 + 30);
                    window.draw(button);
                    window.draw(buttonText);
                }
            }
        }

        window.display();
    }

    return 0;
}
