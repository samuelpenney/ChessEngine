#include "Game.h"
#include <SFML/Graphics.hpp>
#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace {

constexpr float TILE = 80.f * 0.9f;
constexpr float BOARD_SIZE = TILE * 8;
constexpr unsigned WIN_W = 800;
constexpr unsigned WIN_H = 800;
constexpr float BOARD_X = (WIN_W - BOARD_SIZE) / 2.f;
constexpr float BOARD_Y = 70.f;
constexpr float BAR_TOP = BOARD_Y + BOARD_SIZE;
constexpr float BAR_HEIGHT = WIN_H - BAR_TOP;
constexpr float BAR_BTN_Y = BAR_TOP + (BAR_HEIGHT - 44.f) / 2.f;

const sf::Color BG(38, 37, 34);
const sf::Color LIGHT_SQ(240, 217, 181);
const sf::Color DARK_SQ(181, 136, 99);
const sf::Color SELECT_COLOR(246, 246, 105, 170);
const sf::Color LAST_MOVE_COLOR(205, 210, 106, 130);
const sf::Color CHECK_COLOR(220, 50, 50, 180);
const sf::Color HINT_COLOR(0, 0, 0, 60);
const sf::Color BTN_COLOR(70, 70, 78);
const sf::Color BTN_HOVER(98, 98, 110);
const sf::Color TEXT_COLOR(235, 235, 235);
const sf::Color MUTED_TEXT(150, 150, 150);

const PieceType PROMO_CHOICES[4] = {PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight};

enum class Screen { Menu, Playing };

struct Button {
    sf::FloatRect rect;
    std::string label;
};

struct GuiState {
    Game game;
    Screen screen = Screen::Menu;
    int selected = -1;
    std::vector<Move> selectedMoves;
    int lastFrom = -1, lastTo = -1;
    int promoFrom = -1, promoTo = -1;
    GameOverStatus status = GameOverStatus::Playing;
};

constexpr float BANNER_Y = BOARD_Y + (BOARD_SIZE - 150.f) / 2.f;

const Button onePlayerBtn{sf::FloatRect({(WIN_W - 240.f) / 2.f, 340.f}, {240.f, 60.f}), "1 Player"};
const Button twoPlayerBtn{sf::FloatRect({(WIN_W - 240.f) / 2.f, 440.f}, {240.f, 60.f}), "2 Players"};
const Button quitBtn{sf::FloatRect({(WIN_W - 240.f) / 2.f, 540.f}, {240.f, 60.f}), "Quit"};
const Button menuBtn{sf::FloatRect({BOARD_X + BOARD_SIZE - 105.f, BAR_BTN_Y}, {105.f, 44.f}), "Menu"};
const Button resetBtn{sf::FloatRect({BOARD_X + BOARD_SIZE - 224.f, BAR_BTN_Y}, {105.f, 44.f}), "Reset"};
const Button playAgainBtn{sf::FloatRect({(WIN_W - 150.f) / 2.f, BANNER_Y + 90.f}, {150.f, 44.f}), "Play Again"};

bool isGameOver(GameOverStatus s) {
    return s == GameOverStatus::Checkmate || s == GameOverStatus::Stalemate ||
           s == GameOverStatus::InsufficientMaterial;
}

// Lets the game launch from the project root, bin/, or src/.
std::string findAsset(const std::string &relPath) {
    for (const char *prefix : {"src/", "../src/", ""}) {
        std::string path = prefix + relPath;
        if (std::filesystem::exists(path)) return path;
    }
    return "src/" + relPath;
}

bool loadFont(sf::Font &font) {
    std::string path = findAsset("font/JetBrainsMono-2.304/fonts/ttf/JetBrainsMono-Regular.ttf");
    if (!font.openFromFile(path)) {
        std::cerr << "Failed to load " << path << std::endl;
        return false;
    }
    return true;
}

bool loadTextures(std::map<Piece, sf::Texture> &textures) {
    const std::map<PieceType, std::string> names = {
        {PieceType::Pawn, "Pawn"}, {PieceType::Knight, "Knight"}, {PieceType::Bishop, "Bishop"},
        {PieceType::Rook, "Rook"}, {PieceType::Queen, "Queen"}, {PieceType::King, "King"}};

    for (const auto &[type, name] : names) {
        for (bool white : {true, false}) {
            std::string path = findAsset(std::string("images/") + (white ? "w" : "b") + name + ".png");
            Piece key = makePiece(type, white);
            sf::Texture tex;
            if (!tex.loadFromFile(path)) {
                std::cerr << "Failed to load " << path << std::endl;
                return false;
            }
            tex.setSmooth(true);
            textures[key] = std::move(tex);
        }
    }
    return true;
}

