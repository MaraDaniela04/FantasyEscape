#include <iostream>
#include <vector>
#include <deque>
#include <array>
#include <utility>
#include <algorithm>
#include <string>
#include <optional>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics.hpp>
#include <cstdlib>
const bool ciRun = std::getenv("CI") != nullptr;

class Item {
    sf::Texture ItemTexture;
    sf::Sprite ItemSp;
    std::string name;
    int id;
    bool empty;
    bool taken;

public:
    Item():ItemSp(ItemTexture), name("empty"), id(-1), empty(true), taken(false) {}

    Item(const std::string &texturePath, std::string itemName, const int itemId)
    : ItemSp(ItemTexture), name(std::move(itemName)), id(itemId), empty(true), taken(false) {
        if (!ItemTexture.loadFromFile(texturePath)) {
            std::cout << "Failed to load item texture" << std::endl;
            return;
        }
        ItemSp.setTexture(ItemTexture, true);
        empty = false;
    }

    Item(const Item &other) : ItemTexture(other.ItemTexture), ItemSp(ItemTexture),
                                name(other.name), id(other.id), empty(other.empty), taken(other.taken) {
        ItemSp.setTexture(ItemTexture, true);
    }

    Item &operator=(const Item &other) {
        if (this != &other) {
            ItemTexture = other.ItemTexture;
            ItemSp.setTexture(ItemTexture, true);
            name = other.name;
            id = other.id;
            empty = other.empty;
            taken = other.taken;
        }
        return *this;
    }

    ~Item()=default;

    bool isEmpty() const { return empty; }

    bool isTaken() const { return taken; }
    void setTaken(bool t) { taken = t; }

    void setPosition(const sf::Vector2f position, const float size){
        const auto textureSize = ItemTexture.getSize();
        const auto tSx = static_cast<float>(textureSize.x);
        const auto tSy = static_cast<float>(textureSize.y);
        if (tSx == 0 || tSy == 0) return;
        ItemSp.setOrigin({tSx / 2.0f, tSy / 2.0f});
        const float scale = size / static_cast<float>(std::max(textureSize.x, textureSize.y));
        ItemSp.setScale({scale, scale});
        ItemSp.setPosition(position);
    }

    void placeIn(const sf::Vector2f slotPos, const float slotSize) {
        const auto textureSize = ItemTexture.getSize();
        const auto tSx = static_cast<float>(textureSize.x);
        const auto tSy = static_cast<float>(textureSize.y);
        if (tSx == 0 || tSy == 0) return;
        ItemSp.setOrigin({tSx / 2.0f, tSy / 2.0f});
        const float scale = (slotSize * 0.8f) / static_cast<float>(std::max(textureSize.x, textureSize.y));
        ItemSp.setScale({scale, scale});
        ItemSp.setPosition({slotPos.x + slotSize / 2.0f, slotPos.y + slotSize / 2.0f});
    }

    void draw(sf::RenderWindow &window) const {
        window.draw(ItemSp);
    }

    bool isClicked(const sf::Vector2f mousePos) const {
        return ItemSp.getGlobalBounds().contains(mousePos);
    }

    friend std::ostream &operator<<(std::ostream& os, const Item& item) {
        os << "Item:" << item.name << " (" << item.id << ")";
        return os;
    }
};

class Slot {
private:
    int slotNumber;
    Item item;
    sf::RectangleShape shape;

public:
    Slot(const int num, const Item& item, const sf::Vector2f size) : slotNumber(num), item(item) {
        shape.setSize(size);
        shape.setFillColor(sf::Color(50, 50, 50, 200));
        shape.setOutlineThickness(2);
        shape.setOutlineColor(sf::Color(200, 200, 200));
    }

    void setPosition(const sf::Vector2f position) {
        shape.setPosition(position);
        item.placeIn(position, shape.getSize().x);
    }

    bool isEmpty() const { return item.isEmpty(); }

    void setItem(const Item &newItem) {
        item = newItem;
        item.placeIn(shape.getPosition(), shape.getSize().x);
    }

    void draw(sf::RenderWindow& window, const bool isSelected) {
        if (isSelected) {
            shape.setOutlineColor(sf::Color(255, 225, 0));
        } else {
            shape.setOutlineColor(sf::Color(90, 90, 90));
        }
        window.draw(shape);
        if (!item.isEmpty())
            item.draw(window);
    }

    friend std::ostream& operator<<(std::ostream& os, const Slot& slot) {
        os << "Slot:" << slot.slotNumber << " (" << slot.item << ")";
        return os;
    }
};

class Inventory {
    int slotCount;
    std::vector<Slot> slots;
    int index;  // Indexul slotului activ

public:
    Inventory(const int count, const float slotSize) : slotCount(count), index(0) {
        for (int i = 0; i < slotCount; i++) {
            slots.emplace_back(i, Item(), sf::Vector2f{slotSize, slotSize});
        }
    }

