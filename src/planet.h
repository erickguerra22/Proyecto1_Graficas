#pragma once
#include <glm/glm.hpp>
#include <string>

struct Planet {
    float rotation;
    float rotationAngle;
    float rotationRadius;
    float speedFactor;
    glm::vec3 translation;
    glm::vec3 currentPos;
    glm::vec3 scale;
    bool hasMoon = false;
};