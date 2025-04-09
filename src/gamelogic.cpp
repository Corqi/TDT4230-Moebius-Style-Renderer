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
SceneNode* cactusFlowerNode;
SceneNode* cactus01Node;
SceneNode* cactus02Node;
SceneNode* rock01Node;
SceneNode* rock02Node;
SceneNode* rock02_1Node;
SceneNode* rock02_2Node;
SceneNode* rock03Node;
SceneNode* bizonBonesNode;
SceneNode* bizonSkullNode;
SceneNode* terrainNode;


unsigned int FBO;
unsigned int RBO;
unsigned int rectVAO, rectVBO;
unsigned int framebufferTexture;
unsigned int normalTexture;
unsigned int depthTexture;

//Light Nodes
SceneNode* LightNode;

// These are heap allocated, because they should not be initialised at the start of the program
sf::SoundBuffer* buffer;
Gloom::Shader* shader;
Gloom::Shader* shaderPP;
sf::Sound* sound;


float rectangleVertices[] = {
    // Coords       // texCoords
    -1.0f, -1.0f,   0.0f, 0.0f,
    1.0f, -1.0f,   1.0f, 0.0f,
    -1.0f, 1.0f,   0.0f, 1.0f,
    
    1.0f, -1.0f,    1.0f, 0.0f,
    1.0f, 1.0f,   1.0f, 1.0f,
    -1.0f, 1.0f,   0.0f, 1.0f
};

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

    PNGImage cactusFlowerTexture = loadPNGFile("../res/textures/CactusFlower_col.png");
    unsigned int cactusFlowerTextureID = generateTextureID(cactusFlowerTexture);
    Mesh cactusFlower = loadModel("../res/models/CactusFlower.glb");

    PNGImage cactusTexture = loadPNGFile("../res/textures/Cactus_col.png");
    unsigned int cactusTextureID = generateTextureID(cactusTexture);
    Mesh cactus = loadModel("../res/models/Cactus.glb");

    PNGImage terrainTexture = loadPNGFile("../res/textures/Terrain_col.png");
    unsigned int terrainTextureID = generateTextureID(terrainTexture);
    Mesh terrain = loadModel("../res/models/TerrainSmooth.glb");

    PNGImage rock01Texture = loadPNGFile("../res/textures/Rock01_col.png");
    unsigned int rock01TextureID = generateTextureID(rock01Texture);
    Mesh rock01 = loadModel("../res/models/Rock01Smooth.glb");

    PNGImage rock02Texture = loadPNGFile("../res/textures/Rock02_col.png");
    unsigned int rock02TextureID = generateTextureID(rock01Texture);
    Mesh rock02 = loadModel("../res/models/Rock02Smooth.glb");

    PNGImage rock03Texture = loadPNGFile("../res/textures/Rock03_col.png");
    unsigned int rock03TextureID = generateTextureID(rock03Texture);
    Mesh rock03 = loadModel("../res/models/Rock03Smooth.glb");

    PNGImage bizonBonesTexture = loadPNGFile("../res/textures/BizonBones_col.png");
    unsigned int bizonBonesTextureID = generateTextureID(bizonBonesTexture);
    Mesh bizonBones = loadModel("../res/models/BizonBonesSmooth.glb");

    PNGImage bizonSkullTexture = loadPNGFile("../res/textures/BizonSkull_col.png");
    unsigned int bizonSkullTextureID = generateTextureID(bizonSkullTexture);
    Mesh bizonSkull = loadModel("../res/models/BizonSkullSmooth.glb");

    // Fill buffers
    unsigned int cactusFlowerVAO = generateBuffer(cactusFlower);
    unsigned int cactusVAO = generateBuffer(cactus);
    unsigned int terrainVAO = generateBuffer(terrain);
    unsigned int rock01VAO = generateBuffer(rock01);
    unsigned int rock02VAO = generateBuffer(rock02);
    unsigned int rock03VAO = generateBuffer(rock03);
    unsigned int bizonBonesVAO = generateBuffer(bizonBones);
    unsigned int bizonSkullVAO = generateBuffer(bizonSkull);

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

    terrainNode = createSceneNode();
    terrainNode->scale = glm::vec3(10.0f);
    terrainNode->nodeType = NORMAL_MAP;
    terrainNode->textureID = terrainTextureID;
    terrainNode->position = {
        0, 0, 0
    };
    terrainNode->rotation = {
        0, 180, 0
    };

    cactusFlowerNode = createSceneNode();
    cactusFlowerNode->scale = glm::vec3(0.7f);
    cactusFlowerNode->nodeType = NORMAL_MAP;
    cactusFlowerNode->textureID = cactusFlowerTextureID;
    cactusFlowerNode->position = {
        11, 1, -8
    };
    cactusFlowerNode->rotation = {
        0, 0.55f, 0
    };

    cactus01Node = createSceneNode();
    cactus01Node->scale = glm::vec3(0.7f);
    cactus01Node->nodeType = NORMAL_MAP;
    cactus01Node->textureID = cactusTextureID;
    cactus01Node->position = {
        16, 1, 0.5
    };
    cactus01Node->rotation = {
        0, -0.85f, 0
    };

    cactus02Node = createSceneNode();
    cactus02Node->scale = glm::vec3(0.7f);
    cactus02Node->nodeType = NORMAL_MAP;
    cactus02Node->textureID = cactusTextureID;
    cactus02Node->position = {
        11, 1, 1
    };
    cactus02Node->rotation = {
        0, 0.3f, 0
    };

    rock01Node = createSceneNode();
    rock01Node->scale = glm::vec3(0.7f);
    rock01Node->nodeType = NORMAL_MAP;
    rock01Node->textureID = rock01TextureID;
    rock01Node->position = {
        15.5f, -0.5, -3
    };
    rock01Node->rotation = {
        0, 0.6f, 0
    };

    rock02Node = createSceneNode();
    rock02Node->scale = glm::vec3(2.0f);
    rock02Node->nodeType = NORMAL_MAP;
    rock02Node->textureID = rock02TextureID;
    rock02Node->position = {
        16.0f, 1, 6
    };
    rock02Node->rotation = {
        0, 0, 0
    };

    rock02_1Node = createSceneNode();
    rock02_1Node->scale = glm::vec3(2.0f);
    rock02_1Node->nodeType = NORMAL_MAP;
    rock02_1Node->textureID = rock02TextureID;
    rock02_1Node->position = {
        4.0f, 1, 10
    };
    rock02_1Node->rotation = {
        0, 0, 0
    };

    rock02_2Node = createSceneNode();
    rock02_2Node->scale = glm::vec3(2.15f);
    rock02_2Node->nodeType = NORMAL_MAP;
    rock02_2Node->textureID = rock02TextureID;
    rock02_2Node->position = {
        2.0f, 1, -9.2
    };
    rock02_2Node->rotation = {
        0, 0, 0
    };

    rock03Node = createSceneNode();
    rock03Node->scale = glm::vec3(2.0f);
    rock03Node->nodeType = NORMAL_MAP;
    rock03Node->textureID = rock03TextureID;
    rock03Node->position = {
        -4.0f, 1.5, 0
    };
    rock03Node->rotation = {
        0, 0.5f, 0
    };

    bizonBonesNode = createSceneNode();
    bizonBonesNode->scale = glm::vec3(0.25f);
    bizonBonesNode->nodeType = NORMAL_MAP;
    bizonBonesNode->textureID = bizonBonesTextureID;
    bizonBonesNode->position = {
        7.0f, 0, -4
    };
    bizonBonesNode->rotation = {
        0, 0.7f, 0
    };

    bizonSkullNode = createSceneNode();
    bizonSkullNode->scale = glm::vec3(0.45f);
    bizonSkullNode->nodeType = NORMAL_MAP;
    bizonSkullNode->textureID = bizonSkullTextureID;
    bizonSkullNode->position = {
        10.0f, 0, -3.8
    };
    bizonSkullNode->rotation = {
        0.0f, -0.1f, 0.9f
    };

    //Debug light
    // Mesh sphere = generateSphere(0.1, 40, 40);
    // unsigned int ballVAO = generateBuffer(sphere);
    // LightNode = createSceneNode();
    // LightNode->vertexArrayObjectID = ballVAO;
    // LightNode->VAOIndexCount       = sphere.indices.size();

    LightNode = createSceneNode();
    LightNode->nodeType = POINT_LIGHT;
    LightNode->id = 0;
    LightNode->color = glm::vec3(255.0, 255.0, 255.0 );
    LightNode->position  = {
        12, 4, -1
    };
    
    rootNode->children.push_back(terrainNode);
    terrainNode->children.push_back(cactusFlowerNode);
    terrainNode->children.push_back(cactus01Node);
    terrainNode->children.push_back(cactus02Node);
    terrainNode->children.push_back(rock01Node);
    terrainNode->children.push_back(rock02Node);
    terrainNode->children.push_back(rock02_1Node);
    terrainNode->children.push_back(rock02_2Node);
    terrainNode->children.push_back(rock03Node);
    terrainNode->children.push_back(bizonBonesNode);
    terrainNode->children.push_back(bizonSkullNode);
    terrainNode->children.push_back(LightNode);

    cactusFlowerNode->vertexArrayObjectID = cactusFlowerVAO;
    cactusFlowerNode->VAOIndexCount       = cactusFlower.indices.size();

    cactus01Node->vertexArrayObjectID = cactusVAO;
    cactus01Node->VAOIndexCount       = cactus.indices.size();

    cactus02Node->vertexArrayObjectID = cactusVAO;
    cactus02Node->VAOIndexCount       = cactus.indices.size();

    rock01Node->vertexArrayObjectID = rock01VAO;
    rock01Node->VAOIndexCount       = rock01.indices.size();

    rock02Node->vertexArrayObjectID = rock02VAO;
    rock02Node->VAOIndexCount       = rock02.indices.size();

    rock02_1Node->vertexArrayObjectID = rock02VAO;
    rock02_1Node->VAOIndexCount       = rock02.indices.size();

    rock02_2Node->vertexArrayObjectID = rock02VAO;
    rock02_2Node->VAOIndexCount       = rock02.indices.size();

    rock03Node->vertexArrayObjectID = rock03VAO;
    rock03Node->VAOIndexCount       = rock03.indices.size();

    bizonBonesNode->vertexArrayObjectID = bizonBonesVAO;
    bizonBonesNode->VAOIndexCount       = bizonBones.indices.size();

    bizonSkullNode->vertexArrayObjectID = bizonSkullVAO;
    bizonSkullNode->VAOIndexCount       = bizonSkull.indices.size();

    terrainNode->vertexArrayObjectID = terrainVAO;
    terrainNode->VAOIndexCount       = terrain.indices.size();


    getTimeDeltaSeconds();

    std::cout << fmt::format("Initialized scene with {} SceneNodes.", totalChildren(rootNode)) << std::endl;

    std::cout << "Ready. Click to start!" << std::endl;
}

