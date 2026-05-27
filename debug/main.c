#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "armd_comm.h"
#include "cJSON/cJSON.h"

static inline void usage(void)
{
    printf("Usage:\n");
    printf("sendcmd [target] [type] \'message\'\n");
    printf("\ttarget: armd\n");
    printf("\ttype: get/set\n");
    printf("\tmessage: [table] [name1] [value1] [name2] [value2] [...]\n");
    printf("\tif get, value is forbidden\n");
}

static bool prepare_json_message(char *msg_buf, int bufsize, const char *target, const char *type, \
        const char *table, int argc, const char *argv[]);

int main(int argc, const char *argv[])
{
    if(argc < 4 || strncmp(argv[1], "armd", 4) != 0)
    {
        usage();
        return -1;
    }

    int fd;
    char message[ARMD_JSON_MSG_MAX_SIZE];
    memset(message, 0, sizeof(message));
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    const char *target = argv[1];
    const char *type = argv[2];
    const char *table = argv[3];

    if(!prepare_json_message(message, ARMD_JSON_MSG_MAX_SIZE, target, type, table, argc - 4, argv + 4))
    {
        printf("json message prepare failed\n");
        return -1;
    }

    printf("json message: %s\n", message);

    fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(fd < 0)
    {
        perror("socket");
        return -1;
    }

    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", ARMD_UNIX_SOCKET_PATH);

    if(sendto(fd, message, strlen(message), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("sendto");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

static bool prepare_json_message(char *msg_buf, int bufsize, const char *target, const char *type, \
        const char *table, int argc, const char *argv[])
{
    cJSON *root = cJSON_CreateObject();
    cJSON *params = NULL;
    if(NULL == root)
    {
        return false;
    }
    
    cJSON_AddStringToObject(root, "target", target);
    cJSON_AddStringToObject(root, "type", type);
    cJSON_AddStringToObject(root, "table", table);

    if(strncmp(type, "get", 3) == 0)
    {
        if(argc != 0)
        {
            params = cJSON_CreateArray();
            if(NULL == params)
            {
                cJSON_Delete(root);
                return false;
            }
            for(int i = 0; i < argc; i++)
            {
                cJSON_AddItemToArray(params, cJSON_CreateString(argv[i]));
            }

            cJSON_AddItemToObject(root, "params", params);
        }
        else
        {
            printf("get all info from %s\n", table);
        }
    }
    else if(argc > 0 && strncmp(type, "set", 3) == 0 && argc % 2 == 0)
    {
        params = cJSON_CreateObject();
        if(NULL == params)
        {
            cJSON_Delete(root);
            return false;
        }
        
        for(int i = 0; i < argc; i += 2)
        {
            cJSON_AddStringToObject(params, argv[i], argv[i + 1]);
        }
        cJSON_AddItemToObject(root, "params", params);
    }
    else
    {
        printf("invalid message\n");
        cJSON_Delete(root);
        return false;
    }

    char *json_str = cJSON_PrintUnformatted(root);
    if(NULL != json_str)
    {
        snprintf(msg_buf, bufsize, "%s", json_str);
        cJSON_free(json_str);
    }
    cJSON_Delete(root);

    return true;
}