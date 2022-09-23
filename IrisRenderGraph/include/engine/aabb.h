#pragma once
#include <glm/glm.hpp>

#undef min
#undef max

struct AABB
{
	glm::vec3 min;
	glm::vec3 max;

	static AABB invertedInfinity()
	{
		AABB result;
		result.min = glm::vec3(FLT_MAX);
		result.max = glm::vec3(-FLT_MAX);
		return result;
	}

    static AABB minMax(glm::vec3 aMin, glm::vec3 aMax)
    {
        AABB result;
        result.min = aMin;
        result.max = aMax;
        return result;
    }

    static AABB centerRadius(glm::vec3 aPoint, glm::vec3 aRadius)
    {
        AABB result;
        result.min = aPoint - aRadius;
        result.max = aPoint + aRadius;
        return result;
    }

    glm::vec3 getCenter() const
    {
        glm::vec3 result = 0.5f * (min + max);
        return result;
    }

    glm::vec3 getDimensions() const
    {
        glm::vec3 result = max - min;
        return result;
    }

    float getSurfaceArea() const
    {
        glm::vec3 dimension = getDimensions();
        float result = float(2.0) * (dimension.x * dimension.y + dimension.x * dimension.z + dimension.y * dimension.z);
        return result;
    }
};

inline AABB growToContain(AABB aabb, glm::vec3 aPoint)
{
    aabb.min = min(aabb.min, aPoint);
    aabb.max = max(aabb.max, aPoint);
    return aabb;
}

inline AABB combine(AABB a, AABB b)
{
    AABB result;
    result.min = min(a.min, b.min);
    result.max = max(a.max, b.max);
    return result;
}

inline AABB intersect(AABB a, AABB b)
{
    AABB result;
    result.min = min(a.min, b.min);
    result.max = max(a.max, b.max);
    return result;
}