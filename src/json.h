#ifndef JSON_H
#define JSON_H

struct json_callbacks {
    void (*on_null)();
    void (*on_boolean)(int value);
    void (*on_number)(double value);
    void (*on_string)(const char* value, size_t length);
};

enum
{
    JSON_PARSE_FAIL    = -1,
    JSON_PARSE_SUCCESS =  0
};

int json_parse(const char* source, struct json_callbacks* callbacks);

#endif