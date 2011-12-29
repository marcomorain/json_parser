#include "../src/json.h"
#include <stdio.h>

static void do_file(const char* file_name)
{
    FILE* file = fopen(file_name, "r");
    

    char * buffer;
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
    result = fread (buffer,1,lSize,pFile);
    if (result != lSize) {fputs ("Reading error",stderr); exit (3);}
    
    /* the whole file is now loaded in the memory buffer. */
    
    // terminate
    fclose (pFile);
    free (buffer);
    return 0;
    
}

int main (int argc, const char * argv[])
{
    struct json_value* value = json_parse("test");
    json_destroy(value);
    return 0;
}