float angle;
void updateFrame(GLFWwindow* window) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    double timeDelta = getTimeDeltaSeconds();

    static float angle = 0.0f; // Persistent angle between frames
    
    // Circular motion for the light
    float radius = 4.0f;      // Radius of the circular path
    float speed = 0.5f;       // Rotation speed in radians per second
    
    // Update angle based on time (smooth continuous motion)
    angle += (float)timeDelta * speed;
    
    // Calculate new light position in a circular path around (11, 4, -3)
    LightNode->position.x = 11.0f + radius * cos(angle);
    LightNode->position.z = -3.0f + radius * sin(angle);
    LightNode->position.y = 3.0f; // Keep same height

    glm::mat4 projection = glm::perspective(glm::radians(80.0f), float(windowWidth) / float(windowHeight), 0.1f, 350.f);

    cameraPosition = glm::vec3(-40, 30, 170);

    // Some math to make the camera move in a nice way
    float lookRotation = 0.0;
    glm::mat4 cameraTransform =
                    glm::rotate(0.4f, glm::vec3(1, 0, 0)) *
                    glm::rotate(lookRotation, glm::vec3(0, 1, 0)) *
                    glm::translate(-cameraPosition);

    glm::mat4 VP = projection * cameraTransform;

    // Move and rotate various SceneNodes

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
    // glm::vec3 ballPosition = glm::vec3(ballNode->modelMatrix * glm::vec4(0.0, 0.0, 0.0, 1.0));
    // glUniform3fv(7, 1, glm::value_ptr(ballPosition));
    // glUniform1d(8, ballRadius);
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
            // glBindTextureUnit(1, node->normalTextureID);

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
    glClearColor(0.157f, 0.565f, 0.863f, 1.0f);
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
