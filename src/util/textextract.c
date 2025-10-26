/* ************************************************************************
 *  File: textextract.c                                     Part of tbaMUD *
 *  Usage: Extract and preprocess descriptive text from MUD world files   *
 *         for LLM training purposes                                       *
 *                                                                         *
 *  This utility extracts text content from .wld, .mob, and .obj files    *
 *  and outputs preprocessed text suitable for language model training.   *
 *                                                                         *
 *  All Rights Reserved                                                    *
 *  Copyright (C) 2024 Vitalia Reborn Project                             *
 ************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include <iconv.h>

#define MAX_STRING_LENGTH 8192
#define MAX_INPUT_LENGTH 256

/* Function prototypes */
void usage(void);
void extract_room_text(FILE *wld_file, FILE *output_file);
void extract_mob_text(FILE *mob_file, FILE *output_file);
void extract_obj_text(FILE *obj_file, FILE *output_file);
char *preprocess_text(char *text);
char *read_string(FILE *fl, char *error);
void skip_spaces(FILE *fl);
char *convert_to_utf8(const char *input);
int is_valid_utf8(const char *str);

/* Global variables */
char buf[MAX_STRING_LENGTH];
char buf2[MAX_STRING_LENGTH];

int main(int argc, char *argv[])
{
    FILE *input_file, *output_file;
    char *file_type;

    if (argc != 4) {
        usage();
        return 1;
    }

    file_type = argv[1];

    if (!(input_file = fopen(argv[2], "r"))) {
        fprintf(stderr, "Error: Cannot open input file '%s'\n", argv[2]);
        return 1;
    }

    if (!(output_file = fopen(argv[3], "a"))) {
        fprintf(stderr, "Error: Cannot open output file '%s'\n", argv[3]);
        fclose(input_file);
        return 1;
    }

    printf("Extracting text from %s file: %s\n", file_type, argv[2]);

    if (strcmp(file_type, "wld") == 0) {
        extract_room_text(input_file, output_file);
    } else if (strcmp(file_type, "mob") == 0) {
        extract_mob_text(input_file, output_file);
    } else if (strcmp(file_type, "obj") == 0) {
        extract_obj_text(input_file, output_file);
    } else {
        fprintf(stderr, "Error: Unknown file type '%s'. Use 'wld', 'mob', or 'obj'\n", file_type);
        fclose(input_file);
        fclose(output_file);
        return 1;
    }

    fclose(input_file);
    fclose(output_file);

    printf("Text extraction completed.\n");
    return 0;
}

void usage(void)
{
    fprintf(stderr, "\nUsage: textextract <type> <input_file> <output_file>\n\n");
    fprintf(stderr, "  type:        File type (wld, mob, or obj)\n");
    fprintf(stderr, "  input_file:  Input MUD world file to process\n");
    fprintf(stderr, "  output_file: Output text file (will be appended to)\n\n");
    fprintf(stderr, "Examples:\n");
    fprintf(stderr, "  textextract wld lib/world/wld/30.wld training_text.txt\n");
    fprintf(stderr, "  textextract mob lib/world/mob/30.mob training_text.txt\n");
    fprintf(stderr, "  textextract obj lib/world/obj/30.obj training_text.txt\n\n");
    fprintf(stderr, "This utility extracts descriptive text from MUD world files\n");
    fprintf(stderr, "and preprocesses it for language model training.\n\n");
}

