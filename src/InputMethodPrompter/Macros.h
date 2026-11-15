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

#define RETURN_UNEXPECTED_IF_FAILED(expression, result)  \
    do {                                                 \
        result = (expression);                           \
        if (FAILED(result))                              \
            return std::unexpected(result);              \
    } while (false)
