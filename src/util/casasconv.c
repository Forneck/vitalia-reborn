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

void print_usage(const char *prog_name)
{
    printf("Uso: %s <casas.txt> <hcontrol>\n", prog_name);
    printf("\n");
    printf("Converte arquivo texto casas.txt para formato binario hcontrol.\n");
    printf("\n");
    printf("Formato do arquivo casas.txt (uma casa por linha):\n");
    printf("  vnum atrium exit_dir owner_id built_on last_payment num_guests [guest1 guest2 ...]\n");
    printf("\n");
    printf("Exemplo:\n");
    printf("  1000 999 2 12345 946684800 946684800 2 23456 34567\n");
    printf("  3001 3000 1 98765 946684800 946684800 0\n");
    printf("\n");
    printf("Campos:\n");
    printf("  vnum        - numero virtual do quarto da casa\n");
    printf("  atrium      - numero virtual do atrio da casa\n");
    printf("  exit_dir    - direcao da saida (0=norte, 1=leste, 2=sul, 3=oeste, 4=cima, 5=baixo)\n");
    printf("  owner_id    - ID do proprietario\n");
    printf("  built_on    - timestamp de quando foi construida\n");
    printf("  last_payment- timestamp do ultimo pagamento\n");
    printf("  num_guests  - numero de convidados (0-%d)\n", MAX_GUESTS);
    printf("  guest1-N    - IDs dos convidados (opcional, baseado em num_guests)\n");
    printf("\n");
}

int convert_casas_to_hcontrol(const char *input_file, const char *output_file)
{
    FILE *in, *out;
    struct house_control_rec house;
    char line[1024];
    int line_num = 0;
    int houses_converted = 0;

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

    /* Process each line */
    while (fgets(line, sizeof(line), in)) {
        int i, items_read;
        long guest_ids[MAX_GUESTS];

        line_num++;

        /* Skip empty lines and comments */
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '#')
            continue;

        /* Initialize the structure */
        memset(&house, 0, sizeof(house));
        house.mode = HOUSE_PRIVATE;

        /* Read the basic fields */
        items_read = sscanf(line, "%hu %hu %hd %ld %ld %ld %d", &house.vnum, &house.atrium, &house.exit_num,
                            &house.owner, (long *)&house.built_on, (long *)&house.last_payment, &house.num_of_guests);

        if (items_read < 7) {
            fprintf(stderr, "AVISO: Linha %d possui formato invalido (campos lidos: %d), pulando...\n", line_num,
                    items_read);
            continue;
        }

        /* Validate number of guests */
        if (house.num_of_guests < 0 || house.num_of_guests > MAX_GUESTS) {
            fprintf(stderr, "AVISO: Linha %d possui numero invalido de convidados (%d), ajustando para 0...\n",
                    line_num, house.num_of_guests);
            house.num_of_guests = 0;
        }

        /* Read guest IDs if present */
        if (house.num_of_guests > 0) {
            char *ptr = line;
            int field = 0;
            int guest_idx = 0;

            /* Skip to the 8th field (first guest) */
            while (*ptr && field < 7) {
                while (*ptr && *ptr != ' ' && *ptr != '\t')
                    ptr++;
                while (*ptr && (*ptr == ' ' || *ptr == '\t'))
                    ptr++;
                field++;
            }

            /* Read guest IDs */
            for (i = 0; i < house.num_of_guests && guest_idx < MAX_GUESTS; i++) {
                if (sscanf(ptr, "%ld", &guest_ids[guest_idx]) == 1) {
                    house.guests[guest_idx] = guest_ids[guest_idx];
                    guest_idx++;
                    /* Move to next field */
                    while (*ptr && *ptr != ' ' && *ptr != '\t')
                        ptr++;
                    while (*ptr && (*ptr == ' ' || *ptr == '\t'))
                        ptr++;
                } else {
                    break;
                }
            }

            if (guest_idx < house.num_of_guests) {
                fprintf(stderr, "AVISO: Linha %d possui menos IDs de convidados (%d) que esperado (%d)\n", line_num,
                        guest_idx, house.num_of_guests);
                house.num_of_guests = guest_idx;
            }
        }

        /* Write the structure to output */
        if (fwrite(&house, sizeof(struct house_control_rec), 1, out) != 1) {
            fprintf(stderr, "ERRO: Falha ao escrever casa %d no arquivo de saida\n", house.vnum);
            fclose(in);
            fclose(out);
            return 1;
        }

        houses_converted++;
        printf("  Casa %d convertida (proprietario: %ld, atrio: %d, convidados: %d)\n", house.vnum, house.owner,
               house.atrium, house.num_of_guests);

        if (houses_converted >= MAX_HOUSES) {
            fprintf(stderr, "AVISO: Numero maximo de casas (%d) atingido, parando conversao.\n", MAX_HOUSES);
            break;
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
    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }

    return convert_casas_to_hcontrol(argv[1], argv[2]);
}
