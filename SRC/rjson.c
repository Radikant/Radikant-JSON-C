#include "rjson.h"
#include "decode.h"
#include "encode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

rjson_doc rjson_decode(const char* json_string) {
    if (!json_string) {
        rjson_doc doc = {NULL, NULL, RJSON_ERROR_BAD_ARG};
        return doc;
    }
    return rjson_decode_with_length(json_string, strlen(json_string));
}

rjson_doc rjson_decode_with_length(const char* json_string, size_t length) {
    rjson_doc doc = {NULL, NULL, RJSON_OK};
    if (!json_string) {
        doc.error = RJSON_ERROR_BAD_ARG;
        return doc;
    }
    
    doc.zone = rjson_zone_create();
    if (!doc.zone) {
        doc.error = RJSON_ERROR_NOMEM;
        return doc;
    }
    
    rjson_stream stream;
    rjson_stream_init(&stream, json_string, length);
    
    doc.root = (rjson_value*)rjson_zone_alloc(doc.zone, sizeof(rjson_value));
    if (!doc.root) {
        rjson_zone_destroy(doc.zone);
        doc.zone = NULL;
        doc.error = RJSON_ERROR_NOMEM;
        return doc;
    }
    
    doc.error = rjson_decode_stream(doc.zone, &stream, doc.root);
    
    rjson_stream_destroy(&stream);
    
    if (doc.error != RJSON_OK) {
        rjson_zone_destroy(doc.zone);
        doc.zone = NULL;
        doc.root = NULL;
    }
    return doc;
}

int rjson_encode(const rjson_value* value, char** out_string, size_t* out_len) {
    if (!value || !out_string) return -1;
    
    rjson_out_stream stream;
    if (rjson_out_stream_init(&stream, 4096) != 0) return -1;
    
    rjson_error_t err = rjson_encode_stream(&stream, value);
    if (err != RJSON_OK) {
        rjson_out_stream_destroy(&stream);
        return -1;
    }
    
    *out_string = stream.buffer;
    if (out_len) *out_len = stream.length;
    return 0;
}

void rjson_free(rjson_doc* doc) {
    if (!doc || !doc->zone) return;
    rjson_zone_destroy(doc->zone);
    doc->zone = NULL;
    doc->root = NULL;
}

rjson_value* rjson_object_get_value(const rjson_value* object, const char* key) {
    if (!key) return NULL;
    return rjson_object_get(object, key, strlen(key));
}

static void print_indent(int indent) {
    for (int i = 0; i < indent; ++i) printf("  ");
}

void rjson_print(const rjson_value* value, int indent) {
    if (!value) return;
    switch (rjson_get_type(value)) {
        case RJSON_NULL: printf("null\n"); break;
        case RJSON_BOOL: printf(value->as.bool_val ? "true\n" : "false\n"); break;
        case RJSON_NUMBER: printf("%g\n", value->as.num_val); break;
        case RJSON_STRING: printf("\"%.*s\"\n", (int)rjson_get_len(value), value->as.str_val); break;
        case RJSON_ARRAY: {
            printf("[\n");
            size_t count = rjson_get_len(value);
            for (size_t i = 0; i < count; ++i) {
                print_indent(indent + 1);
                rjson_print(&value->as.elements[i], indent + 1);
            }
            print_indent(indent);
            printf("]\n");
            break;
        }
        case RJSON_OBJECT: {
            printf("{\n");
            size_t count = rjson_get_len(value);
            for (size_t i = 0; i < count; ++i) {
                print_indent(indent + 1);
                const rjson_kv* kv = &value->as.kvs[i];
                printf("\"%.*s\": ", (int)rjson_get_len(&kv->key), kv->key.as.str_val);
                rjson_print(&kv->value, indent + 1);
            }
            print_indent(indent);
            printf("}\n");
            break;
        }
    }
}