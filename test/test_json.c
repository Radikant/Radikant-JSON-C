#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For strcmp
#include "rjson.h"

// A sample JSON string to test the parser
const char* sample_json = "{\n"
    "  \"name\": \"Radikant-JSON-C\",\n"
    "  \"version\": 1.0,\n"
    "  \"is_beta\": false,\n"
    "  \"author\": null,\n"
    "  \"features\": [\n"
    "    \"Parsing\",\n"
    "    \"Tree structure\",\n"
    "    \"Memory management\"\n"
    "  ],\n"
    "  \"details\": {\n"
    "    \"language\": \"C\",\n"
    "    \"lines_of_code\": 300\n"
    "  }\n"
    "}";

int main() {
    printf("--- Radikant JSON-C Parser Demo ---\n");
    printf("Parsing the following JSON:\n%s\n\n", sample_json);

    // Parse the string
    rjson_value* parsed_json = rjson_parse(sample_json);

    if (parsed_json == NULL) {
        fprintf(stderr, "Failed to parse JSON. Exiting.\n");
        return 1;
    }

    // Print the parsed structure to verify correctness
    printf("--- Parsed Structure ---\n");
    rjson_print(parsed_json, 0);
    printf("\n\n");

    // Example of accessing data using the new find key function
    printf("--- Accessing Data ---\n");
    
    rjson_value* name_val = rjson_object_get_value(parsed_json, "name");
    if (name_val && name_val->type == RJSON_STRING) {
        printf("Library Name: %s\n", name_val->as.str_val);
    } else {
        printf("Key 'name' not found or has incorrect type.\n");
    }

    rjson_value* version_val = rjson_object_get_value(parsed_json, "version");
    if (version_val && version_val->type == RJSON_NUMBER) {
        printf("Version: %.1f\n", version_val->as.num_val);
    } else {
        printf("Key 'version' not found or has incorrect type.\n");
    }

    rjson_value* features_val = rjson_object_get_value(parsed_json, "features");
    if (features_val && features_val->type == RJSON_ARRAY && features_val->as.arr_val.count > 0) {
        rjson_value* first_feature = features_val->as.arr_val.elements[0];
        if (first_feature && first_feature->type == RJSON_STRING) {
             printf("First feature: \"%s\"\n", first_feature->as.str_val);
        }
    } else {
        printf("Key 'features' not found, is not an array, or is empty.\n");
    }

    // Clean up all allocated memory
    rjson_free(parsed_json);
    printf("\nSuccessfully parsed and freed memory.\n");

    return 0;
}