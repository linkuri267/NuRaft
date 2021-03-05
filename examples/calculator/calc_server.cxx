/************************************************************************
Copyright 2017-2019 eBay Inc.
Author/Developer(s): Jung-Sang Ahn

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
**************************************************************************/

#include "calc_state_machine.hxx"
#include "in_memory_state_mgr.hxx"
#include "logger_wrapper.hxx"
#include "crypto.cpp"

#include "nuraft.hxx"
#include "Player.cpp"
#include "test_common.h"

#include <iostream>
#include <sstream>

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <thread>
#include <vector>
#include <strstream>
#include <deque>
#include <unordered_map>
#include <assert.h>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#define PORT 8080

using namespace nuraft;

namespace calc_server {

static const raft_params::return_method_type CALL_TYPE
    = raft_params::blocking;
//  = raft_params::async_handler;

std::string own_private_key_file;
std::string own_public_key_file;

#include "example_common.hxx"

calc_state_machine* get_sm() {
    return static_cast<calc_state_machine*>( stuff.sm_.get() );
}

std::string generate_key_filename(int player_number, bool is_public) {
    std::string s = "player" + std::to_string(player_number);
    if (is_public) {
        s += "public.pem";
    } else {
        s += "private.pem";
    }
    return s;
}

void handle_result(ptr<TestSuite::Timer> timer,
                   raft_result& result,
                   ptr<std::exception>& err)
{
    if (result.get_result_code() != cmd_result_code::OK) {
        // Something went wrong.
        // This means committing this log failed,
        // but the log itself is still in the log store.
        std::cout << "failed: " << result.get_result_code() << ", "
                  << TestSuite::usToString( timer->getTimeUs() )
                  << std::endl;
        return;
    }
    ptr<buffer> buf = result.get();
    uint64_t ret_value = buf->get_ulong();
    std::cout << "succeeded, "
              << TestSuite::usToString( timer->getTimeUs() )
              << ", return value: "
              << ret_value << std::endl
              << "state machine value: "
              << get_sm()->get_current_value() << std::endl
              << "player1 pos: "
              << get_sm()->get_player_pos(1) << std::endl
              << ", player2 pos: " 
              << get_sm()->get_player_pos(2)
              << std::endl;
}

void append_log(const std::string& cmd,
                const std::vector<std::string>& tokens)
{
    // char cmd_char = cmd[0];
    // int operand = atoi( tokens[0].substr(1).c_str() );
    // calc_state_machine::op_type op = calc_state_machine::ADD;
    // switch (cmd_char) {
    // case '+':   op = calc_state_machine::ADD;   break;
    // case '-':   op = calc_state_machine::SUB;   break;
    // case '*':   op = calc_state_machine::MUL;   break;
    // case '/':
    //     op = calc_state_machine::DIV;
    //     if (!operand) {
    //         std::cout << "cannot divide by zero" << std::endl;
    //         return;
    //     }
    //     break;
    // default:    op = calc_state_machine::SET;   break;
    // };

    int player = atoi( tokens[1].c_str() );
    int pos = atoi( tokens[2].c_str() );

    std::cout << "player " << player << " attempting to move to pos " << pos << std::endl;

    // Serialize and generate Raft log to append.
    ptr<buffer> new_log = calc_state_machine::enc_log( {player, pos} );

    // To measure the elapsed time.
    ptr<TestSuite::Timer> timer = cs_new<TestSuite::Timer>();

    // Do append.
    ptr<raft_result> ret = stuff.raft_instance_->append_entries( {new_log} );

    if (!ret->get_accepted()) {
        // Log append rejected, usually because this node is not a leader.
        std::cout << "failed to replicate: "
                  << ret->get_result_code() << ", "
                  << TestSuite::usToString( timer->getTimeUs() )
                  << std::endl;
        return;
    }
    // Log append accepted, but that doesn't mean the log is committed.
    // Commit result can be obtained below.

    if (CALL_TYPE == raft_params::blocking) {
        // Blocking mode:
        //   `append_entries` returns after getting a consensus,
        //   so that `ret` already has the result from state machine.
        ptr<std::exception> err(nullptr);
        handle_result(timer, *ret, err);

    } else if (CALL_TYPE == raft_params::async_handler) {
        // Async mode:
        //   `append_entries` returns immediately.
        //   `handle_result` will be invoked asynchronously,
        //   after getting a consensus.
        ret->when_ready( std::bind( handle_result,
                                    timer,
                                    std::placeholders::_1,
                                    std::placeholders::_2 ) );

    } else {
        assert(0);
    }
}

void print_status(const std::string& cmd,
                  const std::vector<std::string>& tokens)
{
    ptr<log_store> ls = stuff.smgr_->load_log_store();
    std::cout
        << "my server id: " << stuff.server_id_ << std::endl
        << "leader id: " << stuff.raft_instance_->get_leader() << std::endl
        << "Raft log range: "
            << ls->start_index()
            << " - " << (ls->next_slot() - 1) << std::endl
        << "last committed index: "
            << stuff.raft_instance_->get_committed_log_idx() << std::endl
        << "state machine value: "
            << get_sm()->get_current_value() << std::endl
        << "player1 pos: "
            << get_sm()->get_player_pos(1) << std::endl
        << "player2 pos: " 
            << get_sm()->get_player_pos(2) << std::endl;
}

void help(const std::string& cmd,
          const std::vector<std::string>& tokens)
{
    std::cout
    << "modify value: <+|-|*|/><operand>\n"
    << "    +: add <operand> to state machine's value.\n"
    << "    -: subtract <operand> from state machine's value.\n"
    << "    *: multiple state machine'value by <operand>.\n"
    << "    /: divide state machine's value by <operand>.\n"
    << "    e.g.) +123\n"
    << "\n"
    << "add server: add <server id> <address>:<port>\n"
    << "    e.g.) add 2 127.0.0.1:20000\n"
    << "\n"
    << "get current server status: st (or stat)\n"
    << "\n"
    << "get the list of members: ls (or list)\n"
    << "\n";
}

std::vector<std::string> split(const std::string& str, const std::string& delim){
    std::vector<std::string> res;
    if("" == str) return res;
    char * strs = new char[str.length() + 1];
    strcpy(strs, str.c_str());

    char * d = new char[delim.length() + 1];
    strcpy(d, delim.c_str());

    char *p = strtok(strs, d);
    while(p){
        std::string s = p;
        res.push_back(s);
        p = strtok(NULL, d);
    }

    return res;
}

int converStringToInt(const std::string &s){
    int val;
    std::strstream ss;
    ss << s;
    ss >> val;
    return val;
}

std::string pack(std::vector<std::string> tokens){
    std::string res = "";
    for(std::string v: tokens){
        res += v;
        res += "|";
    }
    res += std::to_string(stuff.raft_instance_->get_committed_log_idx());
    res += "|";
    res += signMessage(own_private_key_file, res);
    std::cout << "created full packet: " << res << std::endl;
    std::cout << "from " << own_private_key_file << std::endl;
    return res;
}

std::vector<std::string> unpack(char msg[]){
    std::vector<std::string> tokens;
    char *token = strtok(msg, "|");
    while(token != NULL){
        //std::cout<<"token: "<<token<<std::endl;
        tokens.push_back(token);
        token = strtok(NULL, "|");
    }
    return tokens;
}

std::string get_server_addr(int server_id){
    std::vector<ptr<srv_config>> configs;
    stuff.raft_instance_->get_srv_config_all(configs);

    int leader_id = stuff.raft_instance_->get_leader();
    if(server_id == leader_id) return "-1";
    for(auto& entry: configs){
        ptr<srv_config>& srv = entry;

        if(srv->get_id() == leader_id){
            return srv->get_endpoint();
        }
    }
    return "-2";
}

int sock_connect(std::string srv_ip, std::string srv_port){
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024];

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("\n Socket creation error \n");
        return -1;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);//htons(converStringToInt(srv_port));

    if(srv_ip == "localhost") srv_ip = "127.0.0.1";

    // convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, &*srv_ip.begin(), &serv_addr.sin_addr) <= 0){
        printf("\nInvalid address/ Address not suported \n");
        return -1;
    }

    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        printf("\nConnection Failed \n");
        return -1;
    }

    return sock;
}

