#include "engine.h"

enum state {start, play, over};
state screen;

// Colors
color originalFill, hoverFill, pressFill;

Engine::Engine() : keys() {
    this->initWindow();
    this->initShaders();
    this->initShapes();

    originalFill = {1, 0, 0, 1};
    hoverFill.vec = originalFill.vec + vec4{0.5, 0.5, 0.5, 0};
    pressFill.vec = originalFill.vec - vec4{0.5, 0.5, 0.5, 0};
}

Engine::~Engine() {}

unsigned int Engine::initWindow(bool debug) {
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, false);

    window = glfwCreateWindow(width, height, "engine", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    // OpenGL configuration
    glViewport(0, 0, width, height);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glfwSwapInterval(1);

    return 0;
}

void Engine::initShaders() {
    // load shader manager
    shaderManager = make_unique<ShaderManager>();

    // Load shader into shader manager and retrieve it
    shapeShader = this->shaderManager->loadShader("../res/shaders/shape.vert", "../res/shaders/shape.frag",  nullptr, "shape");

    // Configure text shader and renderer
    textShader = shaderManager->loadShader("../res/shaders/text.vert", "../res/shaders/text.frag", nullptr, "text");
    fontRenderer = make_unique<FontRenderer>(shaderManager->getShader("text"), "../res/fonts/MxPlus_IBM_BIOS.ttf", 24);

    // Set uniforms
    textShader.use().setVector2f("vertex", vec4(100, 100, .5, .5));
    shapeShader.use().setMatrix4("projection", this->PROJECTION);
}

void Engine::initShapes() {
    // red spawn button centered in the top left corner
    spawnButton = make_unique<Rect>(shapeShader, vec2{width/2,height/2}, vec2{100, 50}, color{1, 0, 0, 1});
}

void Engine::processInput() {
    glfwPollEvents();

    // Set keys to true if pressed, false if released
    for (int key = 0; key < 1024; ++key) {
        if (glfwGetKey(window, key) == GLFW_PRESS)
            keys[key] = true;
        else if (glfwGetKey(window, key) == GLFW_RELEASE)
            keys[key] = false;
    }

    // Close window if escape key is pressed
    if (keys[GLFW_KEY_ESCAPE])
        glfwSetWindowShouldClose(window, true);

    // Mouse position saved to check for collisions
    glfwGetCursorPos(window, &MouseX, &MouseY);

    // TODO: If we're in the start screen and the user presses s, change screen to play
    // Hint: The index is GLFW_KEY_S
    if (screen == start && keys[GLFW_KEY_S])
        screen = play;

    // TODO: If we're in the play screen and an arrow key is pressed, move the spawnButton
    // Hint: one of the indices is GLFW_KEY_UP
    // TODO: Make sure the spawnButton cannot go off the screen
    if (screen == play) {
        if (keys[GLFW_KEY_UP] && spawnButton->getTop() < height)
            spawnButton->moveY(1.0);
        if (keys[GLFW_KEY_DOWN] && spawnButton->getBottom() > 0)
            spawnButton->moveY(-1.0);
        if (keys[GLFW_KEY_LEFT] && spawnButton->getLeft() > 0)
            spawnButton->moveX(-1.0);
        if (keys[GLFW_KEY_RIGHT] && spawnButton->getRight() < width)
            spawnButton->moveX(1.0);
    }

    // Mouse position is inverted because the origin of the window is in the top left corner
    MouseY = height - MouseY; // Invert y-axis of mouse position
    bool buttonOverlapsMouse = spawnButton->isOverlapping(vec2(MouseX, MouseY));
    bool mousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    // TODO: When in play screen, if the user hovers or clicks on the button then change the spawnButton's color
    // Hint: look at the color objects declared at the top of this file

    // TODO: When in play screen, if the button was released then spawn confetti
    // Hint: the button was released if it was pressed last frame and is not pressed now
    // TODO: Make sure the spawn button is its original color when the user is not hovering or clicking on it.
    if (screen == play) {
        if (buttonOverlapsMouse) {
            if (mousePressed) {
                spawnButton->setColor(pressFill);
            } else {
                spawnButton->setColor(hoverFill);
            }
        } else {
            spawnButton->setColor(originalFill);
        }
    }

    if (screen == play && mousePressedLastFrame && !mousePressed) {
        spawnConfetti();
    }

    // Save mousePressed for next frame
    mousePressedLastFrame = mousePressed;

}


void Engine::update() {
    // Calculate delta time
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // TODO: End the game when the user spawns 100 confetti
    // If the size of the confetti vector reaches 100, change screen to over
    if (screen == play && confetti.size() >= 100) {
        screen = over;
    }
}

void Engine::render() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Set background color
    glClear(GL_COLOR_BUFFER_BIT);

    // Set shader to use for all shapes
    shapeShader.use();

    // Render differently depending on screen
    switch (screen) {
        case start: {
            string message = "Press s to start";
            // (12 * message.length()) is the offset to center text.
            // 12 pixels is the width of each character scaled by 1.
            this->fontRenderer->renderText(message, width/2 - (12 * message.length()), height/2, 1, vec3{1, 1, 1});
            break;
        }
        case play: {
            // TODO: call setUniforms and draw on the spawnButton and all of the confetti pieces
            //  Hint: make sure you draw the spawn button after the confetti to make it appear on top
            // Render font on top of spawn button
            // Render the confetti pieces


            for (const auto& confetto : confetti) {
                confetto->setUniforms();
                confetto->draw();
            }
            spawnButton->setUniforms();
            spawnButton->draw();

            fontRenderer->renderText("Spawn", spawnButton->getPos().x - 30, spawnButton->getPos().y - 5, 0.5, vec3{1, 1, 1});


            break;
        }
        case over: {
            string message = "You win!";
            // Display the message on the screen
            this->fontRenderer->renderText(message, width/2 - (12 * message.length()), height/2, 1, vec3{1, 1, 1});
            break;
        }
    }

    glfwSwapBuffers(window);
}

int x = 1;
int y = 1;

int increase = 2;

void Engine::spawnConfetti() {
    vec2 pos = {rand() % (int)width, rand() % (int)height};
    // TODO: Make each piece of confetti a different size, getting bigger with each spawn.
    //  The smallest should be a square of size 1 and the biggest should be a square of size 100

    // Increase the size by the specified amount
    x = x + increase;
    y = y + increase;

    // Ensure the size doesn't exceed the maximum size (100)
    if (x > 100) {
        x = 100;
    }
    if (y > 100) {
        y = 100;
    }

    vec2 size = {x, y};
    color color = {float(rand() % 10 / 10.0), float(rand() % 10 / 10.0), float(rand() % 10 / 10.0), 1.0f};
    confetti.push_back(make_unique<Rect>(shapeShader, pos, size, color));
}

bool Engine::shouldClose() {
    return glfwWindowShouldClose(window);
}