/* Extract text content from room (.wld) files */
void extract_room_text(FILE *wld_file, FILE *output_file)
{
    char line[MAX_INPUT_LENGTH];
    char *room_name, *room_desc, *exit_desc;
    char last_exit_desc[MAX_STRING_LENGTH] = "";
    int room_num;

    while (fgets(line, sizeof(line), wld_file)) {
        if (line[0] == '#') {
            /* Found a room number */
            if (sscanf(line, "#%d", &room_num) == 1) {
                /* Read room name */
                room_name = read_string(wld_file, "room name");
                if (room_name && strlen(room_name) > 0) {
                    char *cleaned_name = preprocess_text(room_name);
                    if (strlen(cleaned_name) > 0) {
                        fprintf(output_file, "=== SALA: %s ===\n", cleaned_name);
                    }
                    free(cleaned_name);
                    free(room_name);
                }

                /* Read room description */
                room_desc = read_string(wld_file, "room description");
                if (room_desc && strlen(room_desc) > 0) {
                    char *cleaned_desc = preprocess_text(room_desc);
                    if (strlen(cleaned_desc) > 10) { /* Skip very short descriptions */
                        fprintf(output_file, "%s\n\n", cleaned_desc);
                    }
                    free(cleaned_desc);
                    free(room_desc);
                }

                /* Skip the room flags line */
                if (fgets(line, sizeof(line), wld_file) == NULL) {
                    break;
                }

                /* Read exit descriptions and extra descriptions */
                last_exit_desc[0] = '\0'; /* Reset last exit description */
                while (fgets(line, sizeof(line), wld_file)) {
                    if (line[0] == 'S') {
                        break; /* End of room */
                    } else if (line[0] == 'D' && strlen(line) > 1) {
                        /* Direction exit - read the description */
                        exit_desc = read_string(wld_file, "exit description");
                        if (exit_desc && strlen(exit_desc) > 0) {
                            char *cleaned_exit = preprocess_text(exit_desc);
                            if (strlen(cleaned_exit) > 10 && strcmp(cleaned_exit, last_exit_desc) != 0) {
                                fprintf(output_file, "%s\n", cleaned_exit);
                                strncpy(last_exit_desc, cleaned_exit, sizeof(last_exit_desc) - 1);
                                last_exit_desc[sizeof(last_exit_desc) - 1] = '\0';
                            }
                            free(cleaned_exit);
                            free(exit_desc);
                        }
                        /* Skip keyword line */
                        read_string(wld_file, "exit keyword");
                        /* Skip the exit info line */
                        if (fgets(line, sizeof(line), wld_file) == NULL) {
                            break;
                        }
                    } else if (line[0] == 'E') {
                        /* Extra description - skip keyword, read description */
                        read_string(wld_file, "extra desc keyword");
                        exit_desc = read_string(wld_file, "extra description");
                        if (exit_desc && strlen(exit_desc) > 0) {
                            char *cleaned_extra = preprocess_text(exit_desc);
                            if (strlen(cleaned_extra) > 10) {
                                fprintf(output_file, "%s\n", cleaned_extra);
                            }
                            free(cleaned_extra);
                            free(exit_desc);
                        }
                    }
                }
                fprintf(output_file, "\n");
            }
        }
    }
}

/* Extract text content from mobile (.mob) files */
void extract_mob_text(FILE *mob_file, FILE *output_file)
{
    char line[MAX_INPUT_LENGTH];
    char *mob_name, *mob_short, *mob_long, *mob_detailed;
    int mob_num;

    while (fgets(line, sizeof(line), mob_file)) {
        if (line[0] == '#') {
            /* Found a mob number */
            if (sscanf(line, "#%d", &mob_num) == 1) {
                /* Read mob keywords */
                mob_name = read_string(mob_file, "mob keywords");

                /* Read mob short description */
                mob_short = read_string(mob_file, "mob short desc");

                /* Read mob long description */
                mob_long = read_string(mob_file, "mob long desc");

                /* Read mob detailed description */
                mob_detailed = read_string(mob_file, "mob detailed desc");

                /* Output mob information */
                if (mob_short && strlen(mob_short) > 0) {
                    char *cleaned_short = preprocess_text(mob_short);
                    if (strlen(cleaned_short) > 0) {
                        fprintf(output_file, "=== PERSONAGEM: %s ===\n", cleaned_short);
                    }
                    free(cleaned_short);
                }

                if (mob_long && strlen(mob_long) > 0) {
                    char *cleaned_long = preprocess_text(mob_long);
                    if (strlen(cleaned_long) > 10) {
                        fprintf(output_file, "%s\n", cleaned_long);
                    }
                    free(cleaned_long);
                }

                if (mob_detailed && strlen(mob_detailed) > 0) {
                    char *cleaned_detailed = preprocess_text(mob_detailed);
                    if (strlen(cleaned_detailed) > 10) {
                        fprintf(output_file, "%s\n", cleaned_detailed);
                    }
                    free(cleaned_detailed);
                }

                fprintf(output_file, "\n");

                /* Free allocated memory */
                if (mob_name)
                    free(mob_name);
                if (mob_short)
                    free(mob_short);
                if (mob_long)
                    free(mob_long);
                if (mob_detailed)
                    free(mob_detailed);

                /* Skip the rest of the mob entry until next mob or end */
                while (fgets(line, sizeof(line), mob_file)) {
                    if (line[0] == '#' || (line[0] == 'E' && strlen(line) < 3)) {
                        /* Found next mob or enhanced mob end marker */
                        fseek(mob_file, -strlen(line), SEEK_CUR);
                        break;
                    }
                }
            }
        }
    }
}

