#include "ver_mgr.h"
#include "armd_log.h"

#include "cJSON/cJSON.h"

typedef struct armd_ver_t {
    char ver_string[16];
    char author[32];
    char author_email[64];
} armd_ver_t;

static armd_ver_t armd_ver = {
    .ver_string = ARMD_VER_STRING,
    .author = ARMD_AUTHOR,
    .author_email = ARMD_AUTHOR_EMAIL,
};

int get_armd_ver_json(char *json_buf, size_t json_buf_size)
{
    cJSON *json = cJSON_CreateObject();
    if(NULL == json)
    {
        armd_log("create json failed\n");
        return -1;
    }

    cJSON_AddStringToObject(json, "vernum", armd_ver.ver_string);
    cJSON_AddStringToObject(json, "author", armd_ver.author);
    cJSON_AddStringToObject(json, "author_email", armd_ver.author_email);

    char *json_str = cJSON_PrintUnformatted(json, json_buf);
    if(NULL != json_str)
    {
        snprintf(json_buf, json_buf_size, "%s", json_str);
        cJSON_free(json_str);
    }
    cJSON_Delete(json);

    return 0;
}

int set_armd_ver_json(const char *json_buf, size_t json_buf_size)
{

    return 0;
}