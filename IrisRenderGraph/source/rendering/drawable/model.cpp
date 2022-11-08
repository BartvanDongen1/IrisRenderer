#include "rendering/drawable/model.h"

glm::mat4 getLocalTransform(ModelNode* aNode)
{
    if (aNode->parent != nullptr)
    {
        return aNode->transform * getLocalTransform(aNode->parent);
    }

    return aNode->transform;
}