// Squares are rank * 8 + file with white on the bottom of the screen.
sf::Vector2f squarePos(int sq) {
    return {BOARD_X + (sq & 7) * TILE, BOARD_Y + (7 - (sq >> 3)) * TILE};
}

int squareAt(sf::Vector2f p) {
    p -= sf::Vector2f(BOARD_X, BOARD_Y);
    if (p.x < 0 || p.y < 0 || p.x >= BOARD_SIZE || p.y >= BOARD_SIZE) return -1;
    int file = static_cast<int>(p.x / TILE);
    int rank = 7 - static_cast<int>(p.y / TILE);
    return rank * 8 + file;
}

sf::FloatRect promoChoiceRect(int i) {
    return sf::FloatRect({BOARD_X + (BOARD_SIZE - 4 * TILE) / 2.f + i * TILE,
                          BOARD_Y + (BOARD_SIZE - TILE) / 2.f}, {TILE, TILE});
}

void drawText(sf::RenderTarget &target, const sf::Font &font, const std::string &str,
              unsigned size, sf::Vector2f pos, sf::Color color, bool centered = false) {
    sf::Text text(font, str, size);
    text.setFillColor(color);
    if (centered) {
        sf::FloatRect b = text.getLocalBounds();
        text.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
    }
    text.setPosition(pos);
    target.draw(text);
}

void drawButton(sf::RenderTarget &target, const sf::Font &font, const Button &btn,
                sf::Vector2f mouse, unsigned textSize = 22) {
    sf::RectangleShape rect(btn.rect.size);
    rect.setPosition(btn.rect.position);
    rect.setFillColor(btn.rect.contains(mouse) ? BTN_HOVER : BTN_COLOR);
    rect.setOutlineThickness(2.f);
    rect.setOutlineColor(sf::Color(120, 120, 130));
    target.draw(rect);
    drawText(target, font, btn.label, textSize,
             btn.rect.position + btn.rect.size / 2.f, TEXT_COLOR, true);
}

void drawPiece(sf::RenderTarget &target, const std::map<Piece, sf::Texture> &textures,
               Piece piece, sf::Vector2f pos) {
    auto it = textures.find(piece);
    if (it == textures.end()) return;
    sf::Sprite sprite(it->second);
    sf::Vector2u size = it->second.getSize();
    sprite.setScale({TILE / size.x, TILE / size.y});
    sprite.setPosition(pos);
    target.draw(sprite);
}

void fillSquare(sf::RenderTarget &target, int sq, sf::Color color) {
    sf::RectangleShape rect({TILE, TILE});
    rect.setPosition(squarePos(sq));
    rect.setFillColor(color);
    target.draw(rect);
}

std::string statusMessage(const GuiState &s) {
    const char *mover = s.game.isWhiteTurn() ? "White" : "Black";
    const char *other = s.game.isWhiteTurn() ? "Black" : "White";
    switch (s.status) {
        case GameOverStatus::Checkmate: return std::string("Checkmate! ") + other + " wins.";
        case GameOverStatus::Stalemate: return "Stalemate! It's a draw.";
        case GameOverStatus::InsufficientMaterial: return "Draw by insufficient material.";
        case GameOverStatus::InCheck: return std::string(mover) + " to move - Check!";
        case GameOverStatus::Playing: break;
    }
    return std::string(mover) + " to move";
}

void resetGame(GuiState &s) {
    s.game.init();
    s.selected = -1;
    s.selectedMoves.clear();
    s.lastFrom = s.lastTo = -1;
    s.promoFrom = s.promoTo = -1;
    s.status = GameOverStatus::Playing;
}

void clearSelection(GuiState &s) {
    s.selected = -1;
    s.selectedMoves.clear();
}

void attemptMove(GuiState &s, int from, int to, PieceType promo) {
    std::string error;
    if (s.game.tryMove(from, to, promo, error)) {
        s.lastFrom = from;
        s.lastTo = to;
        s.status = s.game.checkGameOver();
    } else {
        std::cerr << error << std::endl;
    }
    clearSelection(s);
    s.promoFrom = s.promoTo = -1;
}

