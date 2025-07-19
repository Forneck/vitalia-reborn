/* *************************************************************************** */
/* code:    formula.c                                                          */
/* desc:    perform mathematics formula and return a value or an error message.*/
/*          (using operators, variables, conditional ? :, and logical)         */
/* author:  Bob Castillo (castillo7@hotmail.com)                               */
/* version: 0.5b      (for circle MUD 3.x with Oasis OLC)                      */
/* date:    25 dec 1999.                                                       */
/*          2020: updated, bugs fix, etc. now for TBA MUD 2020.                */
/* *************************************************************************** */
/* Copyright (c) 1999 castillo7@hotmail.com

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. */
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "spells.h"
#include "oasis.h"
#include "spedit.h"
#include "db.h"
#include "formula.h"
#include "string.h"

// #define DEBUG_FORMULA

const char *list_codes[] = {" ",         /* 00 */
                            "&&",        /* 01 (a) */
                            "||",        /* 02 (b) */
                            ">=",        /* 03 (c) */
                            ">",         /* 04 (d) */
                            "<=",        /* 05 (e) */
                            "<",         /* 06 (f) */
                            "==",        /* 07 (g) */
                            "!=",        /* 08 (h) */
                            "!",         /* 09 (i) */
                            "^",         /* 10 (j) */
                            "&",         /* 11 (k) */
                            "|",         /* 12 (l) */
                            "-",         /* 13 (m) */
                            "+",         /* 14 (n) */
                            "/",         /* 15 (o) */
                            "*",         /* 16 (p) */
                            "(",         /* 17 (q) */
                            ")",         /* 18 (r) */
                            "?",         /* 19 (s) */
                            ":",         /* 20 (t) */
                            "RAND(",     /* 21 (u) */
                            "SELF.",     /* 22 (v) */
                            "VICT.",     /* 23 (w) */
                            "%",         /* 24 (x) */
                                         /*    (y) (reserved for digit) */
                            "STR",       /* 25 (z) */
                            "DEX",       /* 26 (z) */
                            "INT",       /* 27 (z) */
                            "WIS",       /* 28 (z) */
                            "CON",       /* 29 (z) */
                            "CHA",       /* 30 (z) */
                            "CLASS",     /* 31 (z) */
                            "LEVEL",     /* 32 (z) */
                            "AGE",       /* 33 (z) */
                            "WEIGHT",    /* 34 (z) */
                            "HEIGHT",    /* 35 (z) */
                            "MAXMANA",   /* 36 (z) */
                            "MAXHIT",    /* 37 (z) */
                            "MAXMOVE",   /* 38 (z) */
                            "GOLD",      /* 39 (z) */
                            "EXP",       /* 40 (z) */
                            "ARMOR",     /* 41 (z) */
                            "HITROLL",   /* 42 (z) */
                            "DAMROLL",   /* 43 (z) */
                            "SAVPARA",   /* 44 (z) */
                            "SAVROD",    /* 45 (z) */
                            "SAVPETRI",  /* 46 (z) */
                            "SAVBREATH", /* 47 (z) */
                            "SAVSPELL",  /* 48 (z) */
                            "HIT",       /* 49 (z) */
                            "MANA",      /* 50 (z) */
                            "MOVE",      /* 51 (z) */
                            "GOOD",      /* 52 (z) */
                            "NEUTRAL",   /* 53 (z) */
                            "EVIL",      /* 54 (z) */
                            "DICE(",     /* 55 (A) */
                            ",",         /* 56 (B) */
                            "PARAM",     /* 57 (C) */
                            "\n"};

void add_to_formula(struct formula **head_formula, int command, int value)
{
    struct formula *Q, *ptr, *prev = NULL;

    CREATE(Q, struct formula, 1);
    for (ptr = *head_formula; ptr; prev = ptr, ptr = ptr->next)
        ;
    if (prev == NULL)
        *head_formula = Q;
    else
        prev->next = Q;
    Q->prev = prev;
    Q->next = NULL;

    Q->command = command;
    Q->value = value;
    Q->sign = 1;
    Q->next = NULL;
}

void free_formula(struct formula **head_formula)
{
    struct formula *q = *head_formula, *n = NULL;

    while (q) {
        n = q->next;
        free(q);
        q = n;
    }

    *head_formula = NULL;
}

