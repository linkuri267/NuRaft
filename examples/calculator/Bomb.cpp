#include <iostream>
#include <vector>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include "Waves.cpp"

class Bomb{
    private: 
        sf::CircleShape bomb;
        int power;
        sf::Clock clock;
        Waves* waves;

    public:
        Bomb(sf::CircleShape bomb, int power){
            this->bomb = bomb;
            this->power = power;
        }

        sf::CircleShape getBomb(){
            return bomb;
        }

        float getTime(){
            return clock.getElapsedTime().asSeconds();
        }

        Waves getWaves(){
            waves = new Waves(bomb.getPosition(), power);
            return *waves;
        }
};