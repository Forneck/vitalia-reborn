/* ************************************************************************
 *  file:  casasconv.c                                      Part of tbaMUD *
 *  Usage: code to convert text-based casas.txt to binary hcontrol format  *
 *  Written for Vitalia Reborn                                             *
 ************************************************************************* */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>

/* Define the types we need from the MUD */
typedef unsigned short int ush_int;
typedef ush_int room_vnum;
typedef signed short int sh_int;

#define MAX_HOUSES 400
#define MAX_GUESTS 10
#define HOUSE_PRIVATE 0

struct house_control_rec {
    room_vnum vnum;          /* vnum of this house		*/
    room_vnum atrium;        /* vnum of atrium		*/
    sh_int exit_num;         /* direction of house's exit	*/
    time_t built_on;         /* date this house was built	*/
    int mode;                /* mode of ownership		*/
    long owner;              /* idnum of house's owner	*/
    int num_of_guests;       /* how many guests for house	*/
    long guests[MAX_GUESTS]; /* idnums of house's guests	*/
    time_t last_payment;     /* date of last house payment   */
    long spare0;
    long spare1;
    long spare2;
    long spare3;
    long spare4;
    long spare5;
    long spare6;
    long spare7;
};

/* Trim leading and trailing whitespace */
static char *trim(char *str)
{
    char *end;

    /* Trim leading space */
    while (isspace((unsigned char)*str))
        str++;

    if (*str == 0) /* All spaces? */
        return str;

    /* Trim trailing space */
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    /* Write new null terminator */
    *(end + 1) = 0;

    return str;
}

void print_usage(const char *prog_name)
{
    printf("Uso: %s <casas.txt> <hcontrol> <atrium_map.txt>\n", prog_name);
    printf("\n");
    printf("Converte arquivo texto casas.txt (formato estruturado) para formato binario hcontrol.\n");
    printf("\n");
    printf("Formato do arquivo casas.txt:\n");
    printf("  class \"nome\" { ... };  # definicoes de classes (ignoradas)\n");
    printf("  house <vnum> {\n");
    printf("    class \"...\";         # classe (ignorada)\n");
    printf("    built <timestamp>;     # quando foi construida\n");
    printf("    owner <id>;            # ID do proprietario\n");
    printf("    payment <timestamp>;   # ultimo pagamento\n");
    printf("    guest <id>;            # convidado (pode ter varios)\n");
    printf("  };\n");
    printf("\n");
    printf("Formato do arquivo atrium_map.txt (uma linha por casa):\n");
    printf("  vnum atrium exit_dir\n");
    printf("\n");
    printf("Exemplo atrium_map.txt:\n");
    printf("  1050 1049 2\n");
    printf("  1051 1049 2\n");
    printf("\n");
    printf("Campos:\n");
    printf("  vnum     - numero virtual do quarto da casa\n");
    printf("  atrium   - numero virtual do atrio da casa\n");
    printf("  exit_dir - direcao da saida (0=norte, 1=leste, 2=sul, 3=oeste, 4=cima, 5=baixo)\n");
    printf("\n");
}

/* Structure to hold atrium mapping */
struct atrium_map {
    room_vnum vnum;
    room_vnum atrium;
    sh_int exit_num;
};

/* Load atrium map from file */
static int load_atrium_map(const char *filename, struct atrium_map *map, int max_entries)
{
    FILE *fp;
    char line[256];
    int count = 0;

    fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "ERRO: Nao foi possivel abrir arquivo de mapeamento '%s': %s\n", filename, strerror(errno));
        return -1;
    }

    while (fgets(line, sizeof(line), fp) && count < max_entries) {
        /* Skip comments and empty lines */
        char *trimmed = trim(line);
        if (*trimmed == '#' || *trimmed == '\0')
            continue;

        if (sscanf(trimmed, "%hu %hu %hd", &map[count].vnum, &map[count].atrium, &map[count].exit_num) == 3) {
            count++;
        }
    }

    fclose(fp);
    return count;
}

/* Find atrium info for a given house vnum */
static int find_atrium_info(room_vnum vnum, struct atrium_map *map, int map_size, room_vnum *atrium, sh_int *exit_num)
{
    int i;
    for (i = 0; i < map_size; i++) {
        if (map[i].vnum == vnum) {
            *atrium = map[i].atrium;
            *exit_num = map[i].exit_num;
            return 1;
        }
    }
    return 0;
}