int get_formula_typeact(char *str, int *ptr)
{
    int cpt = 0;

    if (str[*ptr] == ' ')
        return CODE_SPACE;
    else if ((str[*ptr] >= '0') && (str[*ptr] <= '9'))
        return CODE_DIGIT;
    while (list_codes[++cpt][0] != '\n')
        if (!strncmp(list_codes[cpt], (char *)&str[*ptr], strlen(list_codes[cpt]))) {
            *ptr += strlen(list_codes[cpt]) - 1;
            return cpt;
        }
    return -1;
}

int check_formula_syntax(int otype_act, int type_act)
{
    // CHAR_TYPE[0][] can't follow [1][]
    const char *test[4][2] = {
        {"abcdefghjklmnopqstuxA", "abcdefghjkloprstxzB"},   // can follow: imnquvwy
                                                            //     !,-,+,(,rand(,self.,vict.,digit
        {"i", "abcdefghijkloprstB"},            // '!' can follow mnquvwzA = -,+,(,rand(,self.,vict., digit, dice(
        {"ryz", "iquvwyzA"},                    // digit, ')', variables CAN'T follow:
                                                //             digit, !, (, rand(, self., vict., variables, dice(.
        {"vw", "abcdefghijklmnopqrstuvwxyAB"}   // self., vict. can follow z = variables.
    };
    int i;

    for (i = 0; i < 4; i++) {
        if (strchr(test[i][0], CHAR_CODE(otype_act))) {
            if (strchr(test[i][1], CHAR_CODE(type_act)))
                return (otype_act * 100 + type_act);
            else
                return 0;
        }
    }
    return 0;
}

void send_formula_error(struct char_data *ch, int error, int spell_vnum, int syserr)
{
    char buf[2048];

    const char *error_5000[] = {"Unknown variable or operator",           /* 5000 */
                                "it should have as much '(' and ')'",     /* 5001 */
                                "it should have as much '?' and ':'",     /* 5002 */
                                "can't perform formula. failed to solve", /* 5003 */
                                "':' was expected but not found",         /* 5004 */
                                "A formula can't end by ':'",             /* 5005 */
                                "division by 0",                          /* 5006 */
                                "Unsupported +++",                        /* 5007 */
                                "Unsupported ---",                        /* 5008 */
                                "it should have as much 'dice(' and ','", /* 5009 */
                                "',' expected but not found"};

    if (syserr)
        sprintf(buf, "SYSERR: got error (%d) from spell (%d) : ", error, spell_vnum);

    if ((error >= 5000) && (error <= LAST_5K_ERROR))
        snprintf(buf, sizeof(buf), "SYNTAX: %s", error_5000[error - 5000]);
    else if ((error >= 9900) || (error < 6000))
        snprintf(buf, sizeof(buf), "SYNTAX: %s can't follow %s",
                 ((error / 100) == CODE_DIGIT) ? "a number" : list_codes[error / 100],
                 ((error % 100) == CODE_DIGIT) ? "a number" : list_codes[error % 100]);
    else if (error >= ERROR_7000)
        snprintf(buf, sizeof(buf), "SYNTAX: A formula can't end by %s", list_codes[error - ERROR_7000]);
    else if (error >= 6000)
        snprintf(buf, sizeof(buf), "SYNTAX: A formula can't start by %s", list_codes[error - 6000]);

    if (syserr == TRUE)
        mudlog(BRF, LVL_BUILDER, TRUE, "%s", buf);
    else {
        send_to_char(ch, "%s\r\n", buf);
    }
}

#ifdef DEBUG_FORMULA
void show_formula(struct char_data *ch, struct formula *head_formula)
{
    char buf[2048];

    int cpt = 0;
    struct formula *Q;

    strcpy(buf, "formula : ");
    for (Q = head_formula; Q; Q = Q->next, cpt++) {
        if (Q->command == CODE_DIGIT)
            sprintf(buf, "%s%d", buf, Q->value);
        else
            sprintf(buf, "%s%s", buf,
                    ((Q->command == CODE_ART_SUB) && (Q->next) && (Q->next->sign == -1))   ? "+"
                    : ((Q->command == CODE_ART_ADD) && (Q->next) && (Q->next->sign == -1)) ? "-"
                                                                                           : list_codes[Q->command]);
    }
    sprintf(buf, "%s, nodes: %d\r\n", buf, cpt);
    send_to_char(ch, "%s", buf);
}

