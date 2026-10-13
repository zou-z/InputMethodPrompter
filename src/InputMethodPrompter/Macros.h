#pragma once

#define RETURN_IF(condition, value)  \
    do {                             \
        if (condition)               \
            return value;            \
    } while (false)

#define RETURN_IF_FAILED(expression, result)  \
    do {                                      \
        result = (expression);                \
        if (FAILED(result))                   \
            return result;                    \
    } while (false)
