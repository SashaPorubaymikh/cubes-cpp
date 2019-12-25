#include <sys/socket.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SFML/Network.hpp>
#include <stdio.h>
#include "scripts/cube.h"
#include "scripts/bullet.h"
#include <cmath>
#include "scripts/get&send.h"
#include <string>
#include "scripts/menu.h"
#include "scripts/font.h"
#include <algorithm>


using namespace std;
using sf::UdpSocket;
using sf::Socket;
using sf::IpAddress;

SDL_Window *window = NULL;
SDL_Renderer *ren = NULL;
TTF_Font *nameFont = NULL;
Font *menuOptionFont = NULL;

int random(int min, int max) {
    return min + rand()%max;
}
int random(int max) {
    return rand()%max;
}

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Can't init SDL2");
        return false;
    }

    if (TTF_Init() == -1) {
        printf("Can't init TTF\n");
        return false;
    }

    nameFont = TTF_OpenFont("fonts/FONT.otf", 12);
    menuOptionFont = new Font("fonts/FONT.ttf", 35);

    window = SDL_CreateWindow("CUBES", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Can't create window");
        return false;
    }

    ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (ren == NULL) {
        printf("Can't create renderer");
        return false;
    }

    return true;
}

void quit() {
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(window);

    ren = NULL;
    window = NULL;

    SDL_Quit();
}

