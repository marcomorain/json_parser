#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "json.h"

typedef char json_char;

struct json_parser
{
    struct json_callbacks* callbacks;    
    
    
    unsigned line;
    unsigned col;
    
    // A full duplicate of the source string.
    // This can be mutated as needed.
    json_char* source;
    json_char* position;
    jmp_buf* buffer;

};

static void skip(struct json_parser* parser)
{
    parser->position++;
    parser->col++;
}

static json_char peek(struct json_parser* parser)
{
    return *parser->position;
}

static void on_null(){ printf("null\n"); }

static void on_boolean(int value)
{
    printf("%s\n", value ? "true" : "false");
}

static void on_number(double value)
{
    printf("%g\n", value);
}

static void on_string(const char* s, size_t length)
{
    puts(" \"");
    for (int i=0; i<length; i++)
    {
        putc(s[i], stdout);
    }
    puts("\"\n");
}

static void on_error(unsigned line, unsigned col, const char* message)
{
    fprintf(stderr, "Error parsing JSON on line %d at character %d\n%s\n", line, col, message);
}

void json_callbacks_init(struct json_callbacks* callbacks)
{
    callbacks->on_boolean = on_boolean;
    callbacks->on_null = on_null;
    callbacks->on_number = on_number;
    callbacks->on_string = on_string;
    callbacks->on_error = on_error;
}

static int json_parse_value(struct json_parser* parser);

int json_parse(const json_char* source, struct json_callbacks* callbacks)
{
    struct json_parser parser;
    parser.line = 0;
    parser.col  = 0;
    parser.callbacks = callbacks;
    parser.source = strdup(source);
    parser.position = parser.source;
    
    jmp_buf buffer; parser.buffer = &buffer;
    if (setjmp(buffer))
    {
        callbacks->on_error(parser.line, parser.col, parser.source);
        return JSON_PARSE_FAIL;
    }
    return json_parse_value(&parser);
}

static void skip_white_space(struct json_parser* parser)
{
    for (;;)
    {
        switch (peek(parser))
        {
            case 0x09: // Horizontal tab
            case 0x20: // Space
                skip(parser);
                continue;

            case 0x0A: // Line feed or New Line
                parser->line++;
                // fall-through
            case 0x0D: // Carriage Return:
                skip(parser);
                parser->col = 0;
                continue;
            default:
                return;
        }
    }
}

static void skip_char(struct json_parser* parser, json_char c)
{
    fprintf(stderr, "Skipping %c\n", c);
    if (c != peek(parser))
    {
        longjmp(*parser->buffer, c);
    }
    skip(parser);
}


static int json_parse_array_tail(struct json_parser* parser)
{
    for (;;)
    {
        if (peek(parser) == ']')
        {
            skip(parser);
            return JSON_PARSE_SUCCESS;
        }

        int more = 0;
        do
        {
            json_parse_value(parser);
            skip_white_space(parser);
            if (peek(parser) == ',')
            {
                skip(parser);
                more = 1;
            }
        } while (more);
    }
}

enum { CHARACTER_BUFFER_STACK_MAX = 64 };
struct character_buffer
{
    json_char stack_storage[CHARACTER_BUFFER_STACK_MAX];
    json_char* heap_storage;
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

static const json_char* character_buffer_get(const struct character_buffer* buffer)
{
    return buffer->heap_storage ? buffer->heap_storage : buffer->stack_storage;
}

static size_t character_buffer_length(const struct character_buffer* buffer){
    return buffer->used;
}


/*
 char
 any-Unicode-character-
 except-"-or-\-or-
 control-character
 \/
 \u four-hex-digits
 */

#include <limits.h>

static unsigned json_parse_hex_digit(struct json_parser* parser)
{
    char c = peek(parser);
    unsigned result = UINT_MAX;
    if (c >= '0' && c <= '9') result = c - '0';
    if (c >= 'A' && c <= 'F') result = c - 'A';
    
    // TODO: This is a syntax error.
    skip(parser);
    return result;
}

static unsigned json_parse_character(struct json_parser* parser)
{
    char c = peek(parser);
    switch(c)
    {
        case '\\':
        {
            skip(parser);
            switch(peek(parser))
            {

#define char_case(c, result) case (c): skip(parser); return (result)
                char_case('t',  '\t');
                char_case('b',  '\b');
                char_case('n',  '\n');
                char_case('r',  '\r');
                char_case('f',  '\f');
                char_case('"',   '"');
                char_case('\\', '\\');
                char_case('/',  '/');
#undef char_case
                case 'u':
                {
                    unsigned result = 0;
                    for (int i=0; i<4; i++)
                    {
                        result = (result << 8)| json_parse_hex_digit(parser);
                    }
                }
            }
        }
            
            
        default:
            skip(parser);
            return c;
    }
}

static int json_parse_string_tail(struct json_parser* parser)
{
    struct character_buffer buffer;
    character_buffer_init(&buffer);
    // string starts at parser->position;
    while(peek(parser) != '"')
    {
        // todo: parse the diddly characters
        // (escapes, unicodes, etc).
        character_buffer_push(&buffer, peek(parser));
        skip(parser);
    }
    // old code to set a null
    // *parser->position = 0;
    skip(parser);
    parser->callbacks->on_string(character_buffer_get(&buffer), 
                                 character_buffer_length(&buffer));
    character_buffer_destroy(&buffer);
    return JSON_PARSE_SUCCESS;
}

static int json_parse_object_tail(struct json_parser* parser)
{
    for (;;)
    {
        skip_white_space(parser);
    
        switch (peek(parser))
        {
            // Finished when we reach the final closing brace
            case '}':
                skip(parser);
                return JSON_PARSE_SUCCESS;
                
            case '"':
                
                // Key
                skip(parser);
                json_parse_string_tail(parser);
                
                // Sep
                skip_white_space(parser);
                skip_char(parser, ':');
                
                // Value
                json_parse_value(parser);
                
                
                // Comma?
                break;
                
            default:
                return JSON_PARSE_FAIL;
        }
    }    
}

static void skip_string(struct json_parser* parser, const json_char* s)
{
    while (*s)
    {
        skip_char(parser, *s);
        s++;
    }
}

static int json_parse_value(struct json_parser* parser)
{
    switch(peek(parser))
    {
        case '{':
        {
            skip(parser);
            return json_parse_object_tail(parser);
        }
        case '[':
        {
            skip(parser);
            return json_parse_array_tail(parser);
        }
            
        // String
        case '"':
        {
            skip(parser);
            return json_parse_string_tail(parser);
        }
        
        // True
        case 't':
        {
            skip_string(parser, "true");
            parser->callbacks->on_boolean(1);
            return JSON_PARSE_SUCCESS;
        }
            
        // False
        case 'f':
        {
            skip_string(parser, "false");
            parser->callbacks->on_boolean(0);
            return JSON_PARSE_SUCCESS;
        }
            
        case 'n':
        {
            skip_string(parser, "null");
            parser->callbacks->on_null();
            return JSON_PARSE_SUCCESS;
        }
    }
    
    return JSON_PARSE_FAIL;
}