    bool addItem(const Item &item) {
        for (int i = 0; i < slotCount; i++) {
            if (slots[i].isEmpty()) {
                slots[i].setItem(item);
                return true;
            }
        }
        return false;
    }

    void setPosition(const float x, const float y, const float slotSize, const float spacing) {
        for (int i = 0; i < slotCount; i++) {
            slots[i].setPosition(sf::Vector2f{x + static_cast<float>(i) * (slotSize + spacing), y});
        }
    }

    void selectSlot(const int ind) {
        if (ind >= 0 && ind < slotCount) {
            index = ind;
        }
    }

    void draw(sf::RenderWindow &window) {
        for (int i = 0; i < slotCount; i++) {
            slots[i].draw(window, i == index);
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const Inventory& inventory) {
        os << "Inventory:" << std::endl;
        for (const auto& slot : inventory.slots) {
            os << " " << slot << std::endl;
        }
        return os;
    }
};

enum class Direction{Left, Right, Up, Down};

class MovementArrows {
    sf::Texture ArrowTexture;
    sf::Sprite Arrow;
    Direction direction;

    static float angle(const Direction dir) {
        switch (dir) {
            case Direction::Left: return 0.0f;
            case Direction::Up: return 90.0f;
            case Direction::Right: return 180.0f;
            case Direction::Down: return 270.0f;
        }
        return 0.0f;
    }

    static const char *directionName(const Direction dir) {
        switch (dir) {
            case Direction::Left: return "left";
            case Direction::Up: return "up";
            case Direction::Right: return "right";
            case Direction::Down: return "down";
        }
        return "";
    }

public:
    MovementArrows(const std::string &texturePath, const sf::Vector2f center, const float size, const Direction dir) : Arrow(ArrowTexture), direction(dir) {
        if (!ArrowTexture.loadFromFile(texturePath)) {
            std::cout<<"Texture load failed"<< texturePath << std::endl;
            return;
        }
        Arrow.setTexture(ArrowTexture, true);

        const auto textureSize = ArrowTexture.getSize();
        Arrow.setOrigin({static_cast<float>(textureSize.x) / 2.0f, static_cast<float>(textureSize.y) / 2.0f});
        const float scale = size/static_cast<float>(textureSize.x);
        Arrow.setScale({scale, scale});
        Arrow.setRotation(sf::degrees(angle(dir)));
        Arrow.setPosition(center);
    }

    friend std::ostream &operator<<(std::ostream &os, const MovementArrows &Arrows) {
        os << "Arrow:" << directionName(Arrows.direction);
        return os;
    }

    void draw(sf::RenderWindow &window) const {
        window.draw(Arrow);
    }

    bool isClicked(const sf::Vector2f mousePos) const {
        return Arrow.getGlobalBounds().contains(mousePos);
    }
};

class Scene {
    sf::Texture backgroundTexture;
    sf::Sprite backgroundSprite;
    std::array<Scene *, 4> neighbours;
    std::string name;
    MovementArrows rightArrow, leftArrow, upArrow, downArrow;

    static constexpr float arrowSize = 25.0f;
    static constexpr float margin = 20.0f;

    static std::size_t idx(Direction d){ return static_cast<std::size_t>(d); }

    static Direction opposite(const Direction d) {
        switch (d) {
            case Direction::Left: return Direction::Right;
                case Direction::Up: return Direction::Down;
                case Direction::Right: return Direction::Left;
                case Direction::Down: return Direction::Up;
        }
        return d;
    }

    const MovementArrows &arrow(const Direction d) const {
        switch (d) {
            case Direction::Left: return leftArrow;
                case Direction::Up: return upArrow;
                case Direction::Right: return rightArrow;
            default: return downArrow;
        }
    }

public:
    Scene(const std::string &bgPath, const std::string &arrowPath, unsigned int const width, unsigned int const height):
        backgroundSprite(backgroundTexture),  neighbours{nullptr, nullptr, nullptr, nullptr}, name(bgPath),
        rightArrow(arrowPath, {static_cast<float>(width) - margin - arrowSize/2.0f, static_cast<float>(height)/2.0f}, arrowSize, Direction::Right),
        leftArrow(arrowPath, {margin + arrowSize/2.0f, static_cast<float>(height)/2.0f}, arrowSize, Direction::Left),
        upArrow(arrowPath, {static_cast<float>(width)/2.0f, margin + arrowSize/2.0f}, arrowSize, Direction::Up),
        downArrow(arrowPath, {static_cast<float>(width)/2.0f, static_cast<float>(height) - 100.0f - arrowSize/2.0f}, arrowSize, Direction::Down) {

        if (!backgroundTexture.loadFromFile(bgPath)) {
            std::cout << "Background texture load failed"<<bgPath<<std::endl;
        }

        backgroundSprite.setTexture(backgroundTexture, true);
        backgroundSprite.setScale({
            static_cast<float>(width)/static_cast<float>(backgroundTexture.getSize().x),
            static_cast<float>(height)/static_cast<float>(backgroundTexture.getSize().y)});
    }

