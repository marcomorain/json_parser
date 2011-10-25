#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <array.h>

struct json_parser
{
    // A full duplicate of the source string.
    // This can be mutated as needed.
    char* source;
    char* position;
};

struct json_parser* json_parser_create(const char* source)
{
    struct json_parser* parser = malloc(sizeof(struct json_parser));
    parser->source = strdup(source);
    parser->position = parser->source;
    return NULL;
}


enum
{
    JSON_TYPE_STRING,
    JSON_TYPE_NUMBER,
    JSON_TYPE_OBJECT,
    JSON_TYPE_ARRAY,
    JSON_TYPE_BOOLEAN,
    JSON_TYPE_NULL
};

struct json_array
{
    size_t length;
    struct json_value** data;
};

struct json_value
{
    int type;
    
    union {
        const char* string;
        int boolean;
        struct json_array array;
    } data;
};

static struct json_value* json_value_create(int type)
{
    struct json_value* value = malloc(sizeof(struct json_value));
    value->type = type;
    return value;
}


static int skip(struct json_parser* parser, char c)
{
    parser->position++;
    return c == *parser->position;
}


static void json_parse_array_tail(struct json_parser* parser, struct json_value* array)
{
    assert(array->type == JSON_TYPE_ARRAY);
    switch(*parser->position)
    {
        case ']':
            return;
    }
}

static struct json_value* json_parse_value(struct json_parser* parser)
{
    switch(*parser->position)
    {
        case '[':
        {
            parser->position++;
            struct json_value* value = json_value_create(JSON_TYPE_ARRAY);
            value->data.array.data = 0;
            value->data.array.length = 0;
            return json_parse_array_tail(parser);
        }
            
        // String
        case '"':
        {
            parser->position++;
            struct json_value* value = json_value_create(JSON_TYPE_STRING);
            value->data.string = parser->position;
            while(*parser->position != '"') parser->position++;
            *parser->position = 0;
            parser->position++;
            return value;
        }
        
        // True
        case 't':
        {
            if (skip(parser, 'r') && 
                skip(parser, 'u') &&
                skip(parser, 'e'))
            {
                struct json_value* value = json_value_create(JSON_TYPE_BOOLEAN);
                value->data.boolean = 1;
                return value;
            }
        }
            
        // False
        case 'f':
        {
            if (skip(parser, 'a') && 
                skip(parser, 'l') &&
                skip(parser, 's') &&
                skip(parser, 'e'))
            {
                struct json_value* value = json_value_create(JSON_TYPE_BOOLEAN);
                value->data.boolean = 0;
                return value;
            }
        }
    }
    
    return NULL;
}

void json_parser_destroy(struct json_parser* parser)
{
    free(parser->source);
    free(parser);
}