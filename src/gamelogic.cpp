#include <chrono>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <SFML/Audio/SoundBuffer.hpp>
#include <utilities/shader.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#include <utilities/timeutils.h>
#include <utilities/mesh.h>
#include <utilities/shapes.h>
#include <utilities/glutils.h>
#include <SFML/Audio/Sound.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fmt/format.h>
#include "gamelogic.h"
#include "sceneGraph.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include "utilities/imageLoader.hpp"
#include "utilities/glfont.h"

enum KeyFrameAction {
    BOTTOM, TOP
};

#include <timestamps.h>

unsigned int currentKeyFrame = 0;
unsigned int previousKeyFrame = 0;

SceneNode* rootNode;
SceneNode* boxNode;
SceneNode* ballNode;
SceneNode* box2Node;
SceneNode* cactusNode;

unsigned int FBO;
unsigned int RBO;
unsigned int rectVAO, rectVBO;
unsigned int framebufferTexture;
unsigned int normalTexture;
unsigned int depthTexture;

//Light Nodes
SceneNode* LightNode;

double ballRadius = 3.0f;

// These are heap allocated, because they should not be initialised at the start of the program
sf::SoundBuffer* buffer;
Gloom::Shader* shader;
Gloom::Shader* shaderPP;
sf::Sound* sound;

const glm::vec3 boxDimensions(180, 90, 90);
const glm::vec3 box2Dimensions(10, 10, 10);

float rectangleVertices[] = {
    // Coords       // texCoords
    -1.0f, -1.0f,   0.0f, 0.0f,
    1.0f, -1.0f,   1.0f, 0.0f,
    -1.0f, 1.0f,   0.0f, 1.0f,
    
    1.0f, -1.0f,    1.0f, 0.0f,
    1.0f, 1.0f,   1.0f, 1.0f,
    -1.0f, 1.0f,   0.0f, 1.0f
};

glm::vec3 ballPosition(0, boxDimensions.z / 2, boxDimensions.z / 2);
// glm::vec3 ballDirection(1, 1, 0.2f);

glm::vec3 cameraPosition;

CommandLineOptions options;

bool hasStarted        = false;
bool hasLost           = false;
bool jumpedToNextFrame = false;
bool isPaused          = false;

bool mouseLeftPressed   = false;
bool mouseLeftReleased  = false;
bool mouseRightPressed  = false;
bool mouseRightReleased = false;

// Modify if you want the music to start further on in the track. Measured in seconds.
const float debug_startTime = 0;
double totalElapsedTime = debug_startTime;
double gameElapsedTime = debug_startTime;

double mouseSensitivity = 1.0;
double lastMouseX = windowWidth / 2;
double lastMouseY = windowHeight / 2;
void mouseCallback(GLFWwindow* window, double x, double y) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

    double deltaX = x - lastMouseX;
    double deltaY = y - lastMouseY;
    glfwSetCursorPos(window, windowWidth / 2, windowHeight / 2);
}

