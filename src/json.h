#ifndef JSON_H
#define JSON_H

struct json_value;
struct json_value* json_parse(const char* source);
void json_destroy(struct json_value* value);

#endif