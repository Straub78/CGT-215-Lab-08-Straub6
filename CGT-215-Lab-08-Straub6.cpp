// CGT-215-Lab-08-Straub6.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFPhysics.h>

using namespace std;
using namespace sf;
using namespace sfp;

const int WINDOW_W = 800;
const int WINDOW_H = 600;
const int START_ARROWS = 5;
const int DUCK_SPAWN_MS = 1500;     // time between ducks
const float DUCK_SPEED = 0.15f;     // pixels per millisecond
const float DUCK_Y = 70.0f;         // row the ducks fly along
const float ARROW_SPEED = 0.6f;     // pixels per millisecond

void LoadTex(Texture& tex, string filename) {
    if (!tex.loadFromFile(filename)) {
        cout << "Could not load " << filename << endl;
        exit(1);
    }
}

void LoadFont(Font& font, string filename) {
    if (!font.loadFromFile(filename)) {
        cout << "Could not load " << filename << endl;
        exit(1);
    }
}

int main()
{
    RenderWindow window(VideoMode(WINDOW_W, WINDOW_H), "Duck Hunter");
    World world(Vector2f(0, 0));   // no gravity

    int score(0);
    int arrows(START_ARROWS);

    // ---------- Crossbow (fixed at bottom center) ----------
    Texture cbowTex;
    LoadTex(cbowTex, "Lab08Images/crossbow.png");
    PhysicsSprite crossBow;
    crossBow.setTexture(cbowTex);
    Vector2f cbSize = crossBow.getSize();
    crossBow.setCenter(Vector2f(WINDOW_W / 2.0f, WINDOW_H - (cbSize.y / 2)));

    // ---------- Arrow (one at a time) ----------
    Texture arrowTex;
    LoadTex(arrowTex, "Lab08Images/arrow.png");
    PhysicsSprite arrow;
    arrow.setTexture(arrowTex);
    bool drawingArrow(false);

    // ---------- Top wall: removes the arrow when it misses ----------
    PhysicsRectangle top;
    top.setSize(Vector2f(WINDOW_W, 10));
    top.setCenter(Vector2f(WINDOW_W / 2.0f, 5));
    top.setStatic(true);
    world.AddPhysicsBody(top);
    top.onCollision = [&drawingArrow, &world, &arrow](PhysicsBodyCollisionResult result) {
        if (result.object2 == arrow) {
            drawingArrow = false;
            world.RemovePhysicsBody(arrow);
        }
        };

    // ---------- Right wall: removes ducks that fly past the screen ----------
    // Placed just off screen so ducks fully exit before being removed.
    PhysicsRectangle rightWall;
    rightWall.setSize(Vector2f(20, WINDOW_H));
    rightWall.setCenter(Vector2f(WINDOW_W + 100, WINDOW_H / 2.0f));
    rightWall.setStatic(true);
    world.AddPhysicsBody(rightWall);

    // ---------- Ducks ----------
    Texture duckTex;
    LoadTex(duckTex, "Lab08Images/red_balloon.png");
    PhysicsShapeList<PhysicsSprite> ducks;

    // ---------- Text ----------
    Font fnt;
    LoadFont(fnt, "arial.ttf");

    Text scoreText;
    scoreText.setFont(fnt);
    Text arrowCountText;
    arrowCountText.setFont(fnt);

    // ---------- Main loop ----------
    Clock clock;
    Time lastTime(clock.getElapsedTime());
    int msUntilNextDuck(0);   // 0 => spawn a duck right away

    // Keep running while the player has arrows OR the last arrow is still flying
    while (arrows > 0 || drawingArrow) {
        Time currentTime(clock.getElapsedTime());
        Time deltaTime(currentTime - lastTime);
        int deltaTimeMS(deltaTime.asMilliseconds());

        if (deltaTimeMS > 0) {
            lastTime = currentTime;
            world.UpdatePhysics(deltaTimeMS);

            // Duck spawn countdown
            msUntilNextDuck -= deltaTimeMS;
            if (msUntilNextDuck <= 0) {
                msUntilNextDuck = DUCK_SPAWN_MS;

                PhysicsSprite& duck = ducks.Create();
                duck.setTexture(duckTex);
                Vector2f dSize = duck.getSize();
                duck.setCenter(Vector2f(-(dSize.x / 2), DUCK_Y));   // starts off screen left
                duck.setVelocity(Vector2f(DUCK_SPEED, 0));
                world.AddPhysicsBody(duck);

                duck.onCollision = [&drawingArrow, &world, &arrow, &duck, &ducks, &rightWall, &score]
                (PhysicsBodyCollisionResult result) {
                    if (result.object2 == arrow) {
                        // Arrow hit the duck: remove both, add score
                        drawingArrow = false;
                        world.RemovePhysicsBody(arrow);
                        world.RemovePhysicsBody(duck);
                        ducks.QueueRemove(duck);
                        score += 10;
                    }
                    else if (result.object2 == rightWall) {
                        // Duck flew off the right side
                        world.RemovePhysicsBody(duck);
                        ducks.QueueRemove(duck);
                    }
                    };
            }
        }

        // Fire the crossbow (one bolt at a time)
        if (Keyboard::isKeyPressed(Keyboard::Space) && !drawingArrow && arrows > 0) {
            drawingArrow = true;
            arrows--;
            arrow.setCenter(crossBow.getCenter());
            arrow.setVelocity(Vector2f(0, -ARROW_SPEED));
            world.AddPhysicsBody(arrow);
        }

        // ---------- Draw ----------
        window.clear(Color(20, 30, 60));
        window.draw(crossBow);
        if (drawingArrow) {
            window.draw(arrow);
        }

        ducks.DoRemovals();
        for (PhysicsShape& duck : ducks) {
            window.draw((PhysicsSprite&)duck);
        }

        scoreText.setString(to_string(score));
        FloatRect sb = scoreText.getGlobalBounds();
        scoreText.setPosition(Vector2f(WINDOW_W - sb.width - 15, WINDOW_H - sb.height - 25));
        window.draw(scoreText);

        arrowCountText.setString(to_string(arrows));
        FloatRect ab = arrowCountText.getGlobalBounds();
        arrowCountText.setPosition(Vector2f(15, WINDOW_H - ab.height - 25));
        window.draw(arrowCountText);

        window.display();
    }

    // ---------- Game over: leave last frame up, add GAME OVER ----------
    Text gameOverText;
    gameOverText.setFont(fnt);
    gameOverText.setString("GAME OVER");
    gameOverText.setCharacterSize(60);
    FloatRect gb = gameOverText.getGlobalBounds();
    gameOverText.setPosition(Vector2f((WINDOW_W - gb.width) / 2, (WINDOW_H - gb.height) / 2));
    window.draw(gameOverText);
    window.display();

    // Wait for the player to release space (if held), then press it again
    while (Keyboard::isKeyPressed(Keyboard::Space)) {}
    while (!Keyboard::isKeyPressed(Keyboard::Space)) {}

    return 0;
}