int HOST(string playerName) {
    UdpSocket socket;
    socket.bind(Socket::AnyPort);
    socket.setBlocking(false);
    IpAddress ADDRESS = IpAddress::getLocalAddress();
    unsigned short PORT = socket.getLocalPort();
    
    SDL_Event e;

    bool run = true;
    int xbutton, ybutton;
    bool rpr = false, lpr = false, upr = false, dpr = false, lmpr = false;

    int fTime = 1000 / 60, fDelta = 0;
    long int fStart, fEnd;
    int mPos[2] = {0, 0};

    vector<Bullet> bullets;
    vector<Player> players;
    vector<int> ports;

    players.push_back(Player(random(590), random(430), PORT, ADDRESS, playerName));

    char pData[2048];
    size_t received;
    IpAddress pAddr;
    unsigned short pPort;

    const vector<string> options = {"START", "BACK"};
    const vector<string> optionsAddOns = {"", ""};
    int currentOption = 0;

    while (run) {
        ports.clear();
        fStart = SDL_GetTicks();
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                return -1;
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_DOWN) {
                    currentOption++;
                    if (currentOption == options.size()) {
                        currentOption = 0;
                    }
                }
                if (e.key.keysym.sym == SDLK_UP) {
                    currentOption--;
                    if (currentOption == -1) {
                        currentOption = options.size() - 1;
                    }
                }
                if (e.key.keysym.sym == SDLK_RETURN) {
                    if (currentOption == 0) {
                        run = false;
                    }
                    if (currentOption == 1) {
                        return 0;
                    }
                }
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    return 0;
                }
            }
         }
    

        int additionalIter = 0;
        int pNum = 0;
        while (pNum < players.size() + 1 + additionalIter) {
            if (socket.receive(pData, 2048, received, pAddr, pPort) != Socket::NotReady) {
                if (pData[0] == 'n') {
                    string name;
                    for (int i = 1; i < strlen(pData); i++) {
                        name = name + pData[i];
                    }
                    if (players.size() <= 5) {
                        players.push_back(Player(random(10, 590), random(10, 430), pPort, pAddr, name));
                        socket.send("HELLO", 2048, pAddr, pPort);
                        additionalIter++;
                    }
                    else {
                        socket.send("GAME STARTED", 2048, pAddr, pPort);
                        additionalIter++;
                    }
                    
                }
                if ((string)pData == "GOOD BYE") {
                    int playerNum = getPlayerIndex(players, pPort);
                    players.erase(players.begin() + playerNum);
                }
                if ((string)pData == "I AM HERE") {
                    ports.push_back((int)pPort);
                    int playerNum = getPlayerIndex(players, pPort);
                    players[playerNum].triedToConn = 0;
                }
            }
            pNum ++;
        }

        for (int p = 1; p < players.size(); p++) {
            int pN = findPort(ports, players[p].port);
            if (pN == -1) {
                players[p].triedToConn++;
                if (players[p].triedToConn >= 300) {
                    players.erase(players.begin() + p);
                }
            }
            else {
                players[p].triedToConn = 0;
            }
        }

        json sjData = {
            {"players", {}}
        };
        string sData;
        for (auto &player:players) {
            json jData = {
                {"color1", player.color[0]},
                {"color2", player.color[1]},
                {"color3", player.color[2]},
                {"name", player.name}
            };
            sjData["players"].push_back(jData);
        }
        sData = sjData.dump();
        for (int i = 1; i < players.size(); i++) {
            socket.send(sData.c_str(), 2048, players[i].address, players[i].port);
        }

        SDL_SetRenderDrawColor(ren, 20, 20, 20, 255);
        SDL_RenderClear(ren);
        menu::draw_player_in_menu(ren, players, nameFont);
        menu::draw_ip_and_port(ren, ADDRESS.toString(), to_string(PORT), menuOptionFont);
        menu::draw_options(ren, options, optionsAddOns, currentOption, menuOptionFont);

        SDL_RenderPresent(ren);
        fEnd = SDL_GetTicks();
        fDelta = (fEnd - fStart);
        if (fDelta < fTime) {
            SDL_Delay(fTime - fDelta);
        }
    }

    for (int pl = 1; pl < players.size(); pl++) {
        socket.send("START GAME", 2048, players[pl].address, players[pl].port);
    }
    run = true;

    for (auto &p : players) {
        p.triedToConn = 0;
    }

    while (run) {
        fStart = SDL_GetTicks();
        ports.clear();

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                return -1;
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_w) {
                    upr = true;
                }
                if (e.key.keysym.sym == SDLK_s) {
                    dpr = true;
                }
                if (e.key.keysym.sym == SDLK_d) {
                    rpr = true;
                } 
                if (e.key.keysym.sym == SDLK_a) {
                    lpr = true;
                }
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    return 0;
                }
            }
            if (e.type == SDL_KEYUP) {
                if (e.key.keysym.sym == SDLK_w) {
                    upr = false;
                }
                if (e.key.keysym.sym == SDLK_s) {
                    dpr = false;
                }
                if (e.key.keysym.sym == SDLK_d) {
                    rpr = false;
                } 
                if (e.key.keysym.sym == SDLK_a) {
                    lpr = false;
                }
            }
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    lmpr = true;
                }
            }
            if (e.type == SDL_MOUSEBUTTONUP) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    lmpr = false;
                }
            }

            if (lmpr) {
                SDL_GetMouseState(&mPos[0], &mPos[1]);
            }

            if ((lpr && rpr) || (!lpr & !rpr)) {
                xbutton = 0;
            }
            else if (lpr) {
                xbutton = 1;
            }
            else {
                xbutton = 2;
            }

            if ((upr && dpr) || (!upr && !dpr)) {
                ybutton = 0;
            }
            else if (upr) {
                ybutton = 1;
            }
            else {
                ybutton = 2;
            }
        }

        int pIndex = 1;
        char rData[2048];
        size_t received;
        IpAddress rAddr;
        unsigned short rPort;
        if (!players[0].is_dead()){
            players[0].update(xbutton, ybutton, &bullets);
            if (lmpr) {
                players[0].shoot(&bullets, mPos);
            }
        }
        int additionalIter = 0;
        while (pIndex < players.size()+1+additionalIter) {
            if (socket.receive(rData, 2048, received, rAddr, rPort) == Socket::NotReady) {}
            else {
                if (rData[0] == 'n') {
                    string name;
                    for (int i = 1; i < strlen(rData); i++) {
                        name = name + rData[i];
                    }
                    socket.send("GAME STARTED", 2048, rAddr, rPort);
                    additionalIter++;
                }
                else if ((string)rData == "I AM HERE") {}
                else if ((string)rData == "GOOD BYE") {
                    int playerNum = getPlayerIndex(players, rPort);
                    players.erase(players.begin()+playerNum);
                }
                else {
                    json jData = json::parse(rData);
                    int playerNum = getPlayerIndex(players, rPort);
                    if (!players[playerNum].is_dead()) {
                        players[playerNum].update(jData["xbutton"], jData["ybutton"], &bullets);
                        if (jData["lmpr"]) {
                            int tempMPos[2] = {jData["mp1"], jData["mp2"]};
                            players[playerNum].shoot(&bullets, tempMPos);
                        }
                    }
                    ports.push_back(rPort);
                }
            }
            pIndex++;
        }

        for (int p = 1; p < players.size(); p++) {
            int pN = findPort(ports, players[p].port);
            if (pN == -1) {
                players[p].triedToConn++;
                players[p].update(0, 0, &bullets);
                if (players[p].triedToConn >= 300) {
                    players.erase(players.begin() + p);
                }
            }
            else {
                players[p].triedToConn = 0;
            }
        }

        int bIndex = 0;
        while (bIndex < bullets.size()) {
            if (!bullets[bIndex].update()){
                bullets.erase(bullets.begin()+bIndex);
                bIndex--;
            };
            bIndex++;
        }

        string sData;
        json sjData = {
            {"players", {}},
            {"bullets", {}}
        };
        for (auto &player : players) {
            json pData = {
                {"x", player.rect.x},
                {"y", player.rect.y},
                {"w", player.rect.w},
                {"h", player.rect.h},
                {"color1", player.color[0]},
                {"color2", player.color[1]},
                {"color3", player.color[2]},
                {"name", player.name}
            };
            sjData["players"].push_back(pData);
        }
        for (auto &bullet : bullets) {
            json bData = {
                {"x", bullet.rect.x},
                {"y", bullet.rect.y},
                {"w", bullet.rect.w},
                {"color1", bullet.color[0]},
                {"color2", bullet.color[1]},
                {"color3", bullet.color[2]}
            };
            sjData["bullets"].push_back(bData);
        }
        sData = sjData.dump();
        for (int player = 1; player < players.size(); player++) {
            if (socket.send(sData.c_str(), 2048, players[player].address, players[player].port) != Socket::Done) {
                cout << "Can't send data to: " << players[player].address << endl;
            }
        }

        SDL_SetRenderDrawColor(ren, 20, 20, 20, 255);
        SDL_RenderClear(ren);

        for (auto &bullet : bullets) {
            bullet.draw(ren);
        }
        for (auto &player : players) {
            player.draw(ren, nameFont);
        }

        SDL_RenderPresent(ren);

        fEnd = SDL_GetTicks();
        fDelta = (fEnd - fStart);
        if (fDelta < fTime) {
            SDL_Delay(fTime - fDelta);
        }
    }

    return 0;
}

