#include <setjmp.h>
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
    jmp_buf* buffer;
};

static struct json_value* json_parse_value(struct json_parser* parser);

struct json_value* json_parse(const char* source)
{
    struct json_parser parser;
    parser.source = strdup(source);
    parser.position = parser.source;
    
    jmp_buf buffer; parser.buffer = &buffer;
    if (setjmp(buffer))
    {
        // Error
        return NULL;
    }
    return json_parse_value(&parser);
}

enum
{
    JSON_TYPE_NULL,
    JSON_TYPE_BOOLEAN,
    JSON_TYPE_NUMBER,
    JSON_TYPE_ARRAY,
    JSON_TYPE_STRING,
    JSON_TYPE_OBJECT,
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
    struct json_value* key;
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


static void json_object_insert(struct json_value* object, struct json_value* key, struct json_value* value)
{
    struct json_object* o = malloc(sizeof(struct json_object));
    o->key = key;
    o->value = value;
    o->next = object->data.object;
    object->data.object = o;
}

static int is_white_space(const char c)
{
    return  c == 0x20 || // Space
            c == 0x09 || // Horizontal tab
            c == 0x0A || // Line feed or New Line
            c == 0x0D;   // Carriage Return
}

static void skip_white_space(struct json_parser* parser)
{
    while(is_white_space(*parser->position)) parser->position++;
}

static void skip(struct json_parser* parser, char c)
{
    parser->position++;
    if (c != *parser->position)
    {
        longjmp(*parser->buffer, c);
    }
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
            json_array_append(&array->data.array, json_parse_value(parser));                
            skip(parser, ',');
        }
    }
}


static struct json_value* json_parse_string_tail(struct json_parser* parser)
{
    struct json_value* value = json_value_create(JSON_TYPE_STRING);
    value->data.string = parser->position;
    while(*parser->position != '"') parser->position++;
    *parser->position = 0;
    parser->position++;
    return value;
}

static void json_parse_object_tail(struct json_parser* parser, struct json_value* object)
{
    
    for (;;)
    {
        skip_white_space(parser);
    
        switch (*parser->position)
        {
            // Finished when we reach the final closing brace
            case '}':
                parser->position++;
                return;
                
            case '"':
                parser->position++;
                struct json_value* key = json_parse_string_tail(parser);
                skip_white_space(parser);
                skip(parser, ':');
                struct json_value* value = json_parse_value(parser);
                json_object_insert(object, key, value);
                break;
                
            default:
                break;
                // error
        }
    }    
}

static void skip_string(struct json_parser* parser, const char* s)
{
    for (char c = *s; (c = *s); s++) skip(parser, c);
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
            return json_parse_string_tail(parser);
        }
        
        // True
        case 't':
        {
            skip_string(parser, "rue");
            struct json_value* value = json_value_create(JSON_TYPE_BOOLEAN);
            value->data.boolean = 1;
            return value;
        }
            
        // False
        case 'f':
        {
            skip_string(parser, "alse");
            struct json_value* value = json_value_create(JSON_TYPE_BOOLEAN);
            value->data.boolean = 0;
            return value;
        }
            
        case 'n':
        {
            skip_string(parser, "ull");
            return json_value_create(JSON_TYPE_NULL);
        }
    }
    
    return NULL;
}

void json_destroy(struct json_value* value)
{
}
