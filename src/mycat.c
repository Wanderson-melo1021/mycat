#include <stdio.h>
#include <stdlib.h>

#include "mycat.h"

int mycat_stream(FILE *stream, const char *label, const MyCatOptions *options) {
    int ch;
    int at_line_start = 1;
    int line_number = 1;

    char buffer[32];
    int written;

    if (!stream || !label || !options) {
        return MYCAT_ERR_INVALID_ARG;
    }

    while ((ch = fgetc(stream)) != EOF) {
        if (at_line_start && options->number_nonblank_lines && ch != '\n') {
            written = snprintf(buffer, sizeof(buffer), "%6d\t", line_number);
            if (written < 0 || (size_t)written >= sizeof(buffer)) {
                return MYCAT_ERR_IO;
            }

            if (fputs(buffer, stdout) == EOF) {
                return MYCAT_ERR_IO;
            }

            line_number++;
        }

        else if (at_line_start && options->number_all_lines) {
            written = snprintf(buffer, sizeof(buffer), "%6d\t", line_number);
            if (written < 0 || (size_t)written >= sizeof(buffer)) {
                return MYCAT_ERR_IO;
            }

            if (fputs(buffer, stdout) == EOF) {
                return MYCAT_ERR_IO;
            }

            line_number++;
        }

        if (ch == '\t' && options->show_tabs) {
            if (fputc('^', stdout) == EOF) {
                return MYCAT_ERR_IO;
            }

            if (fputc('I', stdout) == EOF) {
                return MYCAT_ERR_IO;
            }

            at_line_start = 0;
            continue;
        }

        if (ch == '\n' && options->show_ends) {
            if (fputc('$', stdout) == EOF) {
                return MYCAT_ERR_IO;
            }
        }

        if (fputc(ch, stdout) == EOF) {
            return MYCAT_ERR_IO;
        }

        if (ch == '\n') {
            at_line_start = 1;
        } else {
            at_line_start = 0;
        }
    }

    if (ferror(stream)) {
        return MYCAT_ERR_IO;
    }

    return MYCAT_OK;
}

int mycat_file(const char *filepath, const MyCatOptions *options) {
    FILE *stream;
    int status;

    if (!filepath || !options) {
        return MYCAT_ERR_INVALID_ARG;
    }

    stream = fopen(filepath, "r");
    if (!stream) {
        return MYCAT_ERR_IO;
    }

    status = mycat_stream(stream, filepath, options);

    if (fclose(stream) == EOF) {
        return MYCAT_ERR_IO;
    }

    return status;
}