bool do_cmd(const std::vector<std::string>& tokens) {
    std::cout << "do_cmd called" <<std::endl;
    if (!tokens.size()) return true;

    const std::string& cmd = tokens[0];

    if (cmd == "q" || cmd == "exit") {
        stuff.launcher_.shutdown(5);
        stuff.reset();
        return false;
    } else if ( cmd == "move" ) {
        std::string srv_addr_str = get_server_addr(stuff.server_id_);
        if(srv_addr_str == "-2"){
            //std::cout<<"-2"<<std::endl;
            perror("getting server addr failed");
            return false;
        }else if(srv_addr_str == "-1"){
            //std::cout<<"-1"<<std::endl;
            if (tokens.size() <= 3) { //non full packet with signature and log index. this means we are sending locally on the leader node (leader node player is sending command)
                append_log(cmd, tokens);
            }
            else {
                //this is remote command, need to verify authority
                //tokens [1] should be player number. lets get it's public key
                std::string remote_public = generate_key_filename(stoi(tokens[1]), true);
                std::cout << "Starting message authority verification. Using public key file: " << remote_public << std::endl;
                std::string msg = "";
                for (int i = 0; i < 4; i++) { //construct the first part of the packet
                    msg += tokens[i];
                    msg += "|";
                }
                std::cout << "Message to verify: " << msg << std::endl;
                std::cout << "Signature: " << (char*) tokens[4].c_str()<< std::endl;
                // bool res = verifySignature(remote_public, msg, (char*) tokens[4].c_str());
                // if (res) {
                //     std::cout << "Verfication successful" << std::endl;
                // } else {
                //     std::cout << "verification unsuccessful" << std::endl;
                // }
            }
        }else{
            //std::cout<<">0"<<std::endl;
            int delim_pos = srv_addr_str.find(":");
            std::string srv_ip = srv_addr_str.substr(0, delim_pos);
            std::string srv_port = srv_addr_str.substr(delim_pos+1, srv_addr_str.length());
            int sock = sock_connect(srv_ip, srv_port);

            std::string res = pack(tokens);
            int n = send(sock, res.c_str(), res.length(), 0);
            if(n < 0){
                std::cout<<"ERROR writing to socket"<<std::endl;
            }else{
                //std::cout<<"tokens message sent"<<std::endl;
            }
            
            char buffer[1024];
            n = read(sock, buffer, 1024);
            if(n < 0){
                std::cout<<"ERROR reading from socket"<<std::endl;
            }else{
                std::cout<<"client receives:"<<buffer<<std::endl;
            }
            close(sock);
            // } else if ( cmd[0] == '+' ||
            //             cmd[0] == '-' ||
            //             cmd[0] == '*' ||
            //             cmd[0] == '/' ) {
            //     // e.g.) +1
            //     append_log(cmd, tokens);
        }
    } else if ( cmd == "add" ) {
        // e.g.) add 2 localhost:12345
        add_server(cmd, tokens);

    } else if ( cmd == "st" || cmd == "stat" ) {
        print_status(cmd, tokens);

    } else if ( cmd == "ls" || cmd == "list" ) {
        server_list(cmd, tokens);

    } else if ( cmd == "h" || cmd == "help" ) {
        help(cmd, tokens);
    }
    return true;
}

}; // namespace calc_server;
using namespace calc_server;

