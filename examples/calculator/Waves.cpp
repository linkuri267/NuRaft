#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>

const float gridSizeF = 60.f;
const unsigned gridSizeU = static_cast<unsigned>(gridSizeF);

class Waves{
    private:
        std::vector<sf::RectangleShape> waves;
        sf::Clock clock;

    public:
        Waves(sf::Vector2f position, int power){
            initWaves(position, power);
            clock.restart().asSeconds();
        }

        void initWaves(sf::Vector2f position, int power){
            if(power < 1) power = 1;
            int len = 4 * power + 1;
            waves.resize(len, sf::RectangleShape(sf::Vector2f(gridSizeF, gridSizeF)));
            float bomb_x = position.x;
            float bomb_y = position.y;

            waves[len-1].setFillColor(sf::Color::Cyan);
            waves[len-1].setPosition(bomb_x, bomb_y);
            for(int i=0; i < len-1; i++){
                waves[i].setFillColor(sf::Color::Cyan);
                float dist = gridSizeF * (i/4 + 1);
                if(i%4 == 0){
                    waves[i].setPosition(bomb_x + dist, bomb_y);
                }else if(i%4 == 1){
                    waves[i].setPosition(bomb_x, bomb_y + dist);
                }else if(i%4 == 2){
                    waves[i].setPosition(bomb_x - dist, bomb_y);
                }else if(i%4 == 3){
                    waves[i].setPosition(bomb_x, bomb_y - dist);
                }
            }
        }

        float getTime(){
            return clock.getElapsedTime().asSeconds();
        }

        std::vector<sf::RectangleShape> getWaves(){
            return waves;
        }
};