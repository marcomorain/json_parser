#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "json.h"

struct json_parser
{
    // A full duplicate of the source string.
    // This can be mutated as needed.
    char* source;
    char* position;
};

static struct json_value* json_parse_value(struct json_parser* parser);

struct json_value* json_parse(const char* source)
{
    struct json_parser parser;
    parser.source = strdup(source);
    parser.position = parser.source;
    return json_parse_value(&parser);
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
    size_t capacity;
    size_t length;
    struct json_value** data;
};

// Linked list
struct json_object
{
    const char* key;
    struct json_value* value;
    struct json_object* next;
};

struct json_value
{
    struct json_parser* parent;
    char* source;
    
    int type;
    
    union {
        const char* string;
        int boolean;
        struct json_array array;
        struct json_object* object;
    } data;
};

static struct json_value* json_value_create(int type)
{
    struct json_value* value = malloc(sizeof(struct json_value));
    value->type = type;
    return value;
}


static size_t max(size_t a, size_t b)
{
    return a > b ? a : b;
}

static void json_array_append(struct json_array* array, struct json_value* element)
{
    if (array->capacity == array->length)
    {
        // Grow
        array->capacity = max(1, 2 * array->capacity);
        array->data = realloc(array->data, sizeof(struct json_value*) * array->capacity);
    }
    
    array->data[array->length] = element;
    array->length++;
}


static void json_object_insert(struct json_value* object, const char* key, struct json_value* element)
{
    struct json_object* o = malloc(sizeof(struct json_object));
    o->key = key;
    o->value = element;
    o->next = object->data.object;
    object->data.object = o;
}

static int skip(struct json_parser* parser, char c)
{
    parser->position++;
    return c == *parser->position;
}


static void json_parse_array_tail(struct json_parser* parser, struct json_value* array)
{
    assert(array->type == JSON_TYPE_ARRAY);
    for (;;) switch(*parser->position)
    {
        case ']':
        {
            // Skip, accept
            parser->position++;
            return;
        }

        default:
        {
            struct json_value* element = json_parse_value(parser);
            if (!element)
            {
                // error
                return;
            }
            
            json_array_append(&array->data.array, element);
                
            if (!skip(parser, ','))
            {
                // error
                return;
            }
        }
    }
}

static void json_parse_object_tail(struct json_parser* parser, struct json_value* object)
{
    
    for (;;)
    {
        switch (*parser->position)
        {
            case '"':
            default:
                // error
        }
    }    
}

static struct json_value* json_parse_value(struct json_parser* parser)
{
    switch(*parser->position)
    {
        case '{':
        {
            parser->position++;
            struct json_value* value = json_value_create(JSON_TYPE_OBJECT);
            value->data.array.data     = 0;
            value->data.array.capacity = 0;
            value->data.array.length   = 0;
            json_parse_object_tail(parser, value);
            return value;
        }
        case '[':
        {
            parser->position++;
            struct json_value* value = json_value_create(JSON_TYPE_ARRAY);
            value->data.array.data     = 0;
            value->data.array.capacity = 0;
            value->data.array.length   = 0;
            json_parse_array_tail(parser, value);
            return value;
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
            
        case 'n':
        {
            if (skip(parser, 'u') && 
                skip(parser, 'l') &&
                skip(parser, 'l'))
            {
                struct json_value* value = json_value_create(JSON_TYPE_NULL);
                return value;
            }
        }
    }
    
    return NULL;
}

void json_destroy(struct json_value* value)
{
}