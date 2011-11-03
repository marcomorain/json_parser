#include "../src/json.h"

int main (int argc, const char * argv[])
{
    struct json_value* value = json_parse("test");
    json_destroy(value);
    return 0;
}