void show_ABC(struct char_data *ch, struct formula *C)
{
    struct formula *A = NULL, *B = NULL;

    if (C)
        B = C->prev;

    if (B)
        A = B->prev;

    if (A)
        send_to_char(ch,
                     "A) Command: %d\r\n"
                     "   Sign:    %d\r\n"
                     "   Value:   %d\r\n",
                     A->command, A->sign, A->value);
    if (B)
        send_to_char(ch,
                     "B) Command: %d\r\n"
                     "   Sign:    %d\r\n"
                     "   Value:   %d\r\n",
                     B->command, B->sign, B->value);
    if (C)
        send_to_char(ch,
                     "C) Command: %d\r\n"
                     "   Sign:    %d\r\n"
                     "   Value:   %d\r\n",
                     C->command, C->sign, C->value);
}
#endif

void remove_node(struct formula **head_formula, struct formula *ptr)
{
    if (ptr->prev)
        ptr->prev->next = ptr->next;
    else
        *head_formula = ptr->next;
    if (ptr->next)
        ptr->next->prev = ptr->prev;
    free(ptr);
}

int remove_brace(struct formula **head_formula, struct formula *C,   // int * rts_code,
                 struct char_data *ch)
{
    struct formula *A, *B;

    B = C->prev;
    A = B->prev;

#ifdef DEBUG_FORMULA
    send_to_char(ch, "remove_brace\r\n");
// show_ABC(ch, C);
#endif

    if (B->command == CODE_ART_RAND) {
        B->command = CODE_DIGIT;
        B->value = (B->value < 0) ? 0 : rand_number(1, B->value);

        remove_node(head_formula, C);

#ifdef DEBUG_FORMULA
        show_formula(ch, *head_formula);
#endif
        return 1;
    }

    if (!A) {
        B->command = CODE_DIGIT;

        remove_node(head_formula, C);

#ifdef DEBUG_FORMULA
        show_formula(ch, *head_formula);
#endif
        return 1;
    }

    // fix alias 12
    if (B->command == CODE_ART_OBRACE) {
        B->command = CODE_DIGIT;

        remove_node(head_formula, C);

#ifdef DEBUG_FORMULA
        show_formula(ch, *head_formula);
#endif
        return 1;
    }

    // remove typical (9) or rand(9)
    if ((A->command == CODE_ART_OBRACE) || (A->command == CODE_ART_RAND)) {
        if (A->command == CODE_ART_RAND)
            B->value = (B->sign == -1) ? 0 : rand_number(1, B->value);

        remove_node(head_formula, C);
        remove_node(head_formula, A);

#ifdef DEBUG_FORMULA
        show_formula(ch, *head_formula);
#endif
        return 1;
    }

    if (A->command == CODE_COMMA) {
#ifdef DEBUG_FORMULA
        send_to_char(ch, "dice (%d, %d)\r\n", A->prev->value, B->value);
#endif
        B->value = dice(A->prev->value, B->value);
        remove_node(head_formula, C);
        remove_node(head_formula, A);
        if (!A->prev || !A->prev->prev) {
            log1("SYSERR: Formula something went wrong!.");
            return 0;
        }
        remove_node(head_formula, A->prev);
        remove_node(head_formula, A->prev->prev);

#ifdef DEBUG_FORMULA
        show_formula(ch, *head_formula);
#endif
        return 1;
    }
    return 0;
}