void server_listening(){
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024];

    // create socket file descriptor
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("socket failed");
        exit(1);
    }

    // forcefully attaching socket to the port 8080
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("setsockopt");
        exit(1);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(PORT);

    if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
        perror("bind failed");
        exit(1);
    }
    if(listen(server_fd, 3) < 0){
        perror("listen");
        exit(1);
    }
    while(true){
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if(new_socket < 0){
            perror("accept");
            continue;
        }
        // if (get_server_addr(stuff.server_id_) != "-1") {
        //     std::cout << "connection refused because not leader" << std::endl;
        //     continue;
        // }
        valread = read(new_socket, buffer, 1024);
        if(valread < 0){
            printf("ERROR reading from socket");
            continue;
        }
        //std::cout<<"server receives buffer:"<<buffer<<std::endl;
        std::vector<std::string> tokens = unpack(buffer);
        //std::cout<<"server receives tokens:"<<tokens<<std::endl;
        char* msg;
        if(do_cmd(tokens)){
            msg = "operation succeeded.";
        }else{
            msg = "operation failed.";
        }
        send(new_socket, msg, strlen(msg), 0);
    }
    close(server_fd);
}

void UI_run(){
    // init game
    float gridSizeF = 60.f;
    unsigned gridSizeU = static_cast<unsigned>(gridSizeF);
    int playerNum = 2;
    std::vector<sf::Color> playerColor = {sf::Color::Red, sf::Color::Yellow};
    std::vector<sf::Vector2f> origins = {sf::Vector2f(2*gridSizeF, 2*gridSizeF), sf::Vector2f(2*gridSizeF, 8*gridSizeF),
        sf::Vector2f(8*gridSizeF, 2*gridSizeF), sf::Vector2f(8*gridSizeF, 8*gridSizeF)};
    int64_t curPlayerValue;
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
        player.setOrigin(origins[i]);
        Player* player_c = new Player(player);
        players.push_back(*player_c);
    }
    

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
        for(int i=0; i<playerNum; i++){
            int pos = get_sm()->get_player_pos(i+1);
            players[i].setPos(pos, i+1);
        }
        curPlayerValue = get_sm()->get_current_value();

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
                    bomb.setPosition(players[curPlayerValue-1].getPlayer().getPosition());
                    Bomb* bomb_g = new Bomb(bomb, power);
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
            if(myPosGrid.x < 10){
                myPosGrid.x += 1;
            }
        }else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Up)){
            if(myPosGrid.y > 0){
                myPosGrid.y -= 1;
            }
        }else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Down)){
            if(myPosGrid.y < 10){
                myPosGrid.y += 1;
            }
        }
        players[curPlayerValue-1].setPos(myPosGrid.x, myPosGrid.y);
            // players[curPlayerValue-1].getPlayer().move(speed * dt, 0.f);

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

        for(auto& player: players){
            window.draw(player.getPlayer());
        }
        
        window.draw(tileSelector);

        //Render UI
        window.setView(window.getDefaultView());

        //Done drawing
        window.display();
    }
}

