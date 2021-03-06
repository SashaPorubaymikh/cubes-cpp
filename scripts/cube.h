#ifndef CUBE_H
#define CUBE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <stdlib.h>
#include <cmath>
#include <math.h>
#include <vector>
#include "bullet.h"
#include <SFML/Network.hpp>
#include <string>
#include <algorithm>
#include "sizeProperties.h"
#include "messages.h"


using sf::IpAddress;
using namespace std;

class Player {
    public:
        int color[3] = {0, 0, 0};
        SDL_Rect rect;
        int port = 0;
        IpAddress address;
        int tryedToConn = 0;
        bool dead = false;
        string name;
        int triedToConn = 0;
        bool rPressed = false;
        bool enterPressed = false;
        unsigned int selfIdentifier;
        bool winner = false;
        short int lifes = 3;
        float slipCoeff = 0.02;
        SDL_Texture *nameTexture = NULL;


        Player(int x, int y, int port, IpAddress address, string name) {
            this->rect = {x, y, playerSize, playerSize};

            int color_index = rand()%9;
            this->next_color[0] = COLORS[color_index][0];
            this->next_color[1] = COLORS[color_index][1];
            this->next_color[2] = COLORS[color_index][2];

            color_index = rand()%9;
            this->color[0] = COLORS[color_index][0];
            this->color[1] = COLORS[color_index][1];
            this->color[2] = COLORS[color_index][2];
            this->port = port;
            this->address = address;
            this->name = name;

            selfIdentifier = rand()%4000000000;
            this->lifes = 3;
        }
        ~Player() {
            SDL_DestroyTexture(this->nameTexture);
            this->nameTexture = NULL;
        }

        void togleReady() {
            if (this->rPressed) {this->rPressed = false;}
            else {this->rPressed = true;}
        }

        void update(int xbutton, int ybutton, vector<Bullet> *bullets, vector<Player> &players, Messages *messages) {
            if (this->shot_timer > 0) {
                this->shot_timer --;
            }
            update_colors();
            if (xbutton == 1) {
                xspeed -= (-this->weight * 9.8 * this->slipCoeff + this->acceleration) / this->weight;
                if (this->xspeed < -this->max_speed) {
                    this->xspeed = -this->max_speed;
                }
            }
            else if (xbutton == 2) {
                xspeed += (-this->weight * 9.8 * this->slipCoeff + this->acceleration) / this->weight;
                if (this->xspeed > this->max_speed) {
                    this->xspeed = this->max_speed;
                }
            }
            else if (xbutton == 0) {
                if (this->xspeed < 0) {
                    this->xspeed += (this->weight * 9.8 * this->slipCoeff) / this->weight;
                }
                if (this->xspeed > 0) {
                    this->xspeed -= (this->weight * 9.8 * this->slipCoeff) / this->weight;
                }
                if (abs(this->xspeed) < 0.5) {
                    this->xspeed = 0;
                }
            }
            if (ybutton == 1) {
                this->yspeed -= (-this->weight * 9.8 * this->slipCoeff + this->acceleration) / this->weight;
                if (this->yspeed < -this->max_speed) {
                    this->yspeed = -this->max_speed;
                }
            }

            else if (ybutton == 2) {
                this->yspeed += (-this->weight * 9.8 * this->slipCoeff + this->acceleration) / this->weight;
                if (this->yspeed > this->max_speed) {
                    this->yspeed = this->max_speed;
                }
            }
            else if (ybutton == 0) {
                if (this->yspeed < 0) {
                    this->yspeed += (this->weight * 9.8 * this->slipCoeff) / this->weight;
                }
                if (this->yspeed > 0) {
                    this->yspeed -= (this->weight * 9.8 * this->slipCoeff) / this->weight;
                }
                if (abs(this->yspeed) < 0.5) {
                    this->yspeed = 0;
                }
            }

            this->collide_wall();
            this->collide(players);

            this->rect.x += round(this->xspeed);
            this->rect.y += round(this->yspeed);

            SDL_Rect tempRect;
            for (auto &bullet: *bullets) {
                if (SDL_IntersectRect(&this->rect, &bullet.rect, &tempRect) == SDL_TRUE && (this->selfIdentifier != bullet.ownerIdentifier)) {
                    string killerName;
                    for (auto &p: players) {
                        if (p.selfIdentifier == bullet.ownerIdentifier) {
                            killerName = p.name;
                        }
                    }
                    
                    this->die();
                    this->xspeed += bullet.xspeed / 4;
                    this->yspeed += bullet.yspeed / 4;
                    if (this->lifes <= 0) {
                        messages->append(killerName + " killed " + this->name);
                    }
                    bullet.die();
                }
            }
        }

