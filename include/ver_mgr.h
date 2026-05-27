#ifndef ARMD_VER_H
#define ARMD_VER_H

#define ARMD_VER_STRING "ARMD-V1.0.0"
#define ARMD_AUTHOR "ZuxiongPan"
#define ARMD_AUTHOR_EMAIL "pzxiong9865@gmail.com"

#include <stddef.h>

int get_armd_ver_json(char *json_buf, size_t json_buf_size);
int set_armd_ver_json(const char *json_buf, size_t json_buf_size);

#endif