void selectSquare(GuiState &s, int sq) {
    s.selected = sq;
    s.selectedMoves.clear();
    for (const Move &m : s.game.legalMoves()) {
        if (m.from() == sq) s.selectedMoves.push_back(m);
    }
}

void handleBoardClick(GuiState &s, int sq) {
    if (s.selected != -1) {
        for (const Move &m : s.selectedMoves) {
            if (m.to() != sq) continue;
            if (m.promo() != PieceType::None) {
                s.promoFrom = m.from();
                s.promoTo = m.to();
            } else {
                attemptMove(s, m.from(), m.to(), PieceType::None);
            }
            return;
        }
    }

    Piece piece = s.game.getBoard().pieceAt(sq);
    bool ownPiece = piece != Piece::None && isWhite(piece) == s.game.isWhiteTurn();
    if (ownPiece && sq != s.selected) {
        selectSquare(s, sq);
    } else {
        clearSelection(s);
    }
}

void handleClick(GuiState &s, sf::RenderWindow &window, sf::Vector2f p) {
    if (s.screen == Screen::Menu) {
        if (onePlayerBtn.rect.contains(p)) {
            // TODO: single player vs engine
        } else if (twoPlayerBtn.rect.contains(p)) {
            resetGame(s);
            s.screen = Screen::Playing;
        } else if (quitBtn.rect.contains(p)) {
            window.close();
        }
        return;
    }

    if (s.promoFrom != -1) {
        for (int i = 0; i < 4; ++i) {
            if (promoChoiceRect(i).contains(p)) {
                attemptMove(s, s.promoFrom, s.promoTo, PROMO_CHOICES[i]);
                return;
            }
        }
        s.promoFrom = s.promoTo = -1;
        clearSelection(s);
        return;
    }

    if (resetBtn.rect.contains(p)) {
        resetGame(s);
        return;
    }
    if (menuBtn.rect.contains(p)) {
        s.screen = Screen::Menu;
        return;
    }

    if (isGameOver(s.status)) {
        if (playAgainBtn.rect.contains(p)) resetGame(s);
        return;
    }

    int sq = squareAt(p);
    if (sq != -1) handleBoardClick(s, sq);
}

void drawMenu(sf::RenderTarget &target, const sf::Font &font, sf::Vector2f mouse) {
    drawText(target, font, "Chess", 72, {WIN_W / 2.f, 180.f}, TEXT_COLOR, true);
    drawText(target, font, "How many players?", 26, {WIN_W / 2.f, 270.f}, MUTED_TEXT, true);
    drawButton(target, font, onePlayerBtn, mouse);
    drawText(target, font, "coming soon", 14,
             {WIN_W / 2.f, onePlayerBtn.rect.position.y + onePlayerBtn.rect.size.y + 14.f},
             MUTED_TEXT, true);
    drawButton(target, font, twoPlayerBtn, mouse);
    drawButton(target, font, quitBtn, mouse);
}

void drawBoard(sf::RenderTarget &target, const sf::Font &font,
               const std::map<Piece, sf::Texture> &textures, const GuiState &s) {
    const Board &board = s.game.getBoard();

    for (int sq = 0; sq < 64; ++sq) {
        bool light = ((sq & 7) + (sq >> 3)) % 2 == 1;
        fillSquare(target, sq, light ? LIGHT_SQ : DARK_SQ);
    }

    if (s.lastFrom != -1) {
        fillSquare(target, s.lastFrom, LAST_MOVE_COLOR);
        fillSquare(target, s.lastTo, LAST_MOVE_COLOR);
    }
    if (s.selected != -1) fillSquare(target, s.selected, SELECT_COLOR);

    if (board.inCheck(s.game.isWhiteTurn())) {
        Piece king = makePiece(PieceType::King, s.game.isWhiteTurn());
        for (int sq = 0; sq < 64; ++sq) {
            if (board.pieceAt(sq) == king) fillSquare(target, sq, CHECK_COLOR);
        }
    }

    for (int i = 0; i < 8; ++i) {
        sf::Color rankColor = (i % 2 == 0) ? LIGHT_SQ : DARK_SQ;
        drawText(target, font, std::string(1, static_cast<char>('8' - i)), 14,
                 {BOARD_X + 4.f, BOARD_Y + i * TILE + 2.f}, rankColor);
        sf::Color fileColor = (i % 2 == 0) ? LIGHT_SQ : DARK_SQ;
        drawText(target, font, std::string(1, static_cast<char>('a' + i)), 14,
                 {BOARD_X + i * TILE + TILE - 12.f, BOARD_Y + BOARD_SIZE - 20.f}, fileColor);
    }

    for (int sq = 0; sq < 64; ++sq) {
        Piece piece = board.pieceAt(sq);
        if (piece != Piece::None) drawPiece(target, textures, piece, squarePos(sq));
    }

    for (const Move &m : s.selectedMoves) {
        sf::Vector2f center = squarePos(m.to()) + sf::Vector2f(TILE / 2.f, TILE / 2.f);
        bool capture = board.pieceAt(m.to()) != Piece::None;
        float radius = capture ? TILE / 2.f - 4.f : TILE / 7.f;
        sf::CircleShape hint(radius);
        hint.setOrigin({radius, radius});
        hint.setPosition(center);
        if (capture) {
            hint.setFillColor(sf::Color::Transparent);
            hint.setOutlineThickness(5.f);
            hint.setOutlineColor(HINT_COLOR);
        } else {
            hint.setFillColor(HINT_COLOR);
        }
        target.draw(hint);
    }
}

