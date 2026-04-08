#ifndef MYCAT_H
#define MYCAT_H

#include <stdio.h>
#include "mycat_options.h"

#define MYCAT_OK 0
#define MYCAT_ERR_INVALID_ARG 1
#define MYCAT_ERR_IO 2

int mycat_stream(FILE *stream, const char *label, const MyCatOptions *options);
int mycat_file(const char *filepath, const MyCatOptions *options);

#endif
