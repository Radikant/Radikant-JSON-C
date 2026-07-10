#include "rjson.h"
#include "parse.h"
#include "serialize.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

rjson_doc rjson_parse(const char* json_string) {
    if (!json_string) {
        rjson_doc doc = {NULL, NULL, RJSON_ERROR_BAD_ARG};
        return doc;
    }
    return rjson_parse_with_length(json_string, strlen(json_string));
}

rjson_doc rjson_parse_with_length(const char* json_string, size_t length) {
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
    
    doc.error = rjson_parse_stream(doc.zone, &stream, &doc.root);
    if (doc.error != RJSON_OK) {
        rjson_zone_destroy(doc.zone);
        doc.zone = NULL;
        doc.root = NULL;
    }
    return doc;
}

int rjson_serialize(const rjson_value* value, char** out_string, size_t* out_len) {
    if (!value || !out_string) return -1;
    
    rjson_out_stream stream;
    if (rjson_out_stream_init(&stream, 256) != 0) return -1;
    
    if (rjson_serialize_stream(&stream, value) != RJSON_OK) {
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
    switch (value->type) {
        case RJSON_NULL: printf("null\n"); break;
        case RJSON_BOOL: printf(value->as.bool_val ? "true\n" : "false\n"); break;
        case RJSON_NUMBER: printf("%g\n", value->as.num_val); break;
        case RJSON_STRING: printf("\"%.*s\"\n", (int)value->as.str_val.len, value->as.str_val.ptr); break;
        case RJSON_ARRAY: {
            printf("[\n");
            for (size_t i = 0; i < value->as.arr_val.count; ++i) {
                print_indent(indent + 1);
                rjson_print(&value->as.arr_val.elements[i], indent + 1);
            }
            print_indent(indent);
            printf("]\n");
            break;
        }
        case RJSON_OBJECT: {
            printf("{\n");
            for (size_t i = 0; i < value->as.obj_val.count; ++i) {
                print_indent(indent + 1);
                const rjson_kv* kv = &value->as.obj_val.kvs[i];
                printf("\"%.*s\": ", (int)kv->key.len, kv->key.ptr);
                rjson_print(&kv->value, indent + 1);
            }
            print_indent(indent);
            printf("}\n");
            break;
        }
    }
}