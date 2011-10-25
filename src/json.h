#ifndef JSON_H
#define JSON_H

struct json_parser;

struct json_parser* json_parser_create(const char* source);
void json_parser_destroy(struct json_parser* parser);

#endif