void drawPromotionPicker(sf::RenderTarget &target, const std::map<Piece, sf::Texture> &textures,
                         const GuiState &s, sf::Vector2f mouse) {
    sf::RectangleShape dim({BOARD_SIZE, BOARD_SIZE});
    dim.setPosition({BOARD_X, BOARD_Y});
    dim.setFillColor(sf::Color(0, 0, 0, 140));
    target.draw(dim);

    bool white = s.game.isWhiteTurn();
    for (int i = 0; i < 4; ++i) {
        sf::FloatRect r = promoChoiceRect(i);
        sf::RectangleShape bg(r.size);
        bg.setPosition(r.position);
        bg.setFillColor(r.contains(mouse) ? SELECT_COLOR : LIGHT_SQ);
        bg.setOutlineThickness(2.f);
        bg.setOutlineColor(DARK_SQ);
        target.draw(bg);
        drawPiece(target, textures, makePiece(PROMO_CHOICES[i], white), r.position);
    }
}

void drawGameOverBanner(sf::RenderTarget &target, const sf::Font &font,
                        const GuiState &s, sf::Vector2f mouse) {
    sf::RectangleShape banner({BOARD_SIZE, 150.f});
    banner.setPosition({BOARD_X, BANNER_Y});
    banner.setFillColor(sf::Color(20, 20, 20, 210));
    target.draw(banner);
    drawText(target, font, statusMessage(s), 32, {WIN_W / 2.f, BANNER_Y + 50.f}, TEXT_COLOR, true);
    drawButton(target, font, playAgainBtn, mouse, 20);
}

void drawBottomBar(sf::RenderTarget &target, const sf::Font &font,
                   const GuiState &s, sf::Vector2f mouse) {
    drawText(target, font, statusMessage(s), 22,
             {BOARD_X, BAR_TOP + BAR_HEIGHT / 2.f - 14.f}, TEXT_COLOR);
    drawButton(target, font, resetBtn, mouse);
    drawButton(target, font, menuBtn, mouse);
}

} // namespace

int main() {
    sf::Font font;
    if (!loadFont(font)) {
        return 1;
    }

    std::map<Piece, sf::Texture> textures;
    if (!loadTextures(textures)) {
        return 1;
    }

    sf::RenderWindow window(sf::VideoMode({WIN_W, WIN_H}), "Chess",
                            sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    GuiState state;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            } else if (const auto *click = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (click->button == sf::Mouse::Button::Left) {
                    handleClick(state, window, window.mapPixelToCoords(click->position));
                }
            } else if (const auto *key = event->getIf<sf::Event::KeyPressed>()) {
                if (state.screen != Screen::Playing) continue;
                if (key->code == sf::Keyboard::Key::R) {
                    resetGame(state);
                } else if (key->code == sf::Keyboard::Key::Escape) {
                    state.promoFrom = state.promoTo = -1;
                    clearSelection(state);
                }
            }
        }

        sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        window.clear(BG);
        if (state.screen == Screen::Menu) {
            drawMenu(window, font, mouse);
        } else {
            drawBoard(window, font, textures, state);
            if (state.promoFrom != -1) drawPromotionPicker(window, textures, state, mouse);
            if (isGameOver(state.status)) drawGameOverBanner(window, font, state, mouse);
            drawBottomBar(window, font, state, mouse);
        }
        window.display();
    }
}