/* Extract text content from object (.obj) files */
void extract_obj_text(FILE *obj_file, FILE *output_file)
{
    char line[MAX_INPUT_LENGTH];
    char *obj_name, *obj_short, *obj_long, *extra_desc;
    int obj_num;

    while (fgets(line, sizeof(line), obj_file)) {
        if (line[0] == '#') {
            /* Found an object number */
            if (sscanf(line, "#%d", &obj_num) == 1) {
                /* Read object keywords */
                obj_name = read_string(obj_file, "obj keywords");

                /* Read object short description */
                obj_short = read_string(obj_file, "obj short desc");

                /* Read object long description */
                obj_long = read_string(obj_file, "obj long desc");

                /* Output object information */
                if (obj_short && strlen(obj_short) > 0) {
                    char *cleaned_short = preprocess_text(obj_short);
                    if (strlen(cleaned_short) > 0) {
                        fprintf(output_file, "=== OBJETO: %s ===\n", cleaned_short);
                    }
                    free(cleaned_short);
                }

                if (obj_long && strlen(obj_long) > 0) {
                    char *cleaned_long = preprocess_text(obj_long);
                    if (strlen(cleaned_long) > 10) {
                        fprintf(output_file, "%s\n", cleaned_long);
                    }
                    free(cleaned_long);
                }

                /* Free allocated memory */
                if (obj_name)
                    free(obj_name);
                if (obj_short)
                    free(obj_short);
                if (obj_long)
                    free(obj_long);

                /* Skip object stats line */
                if (fgets(line, sizeof(line), obj_file) == NULL) {
                    break;
                }

                /* Look for extra descriptions and affects */
                while (fgets(line, sizeof(line), obj_file)) {
                    if (line[0] == '#') {
                        /* Found next object */
                        fseek(obj_file, -strlen(line), SEEK_CUR);
                        break;
                    } else if (line[0] == 'E') {
                        /* Extra description - skip keyword, read description */
                        read_string(obj_file, "extra desc keyword");
                        extra_desc = read_string(obj_file, "extra description");
                        if (extra_desc && strlen(extra_desc) > 0) {
                            char *cleaned_extra = preprocess_text(extra_desc);
                            if (strlen(cleaned_extra) > 10) {
                                fprintf(output_file, "%s\n", cleaned_extra);
                            }
                            free(cleaned_extra);
                            free(extra_desc);
                        }
                    } else if (line[0] == '$') {
                        /* End of file */
                        break;
                    }
                    /* Skip other lines (A for affects, etc.) */
                }

                fprintf(output_file, "\n");
            }
        }
    }
}

/* Preprocess text for LLM training */
char *preprocess_text(char *text)
{
    char *result;
    char *src, *dst;
    int len;

    if (!text)
        return strdup("");

    len = strlen(text);
    result = malloc(len + 1);
    if (!result)
        return strdup("");

    src = text;
    dst = result;

    /* Remove leading whitespace */
    while (*src && isspace(*src))
        src++;

    /* Copy text, cleaning as we go */
    while (*src) {
        /* Remove tilde terminators */
        if (*src == '~') {
            src++;
            continue;
        }

        /* Convert multiple whitespace to single space */
        if (isspace(*src)) {
            *dst++ = ' ';
            while (*src && isspace(*src))
                src++;
            continue;
        }

        /* Copy normal characters */
        *dst++ = *src++;
    }

    /* Remove trailing whitespace */
    while (dst > result && isspace(*(dst - 1)))
        dst--;

    *dst = '\0';

    return result;
}

