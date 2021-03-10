#include <iostream>
#include <vector>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include "Waves.cpp"

class Bomb{
    private: 
        sf::CircleShape bomb;
        sf::Vector2u posGrid;
        int power;
        sf::Clock clock;
        Waves* waves;

    public:
        Bomb(sf::CircleShape bomb, int power){
            this->bomb = bomb;
            this->power = power;
            this->posGrid.x = (int)(bomb.getPosition().x / gridSizeU + 0.5f);
            this->posGrid.y = (int)(bomb.getPosition().y / gridSizeU + 0.5f);
        }

        sf::CircleShape getBomb(){
            return bomb;
        }

        void setPos(int x, int y){
            this->posGrid.x = x;
            this->posGrid.y = y;
            bomb.setPosition(x * gridSizeF, y * gridSizeF);
        }

        float getTime(){
            return clock.getElapsedTime().asSeconds();
        }

        Waves getWaves(){
            waves = new Waves(bomb.getPosition(), power);
            return *waves;
        }
};