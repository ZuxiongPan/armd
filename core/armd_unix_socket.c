#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "armd_unix_socket.h"
#include "armd_comm.h"
#include "armd_log.h"
#include "ver_mgr.h"

#include "cJSON/cJSON.h"

int armd_unix_socket_create(const char *path)
{
    int sockfd = -1;
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));

    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(sockfd < 0)
    {
        armd_log("create socket failed, errno: %d\n", errno);
        return -1;
    }

    unlink(path);
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path);
    if(bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        armd_log("bind socket failed, errno: %d\n", errno);
        close(sockfd);
        return -1;
    }

    return sockfd;
}

void armd_unix_socket_cb(int fd, uint32_t event, void *arg)
{
    char buffer[ARMD_JSON_MSG_MAX_SIZE];
    memset(buffer, 0, sizeof(buffer));

    ssize_t len = recvfrom(fd, buffer, sizeof(buffer) - 1, 0, NULL, NULL);
    if(len < 0)
    {
        armd_log("recvfrom failed, errno: %d\n", errno);
        return ;
    }

    buffer[len] = '\0';
    armd_log("receive message %s\n", buffer);

    cJSON *json = cJSON_Parse(buffer);
    if(NULL == json)
    {
        armd_log("parse json failed\n");
        return ;
    }

    cJSON *type = cJSON_GetObjectItem(json, "type");
    if(NULL == type)
    {
        armd_log("type not found\n");
        cJSON_Delete(json);
        return ;
    }

    cJSON *table = cJSON_GetObjectItem(json, "table");
    if(NULL == table)
    {
        armd_log("table not found\n");
        cJSON_Delete(json);
        return ;
    }

    if(strncmp(type->valuestring, "get", 3) == 0 && strncmp(table->valuestring, "version", 7) == 0)
    {
        get_armd_ver_json(buffer, sizeof(buffer));
        armd_log("get %s %s\n", table->valuestring, buffer);
    }
    else if(strncmp(type->valuestring, "set", 3) == 0 && strncmp(table->valuestring, "version", 7) == 0)
    {
        set_armd_ver_json(buffer, sizeof(buffer));
    }
    else
    {
        armd_log("unknown type %s or table %s\n", type->valuestring, table->valuestring);
    }

    cJSON_Delete(json);

    return ;
}