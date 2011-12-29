#ifndef JSON_H
#define JSON_H

struct json_callbacks {
    void (*on_null)();
    void (*on_boolean)(int value);
    void (*on_number)(double value);
};

struct json_value;
struct json_value* json_parse(const char* source, struct json_callbacks* callbacks);
void json_destroy(struct json_value* value);

#endif