struct formula *formula_do_oper(struct formula **head_formula, struct formula *B, int spell_vnum, struct char_data *ch,
                                int *rts_code)
{
    struct formula *A, *C;

    if (!(A = B->prev) && (B->command != CODE_ART_NOT) && (B->command != CODE_ART_ADD) &&
        (B->command != CODE_ART_SUB)) {
        *rts_code = 6000 + B->command;
        return NULL;
    }

    if (!(C = B->next)) {
        *rts_code = ERROR_7000 + B->command;
        return NULL;
    }

    switch (C->command) {
        case CODE_ART_SUB:
            if (C->next)
                C->next->sign = -1;
            remove_node(head_formula, C);
#ifdef DEBUG_FORMULA
            show_formula(ch, *head_formula);
#endif
            return (B);
        case CODE_ART_ADD:
            remove_node(head_formula, C);
#ifdef DEBUG_FORMULA
            show_formula(ch, *head_formula);
#endif
            return (B);
    }

    if (A) { /* !20 or -10, +10 there's no A*/
        A->value *= A->sign;
        A->sign = 1;
    }
    C->value *= C->sign;
    C->sign = 1;
    switch (B->command) {
        case CODE_CON_IF:
            if (!C->next || (C->next->command != CODE_CON_ELSE)) {
                *rts_code = ERROR_5004;
                return NULL;
            } else {
                if (!C->next->next) {
                    *rts_code = ERROR_5005;
                    return NULL;
                }
                A->value = A->value ? C->value : C->next->next->value;
                remove_node(head_formula, C->next->next);
                remove_node(head_formula, C->next);
                remove_node(head_formula, C);
                remove_node(head_formula, B);
#ifdef DEBUG_FORMULA
                show_formula(ch, *head_formula);
#endif
            }
            return (A);
        case CODE_LOG_OR:
            A->value = (A->value || C->value) ? 1 : 0;
            break;
        case CODE_LOG_AND:
            A->value = (A->value && C->value) ? 1 : 0;
            break;
        case CODE_LOG_GREQ:
            A->value = (A->value >= C->value) ? 1 : 0;
            break;
        case CODE_LOG_GR:
            A->value = (A->value > C->value) ? 1 : 0;
            break;
        case CODE_LOG_LTEQ:
            A->value = (A->value <= C->value) ? 1 : 0;
            break;
        case CODE_LOG_LT:
            A->value = (A->value < C->value) ? 1 : 0;
            break;
        case CODE_LOG_EQ:
            A->value = (A->value == C->value) ? 1 : 0;
            break;
        case CODE_LOG_NOTEQ:
            A->value = (A->value != C->value) ? 1 : 0;
            break;
        case CODE_ART_XOR:
            A->value = (A->value ^ C->value);
            break;
        case CODE_ART_AND:
            A->value = (A->value & C->value);
            break;
        case CODE_ART_OR:
            A->value = (A->value | C->value);
            break;
        case CODE_ART_MOD:
            A->value = (A->value % C->value);
            break;
        case CODE_ART_SUB:
            if (!A) {
                C->sign = -1;
                remove_node(head_formula, B);
#ifdef DEBUG_FORMULA
                show_formula(ch, *head_formula);
#endif
                return (C);
            } else
                A->value = (A->value - C->value);
            break;
        case CODE_ART_ADD:
            if (!A) {
                remove_node(head_formula, B);
#ifdef DEBUG_FORMULA
                show_formula(ch, *head_formula);
#endif
                return (C);
            } else
                A->value = (A->value + C->value);
            break;
        case CODE_ART_DIV:
            if (C->value)
                A->value = (A->value / C->value);
            else {
                *rts_code = ERROR_5006;
                return NULL;
            }
            break;
        case CODE_ART_MUL:
            A->value = (A->value * C->value);
            break;
        case CODE_ART_NOT:
            C->sign = 1;
            C->value = !C->value ? 1 : 0;
            remove_node(head_formula, B);
#ifdef DEBUG_FORMULA
            show_formula(ch, *head_formula);
#endif
            return (C);
    }
    remove_node(head_formula, C);
    remove_node(head_formula, B);
// this code does the same in a quicker way, but above is easier to understand
// ------------------------------------
// if ((A->next = C->next))
//   (C->next)->prev = A;
// free (C);
// free (B);
#ifdef DEBUG_FORMULA
    show_formula(ch, *head_formula);
#endif
    return (A);
}

