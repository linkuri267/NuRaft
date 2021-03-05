#include <iostream>
#include <vector>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include "Bomb.cpp"

class Player{
    private:
        sf::RectangleShape player;
        sf::Vector2u posGrid;

    public:
        Player(sf::RectangleShape p){
            this->player = p;
            this->posGrid.x = (int)(p.getPosition().x / gridSizeU + 0.5f);
            this->posGrid.y = (int)(p.getPosition().y / gridSizeU + 0.5f);
        }

        void setPos(int x, int y){
            this->posGrid.x = x;
            this->posGrid.y = y;
            player.setPosition(x * gridSizeF, y * gridSizeF);
        }

        sf::RectangleShape getPlayer(){
            return player;
        }

        sf::Vector2u getPosGrid(){
            return posGrid;
        }
};