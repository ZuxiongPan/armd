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

    char *json_str = cJSON_PrintUnformatted(json);
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
    cJSON *json = cJSON_Parse(json_buf);
    if(NULL == json)
    {
        armd_log("parse json failed\n");
        return -1;
    }

    cJSON *param = cJSON_GetObjectItem(json, "params");
    if(NULL == param)
    {
        armd_log("param not found\n");
        cJSON_Delete(json);
        return -1;
    }
    
    cJSON *vernum = cJSON_GetObjectItem(param, "vernum");
    if(NULL == vernum)
    {
        armd_log("vernum not found\n");
        cJSON_Delete(json);
        return -1;
    }

    cJSON *author = cJSON_GetObjectItem(param, "author");
    if(NULL == author)
    {
        armd_log("author not found\n");
        cJSON_Delete(json);
        return -1;
    }

    cJSON *author_email = cJSON_GetObjectItem(param, "author_email");
    if(NULL == author_email)
    {
        armd_log("author_email not found\n");
        cJSON_Delete(json);
        return -1;
    }

    snprintf(armd_ver.ver_string, sizeof(armd_ver.ver_string), "%s", vernum->valuestring);
    snprintf(armd_ver.author, sizeof(armd_ver.author), "%s", author->valuestring);
    snprintf(armd_ver.author_email, sizeof(armd_ver.author_email), "%s", author_email->valuestring);

    cJSON_Delete(json);

    return 0;
}