void initGame(GLFWwindow* window, CommandLineOptions gameOptions) {
    buffer = new sf::SoundBuffer();
    if (!buffer->loadFromFile("../res/Hall of the Mountain King.ogg")) {
        return;
    }

    options = gameOptions;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetCursorPosCallback(window, mouseCallback);

    shader = new Gloom::Shader();
    shader->makeBasicShader("../res/shaders/simple.vert", "../res/shaders/simple.frag");
    shader->activate();

    // Post-processing shader
    shaderPP = new Gloom::Shader();
    shaderPP->makeBasicShader("../res/shaders/framebuffer.vert", "../res/shaders/framebuffer.frag");
    shaderPP->activate();
    glUniform1i(shaderPP->getUniformFromName("screenTexture"), 0);
    glUniform1i(shaderPP->getUniformFromName("normalTexture"), 1);
    glUniform1i(shaderPP->getUniformFromName("depthTexture"), 2);

    // Create meshes
    Mesh box = cube(boxDimensions, glm::vec2(90), true, true);
    Mesh box2 = cube(box2Dimensions, glm::vec2(90), true, false);
    Mesh sphere = generateSphere(1.0, 40, 40);
    Mesh cactus = loadModel("../res/models/Cactus1.gltf");

    // Fill buffers
    unsigned int ballVAO = generateBuffer(sphere);
    unsigned int boxVAO  = generateBuffer(box);
    unsigned int box2VAO  = generateBuffer(box2);
    unsigned int cactus1VAO = generateBuffer(cactus);

    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectVBO);
    glBindVertexArray(rectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangleVertices), &rectangleVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
   
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);


    // Post-processing buffer
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    glGenTextures(1, &framebufferTexture);
	glBindTexture(GL_TEXTURE_2D, framebufferTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, windowWidth, windowHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferTexture, 0);

    // Generate normaltexture and depthtexture for post-processing
    glGenTextures(1, &normalTexture);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, windowWidth, windowHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normalTexture, 0);

    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, windowWidth, windowHeight, 0,  GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, depthTexture, 0);

    GLenum attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    glGenRenderbuffers(1, &RBO);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, windowWidth, windowHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

    auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer error: " << fboStatus << std::endl;


    // Construct scene
    rootNode = createSceneNode();
    boxNode  = createSceneNode();

    ballNode = createSceneNode();
    box2Node = createSceneNode();
    cactusNode = createSceneNode();
    cactusNode->scale = glm::vec3(10.0f);
    LightNode = createSceneNode();
    LightNode->nodeType = POINT_LIGHT;
    LightNode->id = 0;
    LightNode->color = glm::vec3(255.0, 255.0, 255.0 );


    rootNode->children.push_back(boxNode);
    rootNode->children.push_back(box2Node);
    rootNode->children.push_back(cactusNode);
    rootNode->children.push_back(ballNode);
    rootNode->children.push_back(LightNode);

    boxNode->vertexArrayObjectID  = boxVAO;
    boxNode->VAOIndexCount        = box.indices.size();

    ballNode->vertexArrayObjectID = ballVAO;
    ballNode->VAOIndexCount       = sphere.indices.size();

    box2Node->vertexArrayObjectID = box2VAO;
    box2Node->VAOIndexCount       = box2.indices.size();

    cactusNode->vertexArrayObjectID = cactus1VAO;
    cactusNode->VAOIndexCount       = cactus.indices.size();


    getTimeDeltaSeconds();

    std::cout << fmt::format("Initialized scene with {} SceneNodes.", totalChildren(rootNode)) << std::endl;

    std::cout << "Ready. Click to start!" << std::endl;
}

