#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <string>
#include <sstream>
#include <stdio.h>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include "Player.cpp"

const char* readFile = "readFile.txt";
const char* writeFile = "writeFile.txt";

int main(){
    std::ofstream fout;
    // init game
    float gridSizeF = 60.f;
    unsigned gridSizeU = static_cast<unsigned>(gridSizeF);
    int playerNum = 2;
    std::vector<sf::Color> playerColor = {sf::Color::Red, sf::Color::Yellow};
    std::vector<sf::Vector2f> origins = {sf::Vector2f(1*gridSizeF, 1*gridSizeF), sf::Vector2f(8*gridSizeF, 1*gridSizeF),
        sf::Vector2f(8*gridSizeF, 1*gridSizeF), sf::Vector2f(8*gridSizeF, 8*gridSizeF)};
    int64_t curPlayerValue = 1;
    int curLastPos = -1;
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
    std::vector<Player> players;
    for(int i=0; i<playerNum; i++){
        sf::RectangleShape player(sf::Vector2f(gridSizeF, gridSizeF));
        player.setFillColor(playerColor[i]);
        player.setPosition(origins[i]);
        Player* player_c = new Player(player);
        players.push_back(*player_c);
    }
    

    std::deque<Bomb> bombs;
    std::deque<Waves> waves;

    const int mapSize = 10;
    std::vector<std::vector <sf::RectangleShape>> tileMap;
    tileMap.resize(mapSize, std::vector<sf::RectangleShape>());
    for(int x = 0; x < mapSize; x++){
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
        std::string line;
        std::ifstream fin(readFile);
        if(fin){
            while(getline(fin, line)){
                std::vector<int> res;
                std::string result;
                std::stringstream input(line);
                while(input>>result){
                    res.push_back(atoi(result.c_str()));
                }
                int x = res[1]/10;
                int y = res[1]%10;
                players[res[0]-1].setPos(x, y);
                std::cout<<"line: "<<line<<std::endl;
            }
            fin.close();
            remove(readFile);
        }
        
        // for(int i=0; i<playerNum; i++){
        //     int pos = get_sm()->get_player_pos(i+1);
        //     players[i].setPos(pos, i+1);
        // }
        // curPlayerValue = get_sm()->get_current_value();

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
                    sf::Vector2u posGrid = players[curPlayerValue-1].getPosGrid();
                    Bomb* bomb_g = new Bomb(bomb, power);
                    bomb_g->setPos(posGrid.x, posGrid.y);
                    std::cout<<"x: "<<posGrid.x<<" , y: "<<posGrid.y<<std::endl;
                    bombs.push_back(*bomb_g);
                }
            }
        }

        // Update
        // Update input
        sf::Vector2u myPosGrid = players[curPlayerValue-1].getPosGrid();
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Left)){
            if(myPosGrid.x > 0){
                myPosGrid.x -= 1;
            }
        }else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Right)){
            if(myPosGrid.x < 9){
                myPosGrid.x += 1;
            }
        }else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Up)){
            if(myPosGrid.y > 0){
                myPosGrid.y -= 1;
            }
        }else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Down)){
            if(myPosGrid.y < 9){
                myPosGrid.y += 1;
            }
        }
        if(myPosGrid.x*10 + myPosGrid.y != curLastPos){
            players[curPlayerValue-1].setPos(myPosGrid.x, myPosGrid.y);
            curLastPos = myPosGrid.x*10 + myPosGrid.y;
            // players[curPlayerValue-1].getPlayer().move(speed * dt, 0.f);

            fout.open(writeFile, std::ios::in | std::ios::app);
            fout<<curPlayerValue<<" "<<curLastPos<<std::endl;
            fout.close();
        }
        

        //Render
        window.clear();

        //Render game elements
        window.setView(view);

        for(int x = 0; x < mapSize; x++){
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

        for(auto& player: players){
            window.draw(player.getPlayer());
        }
        
        window.draw(tileSelector);

        //Render UI
        window.setView(window.getDefaultView());

        //Done drawing
        window.display();
    }

    return 0;
}