#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "mycat.h"

static void mycat_report_io_error(const char *label) {
    if (!label) {
        return;
    }

    if (errno != 0) {
        fprintf(stderr, "mycat: %s: %s\n", label, strerror(errno));
    } else {
        fprintf(stderr, "mycat: %s: I/O error\n", label);
    }
}

static int mycat_write_cat_prefix(void) {
    if (fputs("🐱\t", stdout) == EOF) {
        mycat_report_io_error("stdout");
        return MYCAT_ERR_IO;
    }

    return MYCAT_OK;
}

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
        if (at_line_start && options->cat_prefix_lines) {
            if (mycat_write_cat_prefix() != MYCAT_OK) {
                return MYCAT_ERR_IO;
            }
        }

        if (at_line_start && options->number_nonblank_lines && ch != '\n') {
            written = snprintf(buffer, sizeof(buffer), "%6d\t", line_number);
            if (written < 0 || (size_t)written >= sizeof(buffer)) {
                mycat_report_io_error(label);
                return MYCAT_ERR_IO;
            }

            if (fputs(buffer, stdout) == EOF) {
                mycat_report_io_error("stdout");
                return MYCAT_ERR_IO;
            }

            line_number++;
        }

        else if (at_line_start && options->number_all_lines) {
            written = snprintf(buffer, sizeof(buffer), "%6d\t", line_number);
            if (written < 0 || (size_t)written >= sizeof(buffer)) {
                mycat_report_io_error(label);
                return MYCAT_ERR_IO;
            }

            if (fputs(buffer, stdout) == EOF) {
                mycat_report_io_error("stdout");
                return MYCAT_ERR_IO;
            }

            line_number++;
        }

        if (ch == '\t' && options->show_tabs) {
            if (fputc('^', stdout) == EOF) {
                mycat_report_io_error("stdout");
                return MYCAT_ERR_IO;
            }

            if (fputc('I', stdout) == EOF) {
                mycat_report_io_error("stdout");
                return MYCAT_ERR_IO;
            }

            at_line_start = 0;
            continue;
        }

        if (ch == '\n' && options->show_ends) {
            if (fputc('$', stdout) == EOF) {
                mycat_report_io_error("stdout");
                return MYCAT_ERR_IO;
            }
        }

        if (fputc(ch, stdout) == EOF) {
            mycat_report_io_error("stdout");
            return MYCAT_ERR_IO;
        }

        if (ch == '\n') {
            at_line_start = 1;
        } else {
            at_line_start = 0;
        }
    }

    if (ferror(stream)) {
        mycat_report_io_error(label);
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
        mycat_report_io_error(filepath);
        return MYCAT_ERR_IO;
    }

    status = mycat_stream(stream, filepath, options);

    if (fclose(stream) == EOF) {
        mycat_report_io_error(filepath);
        return MYCAT_ERR_IO;
    }

    return status;
}