int convert_casas_to_hcontrol(const char *input_file, const char *output_file, const char *atrium_file)
{
    FILE *in, *out;
    struct house_control_rec house;
    struct atrium_map atrium_map[MAX_HOUSES];
    char line[1024];
    int line_num = 0;
    int houses_converted = 0;
    int in_house_block = 0;
    int atrium_map_size = 0;

    /* Load atrium mapping */
    atrium_map_size = load_atrium_map(atrium_file, atrium_map, MAX_HOUSES);
    if (atrium_map_size < 0) {
        return 1;
    }
    printf("Carregado mapeamento de atrio com %d entrada(s).\n", atrium_map_size);

    /* Open input file */
    in = fopen(input_file, "r");
    if (!in) {
        fprintf(stderr, "ERRO: Nao foi possivel abrir arquivo de entrada '%s': %s\n", input_file, strerror(errno));
        return 1;
    }

    /* Open output file */
    out = fopen(output_file, "wb");
    if (!out) {
        fprintf(stderr, "ERRO: Nao foi possivel abrir arquivo de saida '%s': %s\n", output_file, strerror(errno));
        fclose(in);
        return 1;
    }

    printf("Convertendo '%s' para '%s'...\n", input_file, output_file);

    /* Initialize current house structure */
    memset(&house, 0, sizeof(house));

    /* Process each line */
    while (fgets(line, sizeof(line), in)) {
        char *trimmed = trim(line);
        line_num++;

        /* Skip empty lines and comments */
        if (*trimmed == '\0' || *trimmed == '#')
            continue;

        /* Skip class definitions */
        if (strncmp(trimmed, "class ", 6) == 0)
            continue;

        /* Check for house block start */
        if (strncmp(trimmed, "house ", 6) == 0) {
            int vnum;
            if (sscanf(trimmed, "house %d", &vnum) == 1) {
                /* Initialize new house structure */
                memset(&house, 0, sizeof(house));
                house.mode = HOUSE_PRIVATE;
                house.vnum = (room_vnum)vnum;
                in_house_block = 1;
            }
            continue;
        }

        /* Process house block contents */
        if (in_house_block) {
            /* Check for end of house block */
            if (strchr(trimmed, '}') != NULL) {
                room_vnum atrium;
                sh_int exit_num;

                /* Look up atrium info */
                if (!find_atrium_info(house.vnum, atrium_map, atrium_map_size, &atrium, &exit_num)) {
                    fprintf(stderr,
                            "AVISO: Casa %d nao encontrada no mapeamento de atrio, pulando...\n"
                            "       Adicione uma linha no arquivo atrium_map: %d <atrium> <exit_dir>\n",
                            house.vnum, house.vnum);
                    in_house_block = 0;
                    continue;
                }

                house.atrium = atrium;
                house.exit_num = exit_num;

                /* Write the structure to output */
                if (fwrite(&house, sizeof(struct house_control_rec), 1, out) != 1) {
                    fprintf(stderr, "ERRO: Falha ao escrever casa %d no arquivo de saida\n", house.vnum);
                    fclose(in);
                    fclose(out);
                    return 1;
                }

                houses_converted++;
                printf("  Casa %d convertida (proprietario: %ld, atrio: %d, saida: %d, convidados: %d)\n", house.vnum,
                       house.owner, house.atrium, house.exit_num, house.num_of_guests);

                in_house_block = 0;

                if (houses_converted >= MAX_HOUSES) {
                    fprintf(stderr, "AVISO: Numero maximo de casas (%d) atingido, parando conversao.\n", MAX_HOUSES);
                    break;
                }
                continue;
            }

            /* Parse house properties */
            if (strstr(trimmed, "built ")) {
                long timestamp;
                if (sscanf(trimmed, " built %ld", &timestamp) == 1) {
                    house.built_on = (time_t)timestamp;
                }
            } else if (strstr(trimmed, "owner ")) {
                long owner_id;
                if (sscanf(trimmed, " owner %ld", &owner_id) == 1) {
                    house.owner = owner_id;
                }
            } else if (strstr(trimmed, "payment ")) {
                long timestamp;
                if (sscanf(trimmed, " payment %ld", &timestamp) == 1) {
                    house.last_payment = (time_t)timestamp;
                }
            } else if (strstr(trimmed, "guest ")) {
                long guest_id;
                if (sscanf(trimmed, " guest %ld", &guest_id) == 1) {
                    if (house.num_of_guests < MAX_GUESTS) {
                        house.guests[house.num_of_guests] = guest_id;
                        house.num_of_guests++;
                    } else {
                        fprintf(stderr, "AVISO: Casa %d excedeu o numero maximo de convidados (%d)\n", house.vnum,
                                MAX_GUESTS);
                    }
                }
            }
        }
    }

    fclose(in);
    fclose(out);

    printf("\nConversao completa! %d casa(s) convertida(s).\n", houses_converted);
    printf("Arquivo binario criado: %s\n", output_file);
    printf("\nNOTA: Certifique-se de fazer backup do arquivo hcontrol existente antes de substituir!\n");

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        print_usage(argv[0]);
        return 1;
    }

    return convert_casas_to_hcontrol(argv[1], argv[2], argv[3]);
}