    friend std::ostream &operator<<(std::ostream& os, const Scene &scene) {
        os << "Scene: " << scene.name << std::endl;
        for (const Direction d : {Direction::Left, Direction::Up, Direction::Right, Direction::Down}) {
            if (scene.neighbours[idx(d)])
                os << " " << scene.arrow(d) << " -> " << scene.neighbours[idx(d)]->name << std::endl;
        }
        return os;
    }

    void connect(const Direction dir, Scene &other) {
        neighbours[idx(dir)] = &other;
        other.neighbours[idx(opposite(dir))] = this;
    }

    void draw(sf::RenderWindow &window) const {
        window.draw(backgroundSprite);
        for (const Direction d:{Direction::Left, Direction::Right, Direction::Up, Direction::Down}) {
            if (neighbours[idx(d)])
                arrow(d).draw(window);
        }
    }

    Scene *arrowClick (const sf::Vector2f mousePos) {
        for (const Direction d:{Direction::Left, Direction::Right, Direction::Up, Direction::Down}) {
            if (neighbours[idx(d)] != nullptr && arrow(d).isClicked(mousePos))
                return neighbours[idx(d)];
        }
        return this;
    }

};

int main() {
    const unsigned int width = 1000, height = 800, slotCount = 5;
    const float slotSize = 60, spacing = 8;

    Item clover("assets/clover.png", "clover", 1);
    clover.setPosition({500.0f, 500.0f}, 60.0f);
    sf::RenderWindow window{sf::VideoMode{{width, height}}, "FantasyEscape"};
    window.setFramerateLimit(60);

    std::string arrowPath = "assets/arrow.png";

    std::deque<Scene> scenes;
    scenes.emplace_back("assets/s1.dark_forest.jpg", arrowPath, width, height);
    scenes.emplace_back("assets/s2.castle.jpg", arrowPath, width, height);
    scenes.emplace_back("assets/s3.lobby.jpg", arrowPath, width, height);
    scenes.emplace_back("assets/s4.library.jpg", arrowPath, width, height);
    scenes.emplace_back("assets/s5.bedroom.jpg", arrowPath, width, height);
    scenes.emplace_back("assets/s6.kitchen.jpg", arrowPath, width, height);

    scenes[0].connect(Direction::Up, scenes[1]);
    scenes[1].connect(Direction::Up, scenes[2]);
    scenes[2].connect(Direction::Up, scenes[3]);
    scenes[2].connect(Direction::Right, scenes[4]);
    scenes[2].connect(Direction::Left, scenes[5]);

    Scene *currentScene = &scenes[0];

    std::cout << scenes[2];

    Inventory hotbar(slotCount, slotSize);

    float totalBarWidth = (slotCount * slotSize) + ((slotCount - 1) * spacing);
    float x = (width - totalBarWidth) / 2;
    float y = height - slotSize - 20;

    hotbar.setPosition(x, y, slotSize, spacing);

    std::cout << hotbar << std::endl;

    while (window.isOpen()) {
        if (ciRun) window.close();

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* buttonPressed = event->getIf<sf::Event::MouseButtonPressed>())
                if (buttonPressed->button == sf::Mouse::Button::Left) {
                    sf::Vector2f mousePos = window.mapPixelToCoords(buttonPressed->position);

                    if (!clover.isTaken() && currentScene == &scenes[0] && clover.isClicked(mousePos)) {
                        if (hotbar.addItem(clover)) {
                            clover.setTaken(true);
                            std::cout << hotbar << std::endl;
                        }
                    }
                    else
                        currentScene = currentScene->arrowClick(mousePos);
                }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->scancode == sf::Keyboard::Scan::Num1) hotbar.selectSlot(0);
                if (keyPressed->scancode == sf::Keyboard::Scan::Num2) hotbar.selectSlot(1);
                if (keyPressed->scancode == sf::Keyboard::Scan::Num3) hotbar.selectSlot(2);
                if (keyPressed->scancode == sf::Keyboard::Scan::Num4) hotbar.selectSlot(3);
                if (keyPressed->scancode == sf::Keyboard::Scan::Num5) hotbar.selectSlot(4);
            }
        }

        window.clear(sf::Color::Black);

        currentScene->draw(window);

        if (!clover.isTaken() && currentScene == &scenes[0])
            clover.draw(window);
        hotbar.draw(window);

        window.display();
    }

    return 0;
}