        void update_colors() {
            if (abs(this->next_color[0] - this->color[0]) < 5 && abs(this->next_color[1] - this->color[1]) < 5 && \
            abs(this->next_color[2] - this->color[2]) < 5){
                int color_index = rand()%9;
                this->next_color[0] = COLORS[color_index][0];
                this->next_color[1] = COLORS[color_index][1];
                this->next_color[2] = COLORS[color_index][2];
            }

            for (int i = 0; i < 3; i++) {
                if (this->next_color[i] < this->color[i]) {
                    this->color[i] -= 4;
                    if (this->color[i] < 0) {
                        this->color[i] = 0;
                    }
                }
                else if (this->next_color[i] > this->color[i]) {
                    this->color[i] += 4;
                    if (this->color[i] > 255) {
                        this->color[i] = 255;
                    }
                }
            }   
        }

        void collide_wall() {
            if (this->rect.x + this->rect.w >= 1366) {
                this->xspeed = -abs(this->xspeed);
            }
            if (this->rect.x <= 0) {
                this->xspeed = abs(this->xspeed);
            }

            if (this->rect.y + this->rect.h >= 768) {
                this->yspeed = -abs(this->yspeed);
            }
            if (this->rect.y <= 0) {
                this->yspeed = abs(this->yspeed);
            }
        }

        void collide(vector<Player> &players) {
            for (auto &player: players) {
                if (player.port != this->port) {
                    SDL_Rect tempRect;
                    if (SDL_IntersectRect(&player.rect, &this->rect, &tempRect) == SDL_TRUE) {
                        int max_w = max(player.rect.x + player.rect.w - this->rect.x, this->rect.x + this->rect.w - player.rect.x);
                        int max_h = max(player.rect.y + player.rect.h - this->rect.y, this->rect.y + this->rect.h - player.rect.y);
                        if (max_w > max_h) {
                            if (abs(this->rect.x + this->rect.w - player.rect.x) < abs(player.rect.x + player.rect.w - this->rect.x)) {
                                this->rect.x = player.rect.x - this->rect.w;
                                this->xspeed = -abs(abs(this->xspeed) + abs(player.xspeed)) / 2;
                                player.xspeed = abs(abs(this->xspeed) + abs(player.xspeed)) / 2;
                                if (this->rect.x < 0) {
                                    this->rect.x = 0;
                                    player.rect.x = this->rect.w;
                                }
                            }
                            else if (abs(this->rect.x + this->rect.w - player.rect.x) > abs(player.rect.x + player.rect.w - this->rect.x)) {
                                this->rect.x = player.rect.x + player.rect.w;
                                this->xspeed = abs(abs(this->xspeed) + abs(player.xspeed)) / 2;
                                player.xspeed = -abs(abs(this->xspeed) + abs(player.xspeed)) / 2;
                                if (this->rect.x + this->rect.w > 1366) {
                                    this->rect.x = 1366 - this->rect.w;
                                    player.rect.x = this->rect.x - player.rect.w;
                                }
                            }
                        }
                        else if (max_h > max_w) {
                            if (abs(this->rect.y + this->rect.h - player.rect.y) < abs(player.rect.y + player.rect.h - this->rect.y)) {
                                this->rect.y = player.rect.y - this->rect.h;
                                this->yspeed = -abs(abs(this->yspeed) + abs(player.yspeed)) / 2;
                                player.yspeed = abs(abs(this->yspeed) + abs(player.yspeed)) / 2;
                                if (this->rect.y < 0) {
                                    this->rect.y = 0;
                                    player.rect.y = this->rect.h;
                                }
                            }
                            else if (abs(this->rect.y + this->rect.h - player.rect.y) > abs(player.rect.y + player.rect.h - this->rect.y)) {
                                this->rect.y = player.rect.y + player.rect.h;
                                this->yspeed = abs(abs(this->yspeed) + abs(player.yspeed)) / 2;
                                player.yspeed = -abs(abs(this->yspeed) + abs(player.yspeed)) / 2;
                                if (this->rect.y + this->rect.h > 768) {
                                    this->rect.y = 768 - this->rect.h;
                                    player.rect.y = this->rect.y - player.rect.h;
                                }
                            }
                        }
                    }
                }
            }
        }

        void shoot(vector<Bullet> *bullets, int mouse_pos[2], float SCREEN_DIFF[2], bool change){
            if (this->shot_timer == 0) {
                int mPX = 0, mPY = 0;
                if (change) {
                    mPX = mouse_pos[0] / SCREEN_DIFF[0];
                    mPY = mouse_pos[1] / SCREEN_DIFF[1];
                }
                else {
                    mPX = mouse_pos[0];
                    mPY = mouse_pos[1];
                }

                float xdiff = this->rect.x + (this->rect.w / 2) - mPX;
                float ydiff = this->rect.y + (this->rect.h / 2) - mPY;
                int diff = round(sqrt(pow(xdiff, 2) + pow(ydiff, 2)) / 10);
                bullets->push_back(Bullet(this->rect.x + (this->rect.w / 2) - 10 / 2, this->rect.y + (this->rect.h / 2) - 10 / 2, \
                -round(xdiff / diff), -round(ydiff / diff), this->selfIdentifier));
                this->shot_timer = 30;
                this->xspeed += (xdiff / diff) / 4;
                this->yspeed += (ydiff / diff) / 4;
            }
        }

