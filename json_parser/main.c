#include "../src/json.h"
#include <stdio.h>
#include <stdlib.h>

static void do_file(const char* file_name)
{
    FILE* file = fopen(file_name, "r");
    
    size_t result;
    

    if (file == NULL)
    {
        fprintf(stderr, "File error %s", file_name);
        return;
    }
    
    // obtain file size:
    fseek(file , 0 , SEEK_END);
    size_t length = ftell(file);
    rewind(file);
    
    // allocate memory to contain the whole file:
    char* buffer = malloc(length);
    
    if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}
    
    // copy the file into the buffer:
    result = fread (buffer, 1, length, file);
    if (result != length) {fputs ("Reading error", stderr); exit (3);}
    
    /* the whole file is now loaded in the memory buffer. */
    
    // terminate
    fclose (file);
    free (buffer);
}

int main (int argc, const char * argv[])
{
    struct json_callbacks callbacks;
    json_callbacks_init(&callbacks);
    json_parse("[true]", &callbacks);
    json_parse("true", &callbacks);
    json_parse("false", &callbacks);
    json_parse("[]", &callbacks);

    json_parse("[true, false]", &callbacks);
    return 0;
}