int perform_formula(struct formula **head_formula, int spell_vnum, struct char_data *ch, int syserr, int *rts_code)
{
    const char *priority_check[5] = {"ijkl", "opx", "nm", "abcdefgh", "s"};
    int exit = 0, brace, result, mode;
    int inf_loop = 0;
    struct formula *Q, *ptr, *tmp;

    if (!*head_formula)
        return 0;

    do {
        for (ptr = NULL, Q = *head_formula; Q; Q = Q->next)
            if ((Q->command == CODE_ART_OBRACE) || (Q->command == CODE_ART_RAND) || (Q->command == CODE_ART_DICE))
                ptr = Q;

        if (ptr == NULL) {
            ptr = *head_formula;
            exit = 1;
        };

        brace = 1;
        mode = 0;
        tmp = ptr;

        do {
            if (++inf_loop > 1000) {
                *rts_code = ERROR_5003;
                send_formula_error(ch, *rts_code, spell_vnum, syserr);
                free_formula(head_formula);
                mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: Infinite loop detected in formula");
                return 0;
            }
            while (ptr && ptr->command != CODE_ART_CBRACE) {
                if (strchr(priority_check[mode], CHAR_CODE(ptr->command))) {
                    ptr = formula_do_oper(head_formula, ptr, spell_vnum, ch, rts_code);
                    if (*rts_code) {
                        send_formula_error(ch, *rts_code, spell_vnum, syserr);
                        free_formula(head_formula);
                        return 0;
                    }
                } else
                    ptr = ptr->next;
            }

            if (ptr && remove_brace(head_formula, ptr, ch))
                brace = 0;

            if (++mode > 4)
                brace = 0;
            else
                ptr = tmp;

        } while (brace);
    } while (!exit);

#ifdef DEBUG_FORMULA
    show_formula(ch, *head_formula);
#endif

    result = (*head_formula)->value * (*head_formula)->sign;
    free_formula(head_formula);
    return (result);
}

