#include <stdio.h>
#include "SRC/rjson.h"
#include "SRC/decode.h"
#include "SRC/zone.h"
#include "SRC/stream.h"

int main() {
    const char* json = "{\"hello\": 123}";
    rjson_doc doc = rjson_decode(json);
    if (doc.error != 0) {
        printf("Parse failed with error: %d\n", doc.error);
        return 1;
    }
    printf("Parse success!\n");
    return 0;
}