void updateFrame(GLFWwindow* window) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    double timeDelta = getTimeDeltaSeconds();

    // const float ballBottomY = boxNode->position.y - (boxDimensions.y/2) + ballRadius + padDimensions.y;
    const float ballTopY    = boxNode->position.y + (boxDimensions.y/2) - ballRadius;
    // const float BallVerticalTravelDistance = ballTopY - ballBottomY;

    const float cameraWallOffset = 30; // Arbitrary addition to prevent ball from going too much into camera

    const float ballMinX = boxNode->position.x - (boxDimensions.x/2) + ballRadius;
    const float ballMaxX = boxNode->position.x + (boxDimensions.x/2) - ballRadius;
    const float ballMinZ = boxNode->position.z - (boxDimensions.z/2) + ballRadius;
    const float ballMaxZ = boxNode->position.z + (boxDimensions.z/2) - ballRadius - cameraWallOffset;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1)) {
        mouseLeftPressed = true;
        mouseLeftReleased = false;
    } else {
        mouseLeftReleased = mouseLeftPressed;
        mouseLeftPressed = false;
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2)) {
        mouseRightPressed = true;
        mouseRightReleased = false;
    } else {
        mouseRightReleased = mouseRightPressed;
        mouseRightPressed = false;
    }

    if(!hasStarted) {
        if (mouseLeftPressed) {
            if (options.enableMusic) {
                sound = new sf::Sound();
                sound->setBuffer(*buffer);
                sf::Time startTime = sf::seconds(debug_startTime);
                sound->setPlayingOffset(startTime);
                sound->play();
            }
            totalElapsedTime = debug_startTime;
            gameElapsedTime = debug_startTime;
            hasStarted = true;
        }
    } else {
        totalElapsedTime += timeDelta;
        if(hasLost) {
            if (mouseLeftReleased) {
                hasLost = false;
                hasStarted = false;
                currentKeyFrame = 0;
                previousKeyFrame = 0;
            }
        } else if (isPaused) {
            if (mouseRightReleased) {
                isPaused = false;
                if (options.enableMusic) {
                    sound->play();
                }
            }
        } else {
            gameElapsedTime += timeDelta;
            if (mouseRightReleased) {
                isPaused = true;
                if (options.enableMusic) {
                    sound->pause();
                }
            }
            // Get the timing for the beat of the song
            for (unsigned int i = currentKeyFrame; i < keyFrameTimeStamps.size(); i++) {
                if (gameElapsedTime < keyFrameTimeStamps.at(i)) {
                    continue;
                }
                currentKeyFrame = i;
            }

            jumpedToNextFrame = currentKeyFrame != previousKeyFrame;
            previousKeyFrame = currentKeyFrame;

            double frameStart = keyFrameTimeStamps.at(currentKeyFrame);
            double frameEnd = keyFrameTimeStamps.at(currentKeyFrame + 1); // Assumes last keyframe at infinity

            double elapsedTimeInFrame = gameElapsedTime - frameStart;
            double frameDuration = frameEnd - frameStart;
            double fractionFrameComplete = elapsedTimeInFrame / frameDuration;

            double ballYCoord;

            KeyFrameAction currentOrigin = keyFrameDirections.at(currentKeyFrame);
            KeyFrameAction currentDestination = keyFrameDirections.at(currentKeyFrame + 1);

            // // Synchronize ball with music
            // if (currentOrigin == BOTTOM && currentDestination == BOTTOM) {
            //     ballYCoord = ballBottomY;
            // } else if (currentOrigin == TOP && currentDestination == TOP) {
            //     ballYCoord = ballBottomY + BallVerticalTravelDistance;
            // } else if (currentDestination == BOTTOM) {
            //     ballYCoord = ballBottomY + BallVerticalTravelDistance * (1 - fractionFrameComplete);
            // } else if (currentDestination == TOP) {
            //     ballYCoord = ballBottomY + BallVerticalTravelDistance * fractionFrameComplete;
            // }
        }
    }

    glm::mat4 projection = glm::perspective(glm::radians(80.0f), float(windowWidth) / float(windowHeight), 0.1f, 350.f);

    cameraPosition = glm::vec3(0, 2, -20);

    // Some math to make the camera move in a nice way
    float lookRotation = 0.1;
    glm::mat4 cameraTransform =
                    glm::rotate(0.3f, glm::vec3(1, 0, 0)) *
                    glm::rotate(lookRotation, glm::vec3(0, 1, 0)) *
                    glm::translate(-cameraPosition);

    glm::mat4 VP = projection * cameraTransform;

    // Move and rotate various SceneNodes
    boxNode->position = { 0, -10, -80 };

    // Set positions of static lights
    // cornerLightNode1->position  = {
    //         boxNode->position.x - (boxDimensions.x/2) + 20,
    //         boxNode->position.y + (boxDimensions.y/2) - 10,
    //         boxNode->position.z - (boxDimensions.z/2) + 5
    //     };
        
    // cornerLightNode2->position  = {
    //     boxNode->position.x + (boxDimensions.x/2) - 20,
    //     boxNode->position.y - (boxDimensions.y/2) + 10,
    //     boxNode->position.z - (boxDimensions.z/2) + 5
    // };
    
    // Alternative positions for colored shadows
    // cornerLightNode1->position  = {
    //     boxNode->position.x - 10,
    //     boxNode->position.y - 37.5,
    //     boxNode->position.z 
    // };
    
    // cornerLightNode2->position  = {
    //     boxNode->position.x + 10,
    //     boxNode->position.y - 37.5,
    //     boxNode->position.z
    // };

    ballNode->position = {
        boxNode->position.x - 20,
        boxNode->position.y - 37.5,
        boxNode->position.z
    };
    ballNode->scale = glm::vec3(ballRadius);
    ballNode->rotation = { 0, totalElapsedTime*2, 0 };

    box2Node->position = {
        boxNode->position.x - 15,
        boxNode->position.y - 17.5,
        boxNode->position.z
    };

    cactusNode->position = {
        boxNode->position.x + 30,
        boxNode->position.y - 17.5,
        boxNode->position.z
    };

    LightNode->position  = {
        boxNode->position.x + 10,
        boxNode->position.y - 37.5,
        boxNode->position.z + 10
    };

    

    updateNodeTransformations(rootNode, glm::identity<glm::mat4>(), VP);
    
    //Calculate orthographic projection at (0,0)
    glm::mat4 orthoProjection = glm::ortho(0.0f, float(windowWidth),
                                           0.0f, float(windowHeight),
                                          -1.0f, 1.0f);
}