int formula_interpreter(struct char_data *self, struct char_data *vict, int spell_vnum, int syserr, char *cmd,
                        int param, int *rts_code)
{
    char buf[2048];

    const char bad_start_code[] = "abcdefghjkloprstxzB";
    const char bad_end_code[] = "abcdefghijklmnopqstuvwxAB";

    struct formula *head_formula = NULL;
    int i = 0, num = 0, otype_act = 0, type_act = 0, self_vict = -1;
    int cpt_obrace = 0, cpt_cbrace = 0, cpt_if = 0, cpt_else = 0;
    int cpt_dice = 0, cpt_comma = 0;
    int cpt_char = 1;

    *rts_code = 0;

    if (!self) {
        *rts_code = ERROR_8000;
        return 0;
    }

    if (!cmd || !*cmd)
        return 0;

    // if no vict, let's self act as the victim.
    if (!vict)
        vict = self;

    // remove all spaces in the formula, and truncate if cmd is bigger than my buffer 2048.
    buf[0] = ' ';

    for (i = 0; i < strlen(cmd) && (i < 2046); i++) {
        switch (cmd[i]) {
            case '(':
                cpt_obrace++;
                break;
            case ')':
                cpt_cbrace++;
                break;
            case '?':
                cpt_if++;
                break;
            case ':':
                cpt_else++;
                break;
            case ',':
                cpt_comma++;
                break;
        }
        if (cmd[i] != ' ')
            buf[cpt_char++] = toupper(cmd[i]);
    }
    buf[cpt_char] = ' ';
    buf[cpt_char + 1] = '\x0';

    if (strstr(buf, "+++")) {
        *rts_code = ERROR_5007;
        send_formula_error(self, *rts_code, spell_vnum, syserr);
        return 0;
    }

    if (strstr(buf, "---")) {
        *rts_code = ERROR_5008;
        send_formula_error(self, *rts_code, spell_vnum, syserr);
        return 0;
    }

    for (i = 0; i < strlen(buf); i++) {
        if (strncmp(&buf[i], "DICE(", 5) == 0)
            cpt_dice++;
    }

    if (cpt_obrace != cpt_cbrace) {
        *rts_code = ERROR_5001;
        send_formula_error(self, *rts_code, spell_vnum, syserr);
        return 0;
    }

    if (cpt_if != cpt_else) {
        *rts_code = ERROR_5002;
        send_formula_error(self, *rts_code, spell_vnum, syserr);
        return 0;
    }

    if (cpt_dice != cpt_comma) {
        *rts_code = ERROR_5009;
        send_formula_error(self, *rts_code, spell_vnum, syserr);
        return 0;
    }

    for (i = 0; i < strlen(buf); i++) {
        if (type_act)
            otype_act = type_act;
        if (!(type_act = get_formula_typeact(buf, &i)) && (i < strlen(buf) - 1))
            continue;

        if (!otype_act) {
            if (strchr(bad_start_code, CHAR_CODE(type_act))) {
                *rts_code = ERROR_6000 + type_act;
                send_formula_error(self, *rts_code, spell_vnum, syserr);
                return 0;
            }
        }

        if (type_act == -1) {
            *rts_code = ERROR_5000;
            send_formula_error(self, *rts_code, spell_vnum, syserr);
            free_formula(&head_formula);
            return 0;
        } else if (type_act && ((type_act != CODE_DIGIT) || (otype_act != CODE_DIGIT)))
            if ((*rts_code = check_formula_syntax(otype_act, type_act))) {
                send_formula_error(self, *rts_code, spell_vnum, syserr);
                free_formula(&head_formula);
                return 0;
            }

        if ((otype_act == CODE_IDE_SELF) || (otype_act == CODE_IDE_VICT))
            switch (type_act) {
                case CODE_VAR_IS_GOOD:
                    num = IS_GOOD(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_IS_NEUTRAL:
                    num = IS_NEUTRAL(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_IS_EVIL:
                    num = IS_EVIL(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_STR:
                    num = GET_STR(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_DEX:
                    num = GET_DEX(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_INT:
                    num = GET_INT(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_WIS:
                    num = GET_WIS(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_CON:
                    num = GET_CON(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_CHA:
                    num = GET_CHA(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_CLASS:
                    num = GET_CLASS(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_LEVEL:
                    num = GET_LEVEL(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_AGE:
                    num = GET_AGE(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_WEIGHT:
                    num = GET_WEIGHT(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_HEIGHT:
                    num = GET_HEIGHT(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_MANA:
                    num = GET_MANA(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_MAXMANA:
                    num = GET_MAX_MANA(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_HIT:
                    num = GET_HIT(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_MAXHIT:
                    num = GET_MAX_HIT(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_MOVE:
                    num = GET_MOVE(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_MAXMOVE:
                    num = GET_MAX_MOVE(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_GOLD:
                    num = GET_GOLD(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_EXP:
                    num = GET_EXP(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_ARMOR:
                    num = GET_AC(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_HITROLL:
                    num = GET_HITROLL(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_DAMROLL:
                    num = GET_DAMROLL(self_vict == CODE_IDE_SELF ? self : vict);
                    break;
                case CODE_VAR_SAVPARA:
                    num = GET_SAVE(self_vict == CODE_IDE_SELF ? self : vict, SAVING_PARA);
                    break;
                case CODE_VAR_SAVROD:
                    num = GET_SAVE(self_vict == CODE_IDE_SELF ? self : vict, SAVING_ROD);
                    break;
                case CODE_VAR_SAVPETRI:
                    num = GET_SAVE(self_vict == CODE_IDE_SELF ? self : vict, SAVING_PETRI);
                    break;
                case CODE_VAR_SAVBREATH:
                    num = GET_SAVE(self_vict == CODE_IDE_SELF ? self : vict, SAVING_BREATH);
                    break;
                case CODE_VAR_SAVSPELL:
                    num = GET_SAVE(self_vict == CODE_IDE_SELF ? self : vict, SAVING_SPELL);
                    break;
            }
        else {
            if (otype_act && ((otype_act != CODE_DIGIT) || (type_act != CODE_DIGIT))) {
                add_to_formula(&head_formula, otype_act, num);
                num = 0;
            }
            switch (type_act) {
                case CODE_IDE_SELF:
                case CODE_IDE_VICT:
                    self_vict = type_act;
                    break;
                case CODE_DIGIT:
                    num = num * 10 + (buf[i] - '0');
                    break;
                case CODE_PARAM:
                    num = param;
                    break;
            }
        }
    }

#ifdef DEBUG_FORMULA
    show_formula(self, head_formula);
#endif

    if (strchr(bad_end_code, CHAR_CODE(otype_act))) {
        *rts_code = ERROR_7000 + otype_act;
        send_formula_error(self, *rts_code, spell_vnum, syserr);
        free_formula(&head_formula);
        return 0;
    }

    return (perform_formula(&head_formula, spell_vnum, self, syserr, rts_code));
}

ACMD(do_formula)
{
    int rts_code;

    send_to_char(ch, "value: %d\r\n", formula_interpreter(ch, 0, 0, FALSE, argument, 0, &rts_code));
}