int CLIENT(string addr, string port, string name) {
    IpAddress ADDRESS = addr;
    unsigned int PORT = stoi(port);
    UdpSocket socket;

    if (socket.bind(Socket::AnyPort) != Socket::Done) {
        cout << "Can't bind socket" << endl;
    }
    socket.setBlocking(false);
    name = "n" + name;
    socket.send(name.c_str(), 2048, ADDRESS, PORT);
    char rData[2048];
    size_t received;
    IpAddress rAddr;
    unsigned short rPort;

    int triedToConn = 0;
    SDL_Event e;
    int fTime = 1000 / 60, fDelta = 0;
    long int fStart, fEnd;
    bool run = true;

    while (run) {
        fStart = SDL_GetTicks();
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                return -1;
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    return 0;
                }
            }
        }

        if (socket.receive(rData, 2048, received, rAddr, rPort) != Socket::NotReady) {
            if ((string)rData == "GAME STARTED") {
                return 0;
            }
            if ((string)rData == "HELLO") {
                run = false;
            }
        }
        else {
            triedToConn++;
        }

        SDL_SetRenderDrawColor(ren, 20, 20, 20, 255);
        SDL_RenderClear(ren);
        menuOptionFont->render(ren, "Connecting to server...", 90, 200, NULL);

        SDL_RenderPresent(ren);

        fEnd = SDL_GetTicks();
        fDelta = (fEnd - fStart);
        if (fDelta < fTime) {
            SDL_Delay(fTime - fDelta);
        }

    }

    int xbutton, ybutton;
    bool rpr = false, lpr = false, upr = false, dpr = false, lmpr = false;
    int mPos[2] = {0, 0};
    run = true;

    vector<Player> players;
    int currentOption = 0;
    vector<string> options = {"BACK"};
    vector<string> optionsAddOns = {""};

    while (run) {
        fStart = SDL_GetTicks();
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                socket.send("GOOD BYE", 2048, ADDRESS, PORT);
                return -1;
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_RETURN) {
                    socket.send("GOOD BYE", 2048, ADDRESS, PORT);
                    return 0;
                }
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    socket.send("GOOD BYE", 2048, ADDRESS, PORT);
                    return 0;
                }
            }
        }

        SDL_SetRenderDrawColor(ren, 20, 20, 20, 255);
        SDL_RenderClear(ren);

        socket.send("I AM HERE", 2048, ADDRESS, PORT);
        if (socket.receive(rData, 2048, received, rAddr, rPort) != Socket::NotReady) {
            if ((string)rData == "START GAME") {
                run = false;
            }
            else {
                players.clear();
                json rjData = json::parse(rData);
                for (auto &pl : rjData["players"]) {
                    players.push_back(Player(0, 0, 0, "0.0.0.0", pl["name"]));
                    players[players.size() - 1].color[0] = pl["color1"];
                    players[players.size() - 1].color[1] = pl["color2"];
                    players[players.size() - 1].color[2] = pl["color2"];
                }
            }
        }
        else {
            triedToConn++;
        }
        if (triedToConn >= 180) {
            menuOptionFont->render(ren, "Trying to connect to server", 50, 5, NULL);
        }

        menu::draw_player_in_menu_client(ren, players, nameFont);
        menu::draw_options(ren, options, optionsAddOns, 0, menuOptionFont);
        SDL_RenderPresent(ren);

        fEnd = SDL_GetTicks();
        fDelta = (fEnd - fStart);
        if (fDelta < fTime) {
            SDL_Delay(fTime - fDelta);
        }

    }

    run = true;
    triedToConn = 0;
    vector<Bullet> bullets;


    while (run) {
        fStart = SDL_GetTicks();

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                socket.send("GOOD BYE", 2048, ADDRESS, PORT);
                return -1;
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_w) {
                    upr = true;
                }
                if (e.key.keysym.sym == SDLK_s) {
                    dpr = true;
                }
                if (e.key.keysym.sym == SDLK_d) {
                    rpr = true;
                } 
                if (e.key.keysym.sym == SDLK_a) {
                    lpr = true;
                }
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    socket.send("GOOD BYE", 2048, ADDRESS, PORT);
                    return 0;
                }
            }
            if (e.type == SDL_KEYUP) {
                if (e.key.keysym.sym == SDLK_w) {
                    upr = false;
                }
                if (e.key.keysym.sym == SDLK_s) {
                    dpr = false;
                }
                if (e.key.keysym.sym == SDLK_d) {
                    rpr = false;
                } 
                if (e.key.keysym.sym == SDLK_a) {
                    lpr = false;
                }
            }
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    lmpr = true;
                }
            }
            if (e.type == SDL_MOUSEBUTTONUP) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    lmpr = false;
                }
            }

            if (lmpr) {
                SDL_GetMouseState(&mPos[0], &mPos[1]);
            }

            if ((lpr && rpr) || (!lpr & !rpr)) {
                xbutton = 0;
            }
            else if (lpr) {
                xbutton = 1;
            }
            else {
                xbutton = 2;
            }

            if ((upr && dpr) || (!upr && !dpr)) {
                ybutton = 0;
            }
            else if (upr) {
                ybutton = 1;
            }
            else {
                ybutton = 2;
            }
        }
        if (run == false) {
            return 0;
        }

        json jsData = {
            {"xbutton", xbutton},
            {"ybutton", ybutton},
            {"lmpr", lmpr},
            {"mp1", mPos[0]},
            {"mp2", mPos[1]}
        };
        string sData = jsData.dump();
        socket.send(sData.c_str(), 2048, ADDRESS, PORT);


        SDL_SetRenderDrawColor(ren, 20, 20, 20, 255);
        SDL_RenderClear(ren);

        if (socket.receive(rData, 2048, received, rAddr, rPort) != Socket::NotReady){
            triedToConn = 0;
            players.clear();
            bullets.clear();
            json jData = json::parse(rData);
            for (auto &lPlayer: jData["players"]) {
                Player player(lPlayer["x"], lPlayer["y"], 0, rAddr, lPlayer["name"]);
                player.color[0] = lPlayer["color1"];
                player.color[1] = lPlayer["color2"];
                player.color[2] = lPlayer["color3"];
                players.push_back(player);
            }
            for (auto &lBullet: jData["bullets"]) {
                Bullet bullet(lBullet["x"], lBullet["y"], 0, 0, rPort);
                bullet.color[0] = lBullet["color1"];
                bullet.color[1] = lBullet["color2"];
                bullet.color[2] = lBullet["color3"];
                bullets.push_back(bullet);
            }
        }
        else {
            triedToConn++;
            if (triedToConn >= 180) {
                menuOptionFont->render(ren, "Trying to connect to server", 50, 5, NULL);
            }
        }

        for (auto &b: bullets) {
            b.draw(ren);
        }
        for (auto &p: players) {
            p.draw(ren, nameFont);
        }

        SDL_RenderPresent(ren);

        fEnd = SDL_GetTicks();
        fDelta = (fEnd - fStart);
        if (fDelta < fTime) {
            SDL_Delay(fTime - fDelta);
        }
    }

    return 0;
}

int main (int argc, char **argv) {
    init();
    while (true) {
        string playerName, port, address;
        int menuRes = menu::main_menu(ren, &address, &port, &playerName, menuOptionFont);
        if (menuRes == -1) {
            return 0;
        }
        if (menuRes == 1) {
            if (HOST(playerName) == -1) {
                break;
            };
        }
        if (menuRes == 2) {
           if (CLIENT(address, port, playerName) == -1) {
               break;
           };
        }
    }
    quit();
    return 0;
}