void updateNodeTransformations(SceneNode* node, glm::mat4 transformationThusFar, glm::mat4 viewTransformation) {
    glm::mat4 transformationMatrix =
              glm::translate(node->position)
            * glm::translate(node->referencePoint)
            * glm::rotate(node->rotation.y, glm::vec3(0,1,0))
            * glm::rotate(node->rotation.x, glm::vec3(1,0,0))
            * glm::rotate(node->rotation.z, glm::vec3(0,0,1))
            * glm::scale(node->scale)
            * glm::translate(-node->referencePoint);

    node->currentTransformationMatrix = viewTransformation * transformationThusFar * transformationMatrix;
    node->modelMatrix = transformationThusFar * transformationMatrix;

    switch(node->nodeType) {
        case GEOMETRY: break;
        case POINT_LIGHT: break;
        case SPOT_LIGHT: break;
    }

    for(SceneNode* child : node->children) {
        updateNodeTransformations(child, node->modelMatrix, viewTransformation);
    }                     
}

void renderNode(SceneNode* node) {
    // shader->activate();
    // MVP
    glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix));
    // Model matrix
    glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(node->modelMatrix));
    // Normals matrix
    glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(node->modelMatrix)));
    glUniformMatrix3fv(5, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    // Camera position
    glUniform3fv(6, 1, glm::value_ptr(cameraPosition));
    // Ball position
    // Make sure it's not relative
    glm::vec3 ballPosition = glm::vec3(ballNode->modelMatrix * glm::vec4(0.0, 0.0, 0.0, 1.0));
    glUniform3fv(7, 1, glm::value_ptr(ballPosition));
    glUniform1d(8, ballRadius);
    // Textures settings
    glUniform1i(9, false);
    glUniform1i(10, false);

    switch(node->nodeType) {
        case GEOMETRY:
            if(node->vertexArrayObjectID != -1) {
                glBindVertexArray(node->vertexArrayObjectID);
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
            break;
        case POINT_LIGHT:
            // Calculate light position
            glm::vec3 lightPosition;
            lightPosition = glm::vec3(node->modelMatrix  * glm::vec4(0.0, 0.0, 0.0, 1.0));

            glUniform3fv(shader->getUniformFromName("light_source[" + std::to_string(node->id) + "].position"), 1, glm::value_ptr(lightPosition)); 
            glUniform3fv(shader->getUniformFromName("light_source[" + std::to_string(node->id) + "].color"), 1, glm::value_ptr(node->color));
    
            break;
        case SPOT_LIGHT: break;
        case UI:
            glUniform1i(9, true);
            glBindTextureUnit(0, node->textureID);

            if (node->vertexArrayObjectID != -1) {
                glBindVertexArray(node->vertexArrayObjectID);
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
            break;
        case NORMAL_MAP:
            glUniform1i(10, true);
            glBindTextureUnit(0, node->textureID);
            glBindTextureUnit(1, node->normalTextureID);

            if (node->vertexArrayObjectID != -1) {
                glBindVertexArray(node->vertexArrayObjectID);
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
            break;
    }

    for(SceneNode* child : node->children) {
        renderNode(child);
    }
}

void renderFrame(GLFWwindow* window) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);


    // Render scene to framebuffer

    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glClearColor(0.3f, 0.5f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    
    shader->activate();
    renderNode(rootNode);

    // Post-processing pass

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    shaderPP->activate();
    glBindVertexArray(rectVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glDrawArrays(GL_TRIANGLES, 0, 6);    
}