int main(int argc, char** argv) {
    if (argc < 3) usage(argc, argv);

    set_server_info(argc, argv);

    //crypto stuff
    std::unordered_map<int, std::string> player_number_key_mapping;
    own_private_key_file = generate_key_filename(stuff.server_id_, false);
    own_public_key_file = generate_key_filename(stuff.server_id_, true);
    //RSA* own_private = createPrivateRSAFromFile((char *)own_private_key_file.c_str());
    //RSA* own_public = createPublicRSAFromFile((char*)own_public_key_file.c_str());


    //crypto testing
    std::string test_msg = "example_packet_without_sign";
    char* signature = signMessage(own_private_key_file, test_msg);
    assert(verifySignature(own_public_key_file, test_msg, signature));

    std::cout << "    -- Replicated Game State Machine with Raft --" << std::endl;
    std::cout << "                         Version 0.1.0" << std::endl;
    std::cout << "    Server ID:    " << stuff.server_id_ << std::endl;
    std::cout << "    Endpoint:     " << stuff.endpoint_ << std::endl;
    std::cout << "    Private Key File:     " << own_private_key_file << std::endl;
    init_raft( cs_new<calc_state_machine>() );

    // std::thread listen_socket(server_listening);
    // std::thread listen_cmd(loop);
    UI_run();
    // listen_cmd.join();
    // listen_socket.join();

    return 0;
}

