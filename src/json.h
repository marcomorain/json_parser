#ifndef JSON_H
#define JSON_H

// For size_t
#include <string.h>

struct json_callbacks {
    void (*on_null)();
    void (*on_boolean)(int value);
    void (*on_number)(double value);
    void (*on_string)(const char* value, size_t length);
    void (*on_error)(unsigned line, unsigned col, const char* message);
};

enum
{
    JSON_PARSE_FAIL    = -1,
    JSON_PARSE_SUCCESS =  0
};

void json_callbacks_init(struct json_callbacks* callbacks);
int json_parse(const char* source, struct json_callbacks* callbacks);

#endif