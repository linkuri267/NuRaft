#include <iostream>
#include <vector>
#include <deque>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include "Bomb.cpp"

int main(){
    // init game
    float gridSizeF = 60.f;
    unsigned gridSizeU = static_cast<unsigned>(gridSizeF);
    int bombNum = 4;
    int power = 1;
    float dt = 0.f;
    sf::Clock dtClock;
    sf::Vector2i mousePosWindow;
    sf::Vector2f mousePosView;
    sf::Vector2u mousePosGrid;

    // init the window
    sf::RenderWindow window(sf::VideoMode(1920, 1080), "SFML works!");
    window.setFramerateLimit(120);

    sf::View view;
    view.setSize(1680.f, 900.f);
    view.setCenter(window.getSize().x / 2.f, window.getSize().y / 2.f);
    float speed = 200.f;

    window.setView(view);

    // init game elements
    sf::RectangleShape player(sf::Vector2f(gridSizeF, gridSizeF));
    player.setFillColor(sf::Color::Red);

    std::deque<Bomb> bombs;
    std::deque<Waves> waves;

    const int mapSize = 10;
    std::vector<std::vector <sf::RectangleShape>> tileMap;
    tileMap.resize(mapSize*2, std::vector<sf::RectangleShape>());
    for(int x = 0; x < mapSize*2; x++){
        tileMap[x].resize(mapSize, sf::RectangleShape());
        for(int y = 0; y < mapSize; y++){
            tileMap[x][y].setSize(sf::Vector2f(gridSizeF, gridSizeF));
            tileMap[x][y].setFillColor(sf::Color::White);
            tileMap[x][y].setOutlineColor(sf::Color::Black);
            tileMap[x][y].setOutlineThickness(2.f);
            tileMap[x][y].setPosition(x * gridSizeF, y * gridSizeF);
        }
    }

    sf::RectangleShape tileSelector(sf::Vector2f(gridSizeF, gridSizeF));
    tileSelector.setFillColor(sf::Color::Transparent);
    tileSelector.setOutlineColor(sf::Color::Green);
    tileSelector.setOutlineThickness(2.f);
    

    while(window.isOpen()){
        //Update dt
        dt = dtClock.restart().asSeconds();

        //Update mouse positions
        mousePosWindow = sf::Mouse::getPosition(window);
        window.setView(view);
        mousePosView = window.mapPixelToCoords(mousePosWindow);
        if(mousePosView.x >= 0.f){
            mousePosGrid.x = mousePosView.x / gridSizeU;
        }
        if(mousePosView.y >= 0.f){
            mousePosGrid.y = mousePosView.y / gridSizeU;
        }
        window.setView(window.getDefaultView());

        //Update game elements
        tileSelector.setPosition(mousePosGrid.x * gridSizeF, mousePosGrid.y * gridSizeF);
        while(!bombs.empty()){
            if(bombs.front().getTime() < 4){
                break;
            }
            waves.push_back(bombs.front().getWaves());
            bombs.pop_front();
        }
        
        while(!waves.empty()){
            if(waves.front().getTime() < 1){
                break;
            }
            waves.pop_front();
        }

        //Update UI


        //Events
        sf::Event event;
        while(window.pollEvent(event)){
            if(event.type == sf::Event::Closed){
                window.close();
            }
            if(event.type == sf::Event::KeyPressed){
                if(event.key.code == sf::Keyboard::Space && bombs.size() < bombNum){
                    sf::CircleShape bomb(gridSizeF / 2);
                    bomb.setFillColor(sf::Color::Cyan);
                    bomb.setPosition((int)(player.getPosition().x / gridSizeU + 0.5f) * gridSizeF, (int)(player.getPosition().y / gridSizeU + 0.5f) * gridSizeF);
                    Bomb* bomb_g = new Bomb(bomb, power);
                    bombs.push_back(*bomb_g);
                }
            }
        }

        // Update
        // Update input
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Left)){
            player.move(-speed * dt, 0.f);
        }else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Right)){
            player.move(speed * dt, 0.f);
        }else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Up)){
            player.move(0.f, -speed * dt);
        }else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Down)){
            player.move(0.f, speed * dt);
        }

        //Render
        window.clear();

        //Render game elements
        window.setView(view);

        for(int x = 0; x < mapSize*2; x++){
            for(int y = 0; y < mapSize; y++){
                window.draw(tileMap[x][y]);
            }
        }

        for(auto& bomb: bombs){
            window.draw(bomb.getBomb());
        }
        for(auto& wave: waves){
            for(auto& wave_tile: wave.getWaves()){
                window.draw(wave_tile);
            }
        }
        window.draw(player);
        window.draw(tileSelector);

        //Render UI
        window.setView(window.getDefaultView());

        //Done drawing
        window.display();
    }

    return 0;
}