        void draw(SDL_Renderer *ren, TTF_Font *nameFont, float SCREEN_DIFF[2]) {
            if (!this->dead) {
                SDL_SetRenderDrawColor(ren, this->color[0], this->color[1], this->color[2], 255);

                SDL_Rect drawRect;
                drawRect.x = (float)this->rect.x * SCREEN_DIFF[0];
                drawRect.y = (float)this->rect.y * SCREEN_DIFF[1];
                drawRect.w = this->rect.w * round(SCREEN_DIFF[0]);
                drawRect.h = this->rect.h * round(SCREEN_DIFF[0]);

                SDL_RenderFillRect(ren, &drawRect);
                this->draw_name(ren, nameFont, drawRect);
            }
        }

        void draw_in_menu(SDL_Renderer *ren, TTF_Font *nameFont, int x, int y, float SCREEN_DIFF[2]) {
            SDL_SetRenderDrawColor(ren, this->color[0], this->color[1], this->color[2], 255);
            SDL_Rect drawRect;

            drawRect.w = this->rect.w * round(SCREEN_DIFF[0]);
            drawRect.h = this->rect.h * round(SCREEN_DIFF[0]);
            drawRect.x = x * SCREEN_DIFF[0];
            drawRect.y = y * SCREEN_DIFF[1];


            SDL_RenderFillRect(ren, &drawRect);
            this->draw_name(ren, nameFont, drawRect);
            this->draw_ready_in_menu(ren, nameFont, drawRect);
        }

        void draw_name(SDL_Renderer *ren, TTF_Font *nameFont, SDL_Rect rect) {
            if (nameFont == NULL) {
                cout << SDL_GetError() << endl;
            }
            SDL_Color nameColor = {(Uint8)this->color[0], (Uint8)this->color[1], (Uint8)this->color[2]};

            SDL_Surface *tempSurface = TTF_RenderText_Blended(nameFont, this->name.c_str(), nameColor);
            if (tempSurface == NULL) {
                cout << "ERROR: " << SDL_GetError() << " " << TTF_GetError() << endl;
            }

            this->nameTexture = SDL_CreateTextureFromSurface(ren, tempSurface);
            if (this->nameTexture == NULL) {
                cout << "Error: " << SDL_GetError() << endl;
            }

            

            SDL_Rect nameRect;
            nameRect.x = rect.x + (rect.w / 2) - (tempSurface->w / 2);
            nameRect.y = rect.y - 20;
            nameRect.w = tempSurface->w; nameRect.h = tempSurface->h;

            SDL_RenderCopy(ren, this->nameTexture, NULL, &nameRect);
            SDL_FreeSurface(tempSurface);
            tempSurface = NULL;
        }

        void draw_ready_in_menu(SDL_Renderer *ren, TTF_Font *nameFont, SDL_Rect rect) {
            SDL_Color nameColor = {(Uint8)this->color[0], (Uint8)this->color[1], (Uint8)this->color[2]};

            SDL_Surface *tempSurface;
            if (this->rPressed) {
                tempSurface = TTF_RenderText_Blended(nameFont, "READY", nameColor);
                if (tempSurface == NULL) {
                    cout << "TEXT RENDER ERROR: " << SDL_GetError() << " " << TTF_GetError() << endl;
                }
            }
            else {
                SDL_Color tempColor = {120, 120, 120};
                tempSurface = TTF_RenderText_Blended(nameFont, "NOT READY", tempColor);
                if (tempSurface == NULL) {
                    cout << "TEXT RENDER ERROR: " << SDL_GetError() << " " << TTF_GetError() << endl;
                }
            }
            
            if (tempSurface == NULL) {
                cout << "ERROR: " << SDL_GetError() << " " << TTF_GetError() << endl;
            }
            this->nameTexture = SDL_CreateTextureFromSurface(ren, tempSurface);
            if (this->nameTexture == NULL) {
                cout << "Error: " << SDL_GetError() << endl;
            }
            

            SDL_Rect nameRect;
            nameRect.x = rect.x + (rect.w / 2) - (tempSurface->w / 2);
            nameRect.y = rect.y + rect.h + 5;
            nameRect.w = tempSurface->w; nameRect.h = tempSurface->h;

            SDL_RenderCopy(ren, this->nameTexture, NULL, &nameRect);
            SDL_FreeSurface(tempSurface);
            tempSurface = NULL;
        }

        void die() {
            this->lifes--;
            if (this->lifes <= 0) {
                this->dead = true;
                this->rect.x = -100;
                this->rect.y = -100;
            }
        }
        
        bool is_dead() {
            return this->dead;
        }

    private:
        
        int next_color[3] = {0, 0, 0};
        int max_speed = 5;
        float xspeed = 0, yspeed = 0;
        int acceleration = 10;
        int weight = 10;
        int shot_timer = 80 + rand()%40;
};

#endif
