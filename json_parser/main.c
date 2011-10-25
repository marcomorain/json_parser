#include "../src/json.h"

int main (int argc, const char * argv[])
{
    struct json_parser* parser = json_parser_create();
    json_parser_destroy(parser);
    return 0;
}