/* Read a string terminated by ~ */
char *read_string(FILE *fl, char *error)
{
    char *utf8_result;
    char tmp[MAX_STRING_LENGTH];
    int length = 0;
    int c;

    /* Skip leading whitespace */
    skip_spaces(fl);

    /* Read until we find a ~ */
    while ((c = fgetc(fl)) != EOF && c != '~' && length < MAX_STRING_LENGTH - 1) {
        tmp[length++] = c;
    }

    if (c == EOF) {
        fprintf(stderr, "Warning: Unexpected EOF while reading %s\n", error);
        return NULL;
    }

    tmp[length] = '\0';

    /* Convert from ISO-8859-1 to UTF-8 */
    utf8_result = convert_to_utf8(tmp);

    return utf8_result;
}

/* Skip whitespace in file */
void skip_spaces(FILE *fl)
{
    int c;

    while ((c = fgetc(fl)) != EOF && isspace(c))
        ;

    if (c != EOF) {
        ungetc(c, fl);
    }
}

/* Check if a string is valid UTF-8 */
int is_valid_utf8(const char *str)
{
    const unsigned char *bytes = (const unsigned char *)str;

    while (*bytes) {
        if (*bytes <= 0x7F) {
            /* ASCII character */
            bytes++;
        } else if ((*bytes & 0xE0) == 0xC0) {
            /* 2-byte sequence */
            if ((bytes[1] & 0xC0) != 0x80)
                return 0;
            bytes += 2;
        } else if ((*bytes & 0xF0) == 0xE0) {
            /* 3-byte sequence */
            if ((bytes[1] & 0xC0) != 0x80 || (bytes[2] & 0xC0) != 0x80)
                return 0;
            bytes += 3;
        } else if ((*bytes & 0xF8) == 0xF0) {
            /* 4-byte sequence */
            if ((bytes[1] & 0xC0) != 0x80 || (bytes[2] & 0xC0) != 0x80 || (bytes[3] & 0xC0) != 0x80)
                return 0;
            bytes += 4;
        } else {
            /* Invalid UTF-8 */
            return 0;
        }
    }

    return 1;
}

/* Convert string from ISO-8859-1 to UTF-8 */
char *convert_to_utf8(const char *input)
{
    iconv_t cd;
    char *output;
    char *inbuf, *outbuf;
    size_t inbytesleft, outbytesleft, output_size;

    if (!input)
        return strdup("");

    /* If input is already valid UTF-8, return a copy */
    if (is_valid_utf8(input)) {
        return strdup(input);
    }

    /* Open iconv conversion descriptor */
    cd = iconv_open("UTF-8", "ISO-8859-1");
    if (cd == (iconv_t)-1) {
        fprintf(stderr, "Warning: iconv_open failed, returning original string\n");
        return strdup(input);
    }

    /* Allocate output buffer (UTF-8 can be up to 4 bytes per character) */
    output_size = strlen(input) * 4 + 1;
    output = malloc(output_size);
    if (!output) {
        iconv_close(cd);
        return strdup("");
    }

    /* Setup pointers for iconv */
    inbuf = (char *)input;
    outbuf = output;
    inbytesleft = strlen(input);
    outbytesleft = output_size - 1;

    /* Perform conversion */
    if (iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft) == (size_t)-1) {
        fprintf(stderr, "Warning: iconv conversion failed, returning original string\n");
        iconv_close(cd);
        free(output);
        return strdup(input);
    }

    /* Null-terminate the output */
    *outbuf = '\0';

    /* Close iconv descriptor */
    iconv_close(cd);

    return output;
}