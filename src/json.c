#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "json.h"

struct json_parser
{
    struct json_callbacks* callbacks;    
    // A full duplicate of the source string.
    // This can be mutated as needed.
    char* source;
    char* position;
    jmp_buf* buffer;

};

static int json_parse_value(struct json_parser* parser);

int json_parse(const char* source, struct json_callbacks* callbacks)
{
    struct json_parser parser;
    parser.callbacks = callbacks;
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


static size_t max(size_t a, size_t b)
{
    return a > b ? a : b;
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


static int json_parse_array_tail(struct json_parser* parser)
{
    for (;;) switch(*parser->position)
    {
        case ']':
        {
            // Skip, accept
            parser->position++;
            return JSON_PARSE_SUCCESS;
        }

        default:
        {
            json_parse_value(parser);
            skip(parser, ',');
        }
    }
}

enum { CHARACTER_BUFFER_STACK_MAX = 64 };
struct character_buffer
{
    char stack_storage[CHARACTER_BUFFER_STACK_MAX];
    char* heap_storage;
    size_t capacity;
    size_t used;

};

static void character_buffer_init(struct character_buffer* buffer)
{
    buffer->capacity = CHARACTER_BUFFER_STACK_MAX;
    buffer->used = 0;
    buffer->heap_storage = NULL;
}

static void character_buffer_destroy(struct character_buffer* buffer)
{
    free(buffer->heap_storage);
    buffer->capacity = 0;
    buffer->used = 0;
}

static int character_buffer_push_heap(struct character_buffer* buffer, char c)
{
    assert(buffer->heap_storage);

    if (buffer->used == buffer->capacity)
    {
        buffer->capacity = 2 * buffer->capacity;
        buffer->heap_storage = realloc(buffer->heap_storage, buffer->capacity);
        if(!buffer->heap_storage) return 0;
    }
    buffer->heap_storage[buffer->used] = c;
    buffer->used++;
    return 1;
}

static int character_buffer_push(struct character_buffer* buffer, char c)
{
    if (buffer->heap_storage)
    {
        return character_buffer_push_heap(buffer, c);
    }
    
    if (buffer->used == buffer->capacity)
    {
        buffer->capacity = 2 * buffer->capacity;
        buffer->heap_storage = malloc(buffer->capacity);
        if(!buffer->heap_storage) return 0;
        memccpy(buffer->heap_storage, buffer->stack_storage, 1, CHARACTER_BUFFER_STACK_MAX);
        return character_buffer_push_heap(buffer, c);
    }
    
    buffer->stack_storage[buffer->used] = c;
    buffer->used++;
    return 1;
}

static const char* character_buffer_get(const struct character_buffer* buffer)
{
    return buffer->heap_storage ? buffer->heap_storage : buffer->stack_storage;
}

static int json_parse_string_tail(struct json_parser* parser)
{
    // string starts at parser->position;
    while(*parser->position != '"') parser->position++;
    // old code to set a null *parser->position = 0;
    parser->position++;
    return JSON_PARSE_SUCCESS;
}

static int json_parse_object_tail(struct json_parser* parser)
{
    for (;;)
    {
        skip_white_space(parser);
    
        switch (*parser->position)
        {
            // Finished when we reach the final closing brace
            case '}':
                parser->position++;
                return JSON_PARSE_SUCCESS;
                
            case '"':
                
                // Key
                parser->position++;
                json_parse_string_tail(parser);
                
                // Sep
                skip_white_space(parser);
                skip(parser, ':');
                
                // Value
                json_parse_value(parser);
                
                
                // Comma?
                break;
                
            default:
                return JSON_PARSE_FAIL;
        }
    }    
}

static void skip_string(struct json_parser* parser, const char* s)
{
    for (char c = *s; (c = *s); s++) skip(parser, c);
}

static int json_parse_value(struct json_parser* parser)
{
    switch(*parser->position)
    {
        case '{':
        {
            parser->position++;
            return json_parse_object_tail(parser);
        }
        case '[':
        {
            parser->position++;
            return json_parse_array_tail(parser);
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
            parser->callbacks->on_boolean(1);
            return JSON_PARSE_SUCCESS;
        }
            
        // False
        case 'f':
        {
            skip_string(parser, "alse");
            parser->callbacks->on_boolean(0);
            return JSON_PARSE_SUCCESS;
        }
            
        case 'n':
        {
            skip_string(parser, "ull");
            parser->callbacks->on_null();
            return JSON_PARSE_SUCCESS;
        }
    }
    
    return JSON_PARSE_FAIL;
}

