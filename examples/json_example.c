#include <stdio.h>

#include <libutils/json.h>
#include <libutils/writer.h>

int JsonParseAndWrite(const char *json_data) {
    JsonElement *json = NULL;
    JsonParseError err = JsonParseAll(&json_data, &json);
    if (err != JSON_PARSE_OK) {
        printf("Error when parsing JSON data: %s\n", JsonParseErrorToString(err));
        return 1;
    } else {
        Writer *stdout_writer = FileWriter(stdout);
        if (stdout_writer == NULL) {
            printf("Failed to create new writer: %m\n");
            JsonDestroy(json);
            return 1;
        }
        JsonWrite(stdout_writer, json, 0 /* base indent level */);
        JsonDestroy(json);
        return 0;
    }
}

int main() {
    int ret = JsonParseAndWrite("{ \"hello\": \"world\" }");
    if (ret != 0) {
        return ret;
    }
    puts("");
    ret = JsonParseAndWrite("{ \"hell\": \"no }");
    if (ret != 1) {
        return 1;
    }
    return 0;
}
