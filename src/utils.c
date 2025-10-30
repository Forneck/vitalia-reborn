/**
 * @file utils.c
 * Various utility functions used within the core mud code.
 *
 * Part of the core tbaMUD source code distribution, which is a derivative
 * of, and continuation of, CircleMUD.
 *
 * All rights reserved.  See license for complete information.
 * Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
 * CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "modify.h"
#include "screen.h"
#include "spells.h"
#include "handler.h"
#include "interpreter.h"
#include "class.h"
#include "act.h"
#include "constants.h"
#include "shop.h"
#include "db.h"
#include "fight.h"
#include "quest.h"
#include "graph.h"
#include "genolc.h"

/** Aportable random number function.
 * @param from The lower bounds of the random number.
 * @param to The upper bounds of the random number. */
int rand_number(int from, int to)
{
    /* error checking in case people call this incorrectly */
    if (from > to) {
        int tmp = from;
        from = to;
        to = tmp;
        log1("SYSERR: rand_number() should be called with lowest, then highest. (%d, %d), not (%d, %d).", from, to, to,
             from);
    }

    /* This should always be of the form: ((float)(to - from + 1) * rand() /
     * (float)(RAND_MAX + from) + from); If you are using rand() due to historical
     * non-randomness of the lower bits in older implementations.  We always use
     * circle_random() though, which shouldn't have that problem. Mean and
     * standard deviation of both are identical (within the realm of statistical
     * identity) if the rand() implementation is non-broken. */
    return ((circle_random() % (to - from + 1)) + from);
}

/** Simulates a single dice roll from one to many of a certain sized die.
 * @param num The number of dice to roll.
 * @param size The number of sides each die has, and hence the number range
 * of the die. */
int dice(int num, int size)
{
    int sum = 0;

    if (size <= 0 || num <= 0)
        return (0);

    while (num-- > 0)
        sum += rand_number(1, size);

    return (sum);
}

/** Return the smaller number. Original note: Be wary of sign issues with this.
 * @param a The first number.
 * @param b The second number. */
int MIN(int a, int b) { return (a < b ? a : b); }

/** Return the larger number. Original note: Be wary of sign issues with this.
 * @param a The first number.
 * @param b The second number. */
int MAX(int a, int b) { return (a > b ? a : b); }

/** Used to capitalize a string. Will not change any mud specific color codes.
 * @param txt The string to capitalize. */
char *CAP(char *txt)
{
    char *p = txt;

    /* Skip all preceeding color codes and ANSI codes */
    while ((*p == '\t' && *(p + 1)) || (*p == '\x1B' && *(p + 1) == '[')) {
        if (*p == '\t')
            p += 2; /* Skip \t sign and color letter/number */
        else {
            p += 2; /* Skip the CSI section of the ANSI code */
            while (*p && !isalpha(*p))
                p++; /* Skip until a 'letter' is found */
            if (*p)
                p++; /* Skip the letter */
        }
    }

    if (*p)
        *p = UPPER(*p);
    return (txt);
}

#if !defined(HAVE_STRLCPY) || defined(NEED_STRLCPY_PROTO)
/** A 'strlcpy' function in the same fashion as 'strdup' below. This copies up
 * to totalsize - 1 bytes from the source string, placing them and a trailing
 * NUL into the destination string. Returns the total length of the string it
 * tried to copy, not including the trailing NUL.  So a '>= totalsize' test
 * says it was truncated. (Note that you may have _expected_ truncation
 * because you only wanted a few characters from the source string.) Portable
 * function, in case your system does not have strlcpy. */
size_t strlcpy(char *dest, const char *source, size_t totalsize)
{
    strncpy(dest, source, totalsize - 1); /* strncpy: OK (we must assume 'totalsize' is correct) */
    dest[totalsize - 1] = '\0';
    return strlen(source);
}
#endif

#if !defined(HAVE_STRDUP)
/** Create a duplicate of a string function. Portable. */
char *strdup(const char *source)
{
    char *new_z;

    CREATE(new_z, char, strlen(source) + 1);
    return (strcpy(new_z, source)); /* strcpy: OK */
}
#endif

/** Strips "\\r\\n" from just the end of a string. Will not remove internal
 * "\\r\\n" values to the string.
 * @post Replaces any "\\r\\n" values at the end of the string with null.
 * @param txt The writable string to prune. */
void prune_crlf(char *txt)
{
    int i = strlen(txt) - 1;

    while (txt[i] == '\n' || txt[i] == '\r')
        txt[i--] = '\0';
}

/** Formats a number with Brazilian-style thousand separators (periods).
 * Returns a static string buffer with the formatted number.
 * For example: 1234567 becomes "1.234.567"
 * @param number The number to format.
 * @return A pointer to a static string buffer containing the formatted number. */
char *format_number_br(int number)
{
    static char result[32];
    char temp[32];
    int i, j, len;

    /* Handle negative numbers */
    int negative = 0;
    if (number < 0) {
        negative = 1;
        number = -number;
    }

    /* Convert number to string */
    snprintf(temp, sizeof(temp), "%d", number);
    len = strlen(temp);

    /* Build result string with separators */
    j = 0;
    if (negative) {
        result[j++] = '-';
    }

    for (i = 0; i < len; i++) {
        /* Add separator every 3 digits from the right */
        if (i > 0 && (len - i) % 3 == 0) {
            result[j++] = '.';
        }
        result[j++] = temp[i];
    }
    result[j] = '\0';

    return result;
}

/** Formats a long number with Brazilian-style thousand separators (periods).
 * Returns a static string buffer with the formatted number.
 * For example: 1234567L becomes "1.234.567"
 * @param number The long number to format.
 * @return A pointer to a static string buffer containing the formatted number. */
char *format_long_br(long number)
{
    static char result[64];
    char temp[64];
    int i, j, len;

    /* Handle negative numbers */
    int negative = 0;
    if (number < 0) {
        negative = 1;
        number = -number;
    }

    /* Convert number to string */
    snprintf(temp, sizeof(temp), "%ld", number);
    len = strlen(temp);

    /* Build result string with separators */
    j = 0;
    if (negative) {
        result[j++] = '-';
    }

    for (i = 0; i < len; i++) {
        /* Add separator every 3 digits from the right */
        if (i > 0 && (len - i) % 3 == 0) {
            result[j++] = '.';
        }
        result[j++] = temp[i];
    }
    result[j] = '\0';

    return result;
}

#ifndef str_cmp
/** a portable, case-insensitive version of strcmp(). Returns: 0 if equal, > 0
 * if arg1 > arg2, or < 0 if arg1 < arg2. Scan until strings are found
 * different or we reach the end of both. */
int str_cmp(const char *arg1, const char *arg2)
{
    int chk, i;

    if (arg1 == NULL || arg2 == NULL) {
        log1("SYSERR: str_cmp() passed a NULL pointer, %p or %p.", (void *)arg1, (void *)arg2);
        return (0);
    }

    for (i = 0; arg1[i] || arg2[i]; i++)
        if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
            return (chk); /* not equal */

    return (0);
}
#endif

#ifndef strn_cmp
/** a portable, case-insensitive version of strncmp(). Returns: 0 if equal, > 0
 * if arg1 > arg2, or < 0 if arg1 < arg2. Scan until strings are found
 * different, the end of both, or n is reached. */
int strn_cmp(const char *arg1, const char *arg2, int n)
{
    int chk, i;

    if (arg1 == NULL || arg2 == NULL) {
        log1("SYSERR: strn_cmp() passed a NULL pointer, %p or %p.", (void *)arg1, (void *)arg2);
        return (0);
    }

    for (i = 0; (arg1[i] || arg2[i]) && (n > 0); i++, n--)
        if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
            return (chk); /* not equal */

    return (0);
}
#endif

/** New variable argument log() function; logs messages to disk.
 * Works the same as the old for previously written code but is very nice
 * if new code wishes to implment printf style log messages without the need
 * to make prior sprintf calls.
 * @param format The message to log. Standard printf formatting and variable
 * arguments are allowed.
 * @param args The comma delimited, variable substitutions to make in str. */
void basic_mud_vlog(const char *format, va_list args)
{
    time_t ct = time(0);
    char timestr[21];
    int i;

    if (logfile == NULL) {
        puts("SYSERR: Using log() before stream was initialized!");
        return;
    }

    if (format == NULL)
        format = "SYSERR: log() received a NULL format.";

    for (i = 0; i < 21; i++)
        timestr[i] = 0;
    strftime(timestr, sizeof(timestr), "%b %d %H:%M:%S %Y", localtime(&ct));

    fprintf(logfile, "%-20.20s :: ", timestr);
    vfprintf(logfile, format, args);
    fputc('\n', logfile);
    fflush(logfile);
}

/** Log messages directly to syslog on disk, no display to in game immortals.
 * Supports variable string modification arguments, a la printf. Most likely
 * any calls to plain old log() have been redirected, via macro, to this
 * function.
 * @param format The message to log. Standard printf formatting and variable
 * arguments are allowed.
 * @param ... The comma delimited, variable substitutions to make in str. */
void basic_mud_log(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    basic_mud_vlog(format, args);
    va_end(args);
}

/** Essentially the touch command. Create an empty file or update the modified
 * time of a file.
 * @param path The filepath to "touch." This filepath is relative to the /lib
 * directory relative to the root of the mud distribution. */
int touch(const char *path)
{
    FILE *fl;

    if (!(fl = fopen(path, "a"))) {
        log1("SYSERR: %s: %s", path, strerror(errno));
        return (-1);
    } else {
        fclose(fl);
        return (0);
    }
}

/** Log mud messages to a file & to online imm's syslogs.
 * @param type The minimum syslog level that needs be set to see this message.
 * OFF, BRF, NRM and CMP are the values from lowest to highest. Using mudlog
 * with type = OFF should be avoided as every imm will see the message even
 * if they have syslog turned off.
 * @param level Minimum character level needed to see this message.
 * @param file TRUE log this to the syslog file, FALSE do not log this to disk.
 * @param str The message to log. Standard printf formatting and variable
 * arguments are allowed.
 * @param ... The comma delimited, variable substitutions to make in str. */
void mudlog(int type, int level, int file, const char *str, ...)
{
    char buf[MAX_STRING_LENGTH];
    struct descriptor_data *i;
    va_list args;

    if (str == NULL)
        return; /* eh, oh well. */

    if (file) {
        va_start(args, str);
        basic_mud_vlog(str, args);
        va_end(args);
    }

    if (level < 0)
        return;

    strcpy(buf, "[ "); /* strcpy: OK */
    va_start(args, str);
    vsnprintf(buf + 2, sizeof(buf) - 6, str, args);
    va_end(args);
    strcat(buf, " ]\r\n"); /* strcat: OK */

    for (i = descriptor_list; i; i = i->next) {
        if (STATE(i) != CON_PLAYING || IS_NPC(i->character)) /* switch */
            continue;
        if (GET_LEVEL(i->character) < level)
            continue;
        if (PLR_FLAGGED(i->character, PLR_WRITING))
            continue;
        if (type > (PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0))
            continue;

        send_to_char(i->character, "%s%s%s", CCGRN(i->character, C_NRM), buf, CCNRM(i->character, C_NRM));
    }
}

/** Take a bitvector and return a human readable
 * description of which bits are set in it.
 * @pre The final element in the names array must contain a one character
 * string consisting of a single newline character "\\n". Caller of function is
 * responsible for creating the memory buffer for the result string.
 * @param[in] bitvector The bitvector to test for set bits.
 * @param[in] names An array of human readable strings describing each possible
 * bit. The final element in this array must be a string made of a single
 * newline character (eg "\\n").
 * If you don't have a 'const' array for the names param, cast it as such.
 * @param[out] result Holds the names of the set bits in bitvector. The bit
 * names will be delimited by a single space.
 * Caller of sprintbit is responsible for creating the buffer for result.
 * Will be set to "NOBITS" if no bits are set in bitvector (ie bitvector = 0).
 * @param[in] reslen The length of the available memory in the result buffer.
 * Ideally, results will be large enough to hold the description of every bit
 * that could possibly be set in bitvector. */
size_t sprintbit(bitvector_t bitvector, const char *names[], char *result, size_t reslen)
{
    size_t len = 0;
    int nlen;
    long nr;

    *result = '\0';

    for (nr = 0; bitvector && len < reslen; bitvector >>= 1) {
        if (IS_SET(bitvector, 1)) {
            nlen = snprintf(result + len, reslen - len, "%s ", *names[nr] != '\n' ? names[nr] : "UNDEFINED");
            if (len + nlen >= reslen || nlen < 0)
                break;
            len += nlen;
        }

        if (*names[nr] != '\n')
            nr++;
    }

    if (!*result)
        len = strlcpy(result, "NOBITS ", reslen);

    return (len);
}

/** Return the human readable name of a defined type.
 * @pre The final element in the names array must contain a one character
 * string consisting of a single newline character "\\n". Caller of function is
 * responsible for creating the memory buffer for the result string.
 * @param[in] type The type number to be translated.
 * @param[in] names An array of human readable strings describing each possible
 * bit. The final element in this array must be a string made of a single
 * newline character (eg "\\n").
 * @param[out] result Holds the translated name of the type.
 * Caller of sprintbit is responsible for creating the buffer for result.
 * Will be set to "UNDEFINED" if the type is greater than the number of names
 * available.
 * @param[in] reslen The length of the available memory in the result buffer. */
size_t sprinttype(int type, const char *names[], char *result, size_t reslen)
{
    int nr = 0;

    while (type && *names[nr] != '\n') {
        type--;
        nr++;
    }

    return strlcpy(result, *names[nr] != '\n' ? names[nr] : "UNDEFINED", reslen);
}

/** Take a bitarray and return a human readable description of which bits are
 * set in it.
 * @pre The final element in the names array must contain a one character
 * string consisting of a single newline character "\\n". Caller of function is
 * responsible for creating the memory buffer for the result string large enough
 * to hold all possible bit translations. There is no error checking for
 * possible array overflow for result.
 * @param[in] bitvector The bitarray in which to test for set bits.
 * @param[in] names An array of human readable strings describing each possible
 * bit. The final element in this array must be a string made of a single
 * newline character (eg "\\n").
 * If you don't have a 'const' array for the names param, cast it as such.
 * @param[in] maxar The number of 'bytes' in the bitarray. This number will
 * usually be pre-defined for the particular bitarray you are using.
 * @param[out] result Holds the names of the set bits in bitarray. The bit
 * names are delimited by a single space. Ideally, results will be large enough
 * to hold the description of every bit that could possibly be set in bitvector.
 * Will be set to "NOBITS" if no bits are set in bitarray (ie all bits in the
 * bitarray are equal to 0).
 */
void sprintbitarray(int bitvector[], const char *names[], int maxar, char *result)
{
    int nr, teller, found = FALSE;

    *result = '\0';

    for (teller = 0; teller < maxar && !found; teller++) {
        for (nr = 0; nr < 32 && !found; nr++) {
            if (IS_SET_AR(bitvector, (teller * 32) + nr)) {
                if (*names[(teller * 32) + nr] != '\n') {
                    if (*names[(teller * 32) + nr] != '\0') {
                        strcat(result, names[(teller * 32) + nr]);
                        strcat(result, " ");
                    }
                } else {
                    strcat(result, "UNDEFINED ");
                }
            }
            if (*names[(teller * 32) + nr] == '\n')
                found = TRUE;
        }
    }

    if (!*result)
        strcpy(result, "NOBITS ");
}

/** Calculate the REAL time passed between two time invervals.
 * @todo Recommend making this function foresightedly useful by calculating
 * real months and years, too.
 * @param t2 The later time.
 * @param t1 The earlier time. */
struct time_info_data *real_time_passed(time_t t2, time_t t1)
{
    long secs;
    static struct time_info_data now;

    secs = t2 - t1;

    now.hours = (secs / SECS_PER_REAL_HOUR) % 24; /* 0..23 hours */
    secs -= SECS_PER_REAL_HOUR * now.hours;

    now.day = (secs / SECS_PER_REAL_DAY); /* 0..34 days  */
    /* secs -= SECS_PER_REAL_DAY * now.day; - Not used. */

    now.month = -1;
    now.year = -1;

    return (&now);
}

/** Calculate the MUD time passed between two time invervals.
 * @param t2 The later time.
 * @param t1 The earlier time. */
struct time_info_data *mud_time_passed(time_t t2, time_t t1)
{
    long secs;
    static struct time_info_data now;

    secs = t2 - t1;

    now.hours = (secs / SECS_PER_MUD_HOUR) % 24; /* 0..23 hours */
    secs -= SECS_PER_MUD_HOUR * now.hours;

    now.day = (secs / SECS_PER_MUD_DAY) % 35; /* 0..34 days  */
    secs -= SECS_PER_MUD_DAY * now.day;

    now.month = (secs / SECS_PER_MUD_MONTH) % 17; /* 0..16 months */
    secs -= SECS_PER_MUD_MONTH * now.month;

    now.year = (secs / SECS_PER_MUD_YEAR); /* 0..XX? years */

    return (&now);
}

/** Translate the current mud time to real seconds (in type time_t).
 * @param now The current mud time to translate into a real time unit. */
time_t mud_time_to_secs(struct time_info_data *now)
{
    time_t when = 0;

    when += now->year * SECS_PER_MUD_YEAR;
    when += now->month * SECS_PER_MUD_MONTH;
    when += now->day * SECS_PER_MUD_DAY;
    when += now->hours * SECS_PER_MUD_HOUR;
    return (time(NULL) - when);
}

/** Calculate a player's MUD age.
 * @todo The minimum starting age of 17 is hardcoded in this function. Recommend
 * changing the minimum age to a property (variable) external to this function.
 * @param ch A valid player character. */
struct time_info_data *age(struct char_data *ch)
{
    static struct time_info_data player_age;

    player_age = *mud_time_passed(time(0), ch->player.time.birth);

    player_age.year += 17; /* All players start at 17 */

    return (&player_age);
}

/** Check if making ch follow victim will create an illegal follow loop. In
 * essence, this prevents someone from following a character in a group that
 * is already being lead by the character.
 * @param ch The character trying to follow.
 * @param victim The character being followed. */
bool circle_follow(struct char_data *ch, struct char_data *victim)
{
    struct char_data *k;

    for (k = victim; k; k = k->master) {
        if (k == ch)
            return (TRUE);
    }

    return (FALSE);
}

/** Call on a character (NPC or PC) to stop them from following someone and
 * to break any charm affect.
 * @todo Make the messages returned from the broken charm affect more
 * understandable.
 * @pre ch MUST be following someone, else core dump.
 * @post The charm affect (AFF_CHARM) will be removed from the character and
 * the character will stop following the "master" they were following.
 * @param ch The character (NPC or PC) to stop from following.
 * */
void stop_follower(struct char_data *ch)
{
    struct follow_type *j, *k;

    /* Makes sure this function is not called when it shouldn't be called. */
    if (ch->master == NULL) {
        core_dump();
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM)) {
        act("Você percebe que $N é $X(um,uma) estúpid$R!", FALSE, ch, 0, ch->master, TO_CHAR);
        act("$n percebe que $N é $X(um,uma) estúpid$R!", FALSE, ch, 0, ch->master, TO_NOTVICT);
        act("$n não gosta de você!", FALSE, ch, 0, ch->master, TO_VICT);
        if (affected_by_spell(ch, SPELL_CHARM))
            affect_from_char(ch, SPELL_CHARM);
    } else {
        act("Você deixa de seguir $N.", FALSE, ch, 0, ch->master, TO_CHAR);
        act("$n deixa de seguir $N.", TRUE, ch, 0, ch->master, TO_NOTVICT);
        if (CAN_SEE(ch->master, ch))
            act("$n deixa de seguir você.", TRUE, ch, 0, ch->master, TO_VICT);
    }

    if (ch->master->followers->follower == ch) { /* Head of follower-list? */
        k = ch->master->followers;
        ch->master->followers = k->next;
        free(k);
    } else { /* locate follower who is not head of list */
        for (k = ch->master->followers; k->next->follower != ch; k = k->next)
            ;

        j = k->next;
        k->next = j->next;
        free(j);
    }

    ch->master = NULL;
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_CHARM);
}

/** Finds the number of follows that are following, and charmed by, the
 * character (PC or NPC).
 * @param ch The character to check for charmed followers.
 */
int num_followers_charmed(struct char_data *ch)
{
    struct follow_type *lackey;
    int total = 0;

    for (lackey = ch->followers; lackey; lackey = lackey->next)
        if (AFF_FLAGGED(lackey->follower, AFF_CHARM) && lackey->follower->master == ch)
            total++;

    return (total);
}

/** Called when a character that follows/is followed dies. If the character
 * is the leader of a group, it stops everyone in the group from following
 * them. Despite the title, this function does not actually perform the kill on
 * the character passed in as the argument.
 * @param ch The character (NPC or PC) to stop from following.
 * */
void die_follower(struct char_data *ch)
{
    struct follow_type *j, *k;

    if (ch->master)
        stop_follower(ch);

    for (k = ch->followers; k; k = j) {
        j = k->next;
        stop_follower(k->follower);
    }
}

/** Adds a new follower to a group.
 * @todo Maybe make circle_follow an inherent part of this function?
 * @pre Make sure to call circle_follow first. ch may also not already
 * be following anyone, otherwise core dump.
 * @param ch The character to follow.
 * @param leader The character to be followed. */
void add_follower(struct char_data *ch, struct char_data *leader)
{
    struct follow_type *k;

    if (ch->master) {
        core_dump();
        return;
    }

    ch->master = leader;

    CREATE(k, struct follow_type, 1);

    k->follower = ch;
    k->next = leader->followers;
    leader->followers = k;

    act("Você agora irá seguir $N.", FALSE, ch, 0, leader, TO_CHAR);
    if (CAN_SEE(leader, ch))
        act("$n começa a seguir você.", TRUE, ch, 0, leader, TO_VICT);
    act("$n começa a seguir $N.", TRUE, ch, 0, leader, TO_NOTVICT);
}

/** Reads the next non-blank line off of the input stream. Empty lines are
 * skipped. Lines which begin with '*' are considered to be comments and are
 * skipped.
 * @pre Caller must allocate memory for buf.
 * @post If a there is a line to be read, the newline character is removed from
 * the file line ending and the string is returned. Else a null string is
 * returned in buf.
 * @param[in] fl The file to be read from.
 * @param[out] buf The next non-blank line read from the file. Buffer given must
 * be at least READ_SIZE (256) characters large. */
int get_line(FILE *fl, char *buf)
{
    char temp[READ_SIZE];
    int lines = 0;
    int sl;

    do {
        if (!fgets(temp, READ_SIZE, fl))
            return (0);
        lines++;
    } while (*temp == '*' || *temp == '\n' || *temp == '\r');

    /* Last line of file doesn't always have a \n, but it should. */
    sl = strlen(temp);
    while (sl > 0 && (temp[sl - 1] == '\n' || temp[sl - 1] == '\r'))
        temp[--sl] = '\0';

    strcpy(buf, temp); /* strcpy: OK, if buf >= READ_SIZE (256) */
    return (lines);
}

/** Create the full path, relative to the library path, of the player type
 * file to open.
 * @todo Make the return type bool.
 * @pre Caller is responsible for allocating memory buffer for the created
 * file name.
 * @post The potential file path to open is created. This function does not
 * actually open any file descriptors.
 * @param[out] filename Buffer to store the full path of the file to open.
 * @param[in] fbufsize The maximum size of filename, and the maximum size
 * of the path that can be written to it.
 * @param[in] mode What type of files can be created. Currently, recognized
 * modes are CRASH_FILE, ETEXT_FILE, SCRIPT_VARS_FILE and PLR_FILE.
 * @param[in] orig_name The player name to create the filepath (of type mode)
 * for. */
int get_filename(char *filename, size_t fbufsize, int mode, const char *orig_name)
{
    const char *prefix, *middle, *suffix;
    char name[PATH_MAX], *ptr;

    if (orig_name == NULL || *orig_name == '\0' || filename == NULL) {
        log1("SYSERR: NULL pointer or empty string passed to get_filename(), %p or %p.", (const void *)orig_name,
             (void *)filename);
        return (0);
    }

    switch (mode) {
        case CRASH_FILE:
            prefix = LIB_PLROBJS;
            suffix = SUF_OBJS;
            break;
        case ETEXT_FILE:
            prefix = LIB_PLRTEXT;
            suffix = SUF_TEXT;
            break;
        case SCRIPT_VARS_FILE:
            prefix = LIB_PLRVARS;
            suffix = SUF_MEM;
            break;
        case PLR_FILE:
            prefix = LIB_PLRFILES;
            suffix = SUF_PLR;
            break;
        default:
            return (0);
    }

    strlcpy(name, orig_name, sizeof(name));
    for (ptr = name; *ptr; ptr++)
        *ptr = LOWER(*ptr);

    switch (LOWER(*name)) {
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
            middle = "A-E";
            break;
        case 'f':
        case 'g':
        case 'h':
        case 'i':
        case 'j':
            middle = "F-J";
            break;
        case 'k':
        case 'l':
        case 'm':
        case 'n':
        case 'o':
            middle = "K-O";
            break;
        case 'p':
        case 'q':
        case 'r':
        case 's':
        case 't':
            middle = "P-T";
            break;
        case 'u':
        case 'v':
        case 'w':
        case 'x':
        case 'y':
        case 'z':
            middle = "U-Z";
            break;
        default:
            middle = "ZZZ";
            break;
    }

    snprintf(filename, fbufsize, "%s%s" SLASH "%s.%s", prefix, middle, name, suffix);
    return (1);
}

/** Calculate the number of player characters (PCs) in the room. Any NPC (mob)
 * is not returned in the count.
 * @param room The room to check for PCs. */
int num_pc_in_room(struct room_data *room)
{
    int i = 0;
    struct char_data *ch;

    for (ch = room->people; ch != NULL; ch = ch->next_in_room)
        if (!IS_NPC(ch))
            i++;

    return (i);
}

/** This function (derived from basic fork() abort() idea by Erwin S Andreasen)
 * causes your MUD to dump core (assuming you can) but continue running. The
 * core dump will allow post-mortem debugging that is less severe than assert();
 * Don't call this directly as core_dump_unix() but as simply 'core_dump()' so
 * that it will be excluded from systems not supporting them. You still want to
 * call abort() or exit(1) for non-recoverable errors, of course. Wonder if
 * flushing streams includes sockets?
 * @param who The file in which this call was made.
 * @param line The line at which this call was made. */
void core_dump_real(const char *who, int line)
{
    log1("SYSERR: Assertion failed at %s:%d!", who, line);

#if 1 /* By default, let's not litter. */
#    if defined(CIRCLE_UNIX)
    /* These would be duplicated otherwise...make very sure. */
    fflush(stdout);
    fflush(stderr);
    fflush(logfile);
    /* Everything, just in case, for the systems that support it. */
    fflush(NULL);

    /* Kill the child so the debugger or script doesn't think the MUD crashed.
     * The 'autorun' script would otherwise run it again. */
    if (fork() == 0)
        abort();
#    endif
#endif
}

/** Count the number bytes taken up by color codes in a string that will be
 * empty space once the color codes are converted and made non-printable.
 * @param string The string in which to check for color codes. */
int count_color_chars(char *string)
{
    int i, len;
    int num = 0;

    if (!string || !*string)
        return 0;

    len = strlen(string);
    for (i = 0; i < len; i++) {
        while (string[i] == '\t') {
            if (string[i + 1] == '\t')
                num++;
            else
                num += 2;
            i += 2;
        }
    }
    return num;
}

/* Not the prettiest thing I've ever written but it does the task which
 * is counting all characters in a string which are not part of the
 * protocol system. This is with the exception of detailed MXP codes. */
int count_non_protocol_chars(char *str)
{
    int count = 0;
    char *string = str;

    while (*string) {
        if (*string == '\r' || *string == '\n') {
            string++;
            continue;
        }
        if (*string == '@' || *string == '\t') {
            string++;
            if (*string != '[' && *string != '<' && *string != '>' && *string != '(' && *string != ')')
                string++;
            else if (*string == '[') {
                while (*string && *string != ']')
                    string++;
                string++;
            } else
                string++;
            continue;
        }
        count++;
        string++;
    }

    return count;
}

/** Tests to see if a room is dark. Rules (unless overridden by ROOM_DARK):
 * Inside and City rooms are always lit. Outside rooms are dark at sunset and
 * night.
 * @todo Make the return value a bool.
 * @param room The real room to test for. */
int room_is_dark(room_rnum room)
{
    if (!VALID_ROOM_RNUM(room)) {
        log1("room_is_dark: Invalid room rnum %d. (0-%d)", room, top_of_world);
        return (FALSE);
    }

    if (world[room].light)
        return (FALSE);

    if (ROOM_FLAGGED(room, ROOM_DARK))
        return (TRUE);

    if (SECT(room) == SECT_INSIDE || SECT(room) == SECT_CITY)
        return (FALSE);

    if (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)
        return (TRUE);

    return (FALSE);
}

/** Calculates the Levenshtein distance between two strings. Currently used
 * by the mud to make suggestions to the player when commands are mistyped.
 * This function is most useful when an index of possible choices are available
 * and the results of this function are constrained and used to help narrow
 * down the possible choices. For more information about Levenshtein distance,
 * recommend doing an internet or wikipedia search.
 * @param s1 The input string.
 * @param s2 The string to be compared to. */
int levenshtein_distance(const char *s1, const char *s2)
{
    int **d, i, j;
    int s1_len = strlen(s1), s2_len = strlen(s2);

    CREATE(d, int *, s1_len + 1);

    for (i = 0; i <= s1_len; i++) {
        CREATE(d[i], int, s2_len + 1);
        d[i][0] = i;
    }

    for (j = 0; j <= s2_len; j++)
        d[0][j] = j;
    for (i = 1; i <= s1_len; i++)
        for (j = 1; j <= s2_len; j++)
            d[i][j] = MIN(d[i - 1][j] + 1, MIN(d[i][j - 1] + 1, d[i - 1][j - 1] + ((s1[i - 1] == s2[j - 1]) ? 0 : 1)));

    i = d[s1_len][s2_len];

    for (j = 0; j <= s1_len; j++)
        free(d[j]);
    free(d);

    return i;
}

/** Removes a character from a piece of furniture. Unlike some of the other
 * _from_ functions, this does not place the character into NOWHERE.
 * @post ch is unattached from the furniture object.
 * @param ch The character to remove from the furniture object.
 */
void char_from_furniture(struct char_data *ch)
{
    struct obj_data *furniture;
    struct char_data *tempch;

    if (!SITTING(ch))
        return;

    if (!(furniture = SITTING(ch))) {
        log1("SYSERR: No furniture for char in char_from_furniture.");
        SITTING(ch) = NULL;
        NEXT_SITTING(ch) = NULL;
        return;
    }

    if (!(tempch = OBJ_SAT_IN_BY(furniture))) {
        log1("SYSERR: Char from furniture, but no furniture!");
        SITTING(ch) = NULL;
        NEXT_SITTING(ch) = NULL;
        GET_OBJ_VAL(furniture, 1) = 0;
        return;
    }

    if (tempch == ch) {
        if (!NEXT_SITTING(ch)) {
            OBJ_SAT_IN_BY(furniture) = NULL;
        } else {
            OBJ_SAT_IN_BY(furniture) = NEXT_SITTING(ch);
        }
    } else {
        for (tempch = OBJ_SAT_IN_BY(furniture); tempch; tempch = NEXT_SITTING(tempch)) {
            if (NEXT_SITTING(tempch) == ch) {
                NEXT_SITTING(tempch) = NEXT_SITTING(ch);
            }
        }
    }
    GET_OBJ_VAL(furniture, 1) -= 1;
    SITTING(ch) = NULL;
    NEXT_SITTING(ch) = NULL;

    if (GET_OBJ_VAL(furniture, 1) < 1) {
        OBJ_SAT_IN_BY(furniture) = NULL;
        GET_OBJ_VAL(furniture, 1) = 0;
    }

    return;
}

/* column_list
   The list is output in a fixed format, and only the number of columns can be adjusted
   This function will output the list to the player
   Vars:
     ch          - the player
     num_cols    - the desired number of columns
     list        - a pointer to a list of strings
     list_length - So we can work with lists that don't end with /n
     show_nums   - when set to TRUE, it will show a number before the list entry.
*/
void column_list(struct char_data *ch, int num_cols, const char **list, int list_length, bool show_nums)
{
    size_t max_len = 0, len = 0, temp_len;
    int num_per_col, col_width, r, c, i, offset = 0;
    char buf[MAX_STRING_LENGTH];

    *buf = '\0';

    /* auto columns case */
    if (num_cols == 0) {
        num_cols = (IS_NPC(ch) ? 80 : GET_SCREEN_WIDTH(ch)) / (max_len + (show_nums ? 5 : 1));
    }

    /* Ensure that the number of columns is in the range 1-10 */
    num_cols = MIN(MAX(num_cols, 1), 10);

    /* Work out the longest list item */
    for (i = 0; i < list_length; i++)
        if (max_len < strlen(list[i]))
            max_len = strlen(list[i]);

    /* Calculate the width of each column */
    if (IS_NPC(ch))
        col_width = 80 / num_cols;
    else
        col_width = (GET_SCREEN_WIDTH(ch)) / num_cols;

    if (show_nums)
        col_width -= 4;

    if (col_width < 0 || (size_t)col_width < max_len)
        log1("Warning: columns too narrow for correct output to %s in simple_column_list (utils.c)", GET_NAME(ch));

    /* Calculate how many list items there should be per column */
    num_per_col = (list_length / num_cols) + ((list_length % num_cols) ? 1 : 0);

    /* Fill 'buf' with the columnised list */
    for (r = 0; r < num_per_col; r++) {
        for (c = 0; c < num_cols; c++) {
            offset = (c * num_per_col) + r;
            if (offset < list_length) {
                if (show_nums)
                    temp_len =
                        snprintf(buf + len, sizeof(buf) - len, "%2d) %-*s ", offset + 1, col_width, list[(offset)]);
                else
                    temp_len = snprintf(buf + len, sizeof(buf) - len, "%-*s ", col_width, list[(offset)]);
                len += temp_len;
            }
        }
        temp_len = snprintf(buf + len, sizeof(buf) - len, "\r\n");
        len += temp_len;
    }

    if (len >= sizeof(buf))
        snprintf((buf + MAX_STRING_LENGTH) - 22, 22, "\r\n*** OVERFLOW ***\r\n");

    /* Send the list to the player */
    page_string(ch->desc, buf, TRUE);
}

/**
 * Search through a string array of flags for a particular flag.
 * @param flag_list An array of flag name strings. The final element must
 * be a string made up of a single newline.
 * @param flag_name The name to search in flag_list.
 */
int get_flag_by_name(const char *flag_list[], char *flag_name)
{
    int i = 0;
    for (; flag_list[i] && *flag_list[i] && strcmp(flag_list[i], "\n") != 0; i++)
        if (!strcmp(flag_list[i], flag_name))
            return (i);
    return (NOFLAG);
}

/**
 * Reads a certain number of lines from the begining of a file, like performing
 * a 'head'.
 * @pre Expects an already open file and the user to supply enough memory
 * in the output buffer to hold the lines read from the file. Assumes the
 * file is a text file. Expects buf to be nulled out if the entire buf is
 * to be used, otherwise, appends file information beyond the first null
 * character. lines_to_read is assumed to be a positive number.
 * @post Rewinds the file pointer to the beginning of the file. If buf is
 * too small to handle the requested output, **OVERFLOW** is appended to the
 * buffer.
 * @param[in] file A pointer to an already successfully opened file.
 * @param[out] buf Buffer to hold the data read from the file. Will not
 * overwrite preexisting information in a non-null string.
 * @param[in] bufsize The total size of the buffer.
 * @param[in] lines_to_read The number of lines to be read from the front of
 * the file.
 */
int file_head(FILE *file, char *buf, size_t bufsize, int lines_to_read)
{
    /* Local variables */
    int lines_read = 0;                            /* The number of lines read so far. */
    char line[READ_SIZE];                          /* Retrieval buffer for file. */
    size_t buflen;                                 /* Amount of previous existing data in buffer. */
    int readstatus = 1;                            /* Are we at the end of the file? */
    int n = 0;                                     /* Return value from snprintf. */
    const char *overflow = "\r\n**OVERFLOW**\r\n"; /* Appended if overflow. */

    /* Quick check for bad arguments. */
    if (lines_to_read <= 0) {
        return lines_to_read;
    }

    /* Initialize local variables not already initialized. */
    buflen = strlen(buf);

    /* Read from the front of the file. */
    rewind(file);

    while ((lines_read < lines_to_read) && (readstatus > 0) && (buflen < bufsize)) {
        /* Don't use get_line to set lines_read because get_line will return
         * the number of comments skipped during reading. */
        readstatus = get_line(file, line);

        if (readstatus > 0) {
            n = snprintf(buf + buflen, bufsize - buflen, "%s\r\n", line);
            buflen += n;
            lines_read++;
        }
    }

    /* Check to see if we had a potential buffer overflow. */
    if (buflen >= bufsize) {
        /* We should never see this case, but... */
        if ((strlen(overflow) + 1) >= bufsize) {
            core_dump();
            snprintf(buf, bufsize, "%s", overflow);
        } else {
            /* Append the overflow statement to the buffer. */
            snprintf(buf + buflen - strlen(overflow) - 1, strlen(overflow) + 1, "%s", overflow);
        }
    }

    rewind(file);

    /* Return the number of lines. */
    return lines_read;
}

/**
 * Reads a certain number of lines from the end of the file, like performing
 * a 'tail'.
 * @pre Expects an already open file and the user to supply enough memory
 * in the output buffer to hold the lines read from the file. Assumes the
 * file is a text file. Expects buf to be nulled out if the entire buf is
 * to be used, otherwise, appends file information beyond the first null
 * character in buf. lines_to_read is assumed to be a positive number.
 * @post Rewinds the file pointer to the beginning of the file. If buf is
 * too small to handle the requested output, **OVERFLOW** is appended to the
 * buffer.
 * @param[in] file A pointer to an already successfully opened file.
 * @param[out] buf Buffer to hold the data read from the file. Will not
 * overwrite preexisting information in a non-null string.
 * @param[in] bufsize The total size of the buffer.
 * @param[in] lines_to_read The number of lines to be read from the back of
 * the file.
 */
int file_tail(FILE *file, char *buf, size_t bufsize, int lines_to_read)
{
    /* Local variables */
    int lines_read = 0;                            /* The number of lines read so far. */
    int total_lines = 0;                           /* The total number of lines in the file. */
    char c;                                        /* Used to fast forward the file. */
    char line[READ_SIZE];                          /* Retrieval buffer for file. */
    size_t buflen;                                 /* Amount of previous existing data in buffer. */
    int readstatus = 1;                            /* Are we at the end of the file? */
    int n = 0;                                     /* Return value from snprintf. */
    const char *overflow = "\r\n**OVERFLOW**\r\n"; /* Appended if overflow. */

    /* Quick check for bad arguments. */
    if (lines_to_read <= 0) {
        return lines_to_read;
    }

    /* Initialize local variables not already initialized. */
    buflen = strlen(buf);
    total_lines = file_numlines(file); /* Side effect: file is rewound. */

    /* Fast forward to the location we should start reading from */
    while (((lines_to_read + lines_read) < total_lines)) {
        do {
            c = fgetc(file);
        } while (c != '\n');

        lines_read++;
    }

    /* We reuse the lines_read counter. */
    lines_read = 0;

    /** From here on, we perform just like file_head */
    while ((lines_read < lines_to_read) && (readstatus > 0) && (buflen < bufsize)) {
        /* Don't use get_line to set lines_read because get_line will return
         * the number of comments skipped during reading. */
        readstatus = get_line(file, line);

        if (readstatus > 0) {
            n = snprintf(buf + buflen, bufsize - buflen, "%s\r\n", line);
            buflen += n;
            lines_read++;
        }
    }

    /* Check to see if we had a potential buffer overflow. */
    if (buflen >= bufsize) {
        /* We should never see this case, but... */
        if ((strlen(overflow) + 1) >= bufsize) {
            core_dump();
            snprintf(buf, bufsize, "%s", overflow);
        } else {
            /* Append the overflow statement to the buffer. */
            snprintf(buf + buflen - strlen(overflow) - 1, strlen(overflow) + 1, "%s", overflow);
        }
    }

    rewind(file);

    /* Return the number of lines read. */
    return lines_read;
}

/** Returns the byte size of a file. We assume size_t to be a large enough type
 * to handle all of the file sizes in the mud, and so do not make SIZE_MAX
 * checks.
 * @pre file parameter must already be opened.
 * @post file will be rewound.
 * @param file The file to determine the size of.
 */
size_t file_sizeof(FILE *file)
{
    size_t numbytes = 0;

    rewind(file);

    /* It would be so much easier to do a byte count if an fseek SEEK_END and
     * ftell pair of calls was portable for text files, but all information
     * I've found says that getting a file size from ftell for text files is
     * not portable. Oh well, this method should be extremely fast for the
     * relatively small filesizes in the mud, and portable, too. */
    while (!feof(file)) {
        fgetc(file);
        numbytes++;
    }

    rewind(file);

    return numbytes;
}

/** Returns the number of newlines "\\n" in a file, which we equate to number of
 * lines. We assume the int type more than adequate to count the number of lines
 * and do not make checks for overrunning INT_MAX.
 * @pre file parameter must already be opened.
 * @post file will be rewound.
 * @param file The file to determine the size of.
 */
int file_numlines(FILE *file)
{
    int numlines = 0;
    char c;

    rewind(file);

    while (!feof(file)) {
        c = fgetc(file);
        if (c == '\n') {
            numlines++;
        }
    }

    rewind(file);

    return numlines;
}

/** A string converter designed to deal with the compile sensitive IDXTYPE.
 * Relies on the friendlier strtol function.
 * @pre Assumes that NOWHERE, NOTHING, NOBODY, NOFLAG, etc are all equal.
 * @param str_to_conv A string of characters to attempt to convert to an
 * IDXTYPE number.
 */
IDXTYPE atoidx(const char *str_to_conv)
{
    long int result;

    /* Check for errors */
    errno = 0;

    result = strtol(str_to_conv, NULL, 10);

    if (errno || (result > IDXTYPE_MAX) || (result < 0))
        return NOWHERE; /* All of the NO* settings should be the same */
    else
        return (IDXTYPE)result;
}

#define isspace_ignoretabs(c) ((c) != '\t' && isspace(c))

/*
   strfrmt (String Format) function
   Used by automap/map system
   Re-formats a string to fit within a particular size box.
   Recognises @ color codes, and if a line ends in one color, the
   next line will start with the same color.
   Ends every line with \tn to prevent color bleeds.
*/
char *strfrmt(char *str, int w, int h, int justify, int hpad, int vpad)
{
    static char ret[MAX_STRING_LENGTH];
    char line[MAX_INPUT_LENGTH];
    char *sp = str;
    char *lp = line;
    char *rp = ret;
    char *wp;
    int wlen = 0, llen = 0, lcount = 0;
    char last_color = 'n';
    bool new_line_started = FALSE;

    memset(line, '\0', MAX_INPUT_LENGTH);
    /* Nomalize spaces and newlines */
    /* Split into lines, including convert \\ into \r\n */
    while (*sp) {
        /* eat leading space */
        while (*sp && isspace_ignoretabs(*sp))
            sp++;
        /* word begins */
        wp = sp;
        wlen = 0;
        while (*sp) { /* Find the end of the word */
            if (isspace_ignoretabs(*sp))
                break;
            if (*sp == '\\' && sp[1] && sp[1] == '\\') {
                if (sp != wp)
                    break; /* Finish dealing with the current word */
                sp += 2;   /* Eat the marker and any trailing space */
                while (*sp && isspace_ignoretabs(*sp))
                    sp++;
                wp = sp;
                /* Start a new line */
                if (hpad)
                    for (; llen < w; llen++)
                        *lp++ = ' ';
                *lp++ = '\r';
                *lp++ = '\n';
                *lp++ = '\0';
                rp += sprintf(rp, "%s", line);
                llen = 0;
                lcount++;
                lp = line;
            } else if (*sp == '`' || *sp == '$' || *sp == '#') {
                if (sp[1] && (sp[1] == *sp))
                    wlen++; /* One printable char here */
                sp += 2;    /* Eat the whole code regardless */
            } else if (*sp == '\t' && sp[1]) {
                char MXPcode = sp[1] == '[' ? ']' : sp[1] == '<' ? '>' : '\0';

                if (!MXPcode)
                    last_color = sp[1];

                sp += 2; /* Eat the code */
                if (MXPcode) {
                    while (*sp != '\0' && *sp != MXPcode)
                        ++sp; /* Eat the rest of the code */
                }
            } else {
                wlen++;
                sp++;
            }
        }
        if (llen + wlen + (lp == line ? 0 : 1) > w) {
            /* Start a new line */
            if (hpad)
                for (; llen < w; llen++)
                    *lp++ = ' ';
            *lp++ = '\t'; /* 'normal' color */
            *lp++ = 'n';
            *lp++ = '\r'; /* New line */
            *lp++ = '\n';
            *lp++ = '\0';
            sprintf(rp, "%s", line);
            rp += strlen(line);
            llen = 0;
            lcount++;
            lp = line;
            if (last_color != 'n') {
                *lp++ = '\t'; /* restore previous color */
                *lp++ = last_color;
                new_line_started = TRUE;
            }
        }
        /* add word to line */
        if (lp != line && new_line_started != TRUE) {
            *lp++ = ' ';
            llen++;
        }
        new_line_started = FALSE;
        llen += wlen;
        for (; wp != sp; *lp++ = *wp++)
            ;
    }
    /* Copy over the last line */
    if (lp != line) {
        if (hpad)
            for (; llen < w; llen++)
                *lp++ = ' ';
        *lp++ = '\r';
        *lp++ = '\n';
        *lp++ = '\0';
        sprintf(rp, "%s", line);
        rp += strlen(line);
        lcount++;
    }
    if (vpad) {
        while (lcount < h) {
            if (hpad) {
                memset(rp, ' ', w);
                rp += w;
            }
            *rp++ = '\r';
            *rp++ = '\n';
            lcount++;
        }
        *rp = '\0';
    }
    return ret;
}

/**
   Takes two long strings (multiple lines) and joins them side-by-side.
   Used by the automap/map system
   @param str1 The string to be displayed on the left.
   @param str2 The string to be displayed on the right.
   @param joiner ???.
*/
char *strpaste(char *str1, char *str2, char *joiner)
{
    static char ret[MAX_STRING_LENGTH + 1];
    char *sp1 = str1;
    char *sp2 = str2;
    char *rp = ret;
    int jlen = strlen(joiner);

    while ((rp - ret) < MAX_STRING_LENGTH && (*sp1 || *sp2)) {
        /* Copy line from str1 */
        while ((rp - ret) < MAX_STRING_LENGTH && *sp1 && !ISNEWL(*sp1))
            *rp++ = *sp1++;
        /* Eat the newline */
        if (*sp1) {
            if (sp1[1] && sp1[1] != sp1[0] && ISNEWL(sp1[1]))
                sp1++;
            sp1++;
        }

        /* Add the joiner */
        if ((rp - ret) + jlen >= MAX_STRING_LENGTH)
            break;
        strcpy(rp, joiner);
        rp += jlen;

        /* Copy line from str2 */
        while ((rp - ret) < MAX_STRING_LENGTH && *sp2 && !ISNEWL(*sp2))
            *rp++ = *sp2++;
        /* Eat the newline */
        if (*sp2) {
            if (sp2[1] && sp2[1] != sp2[0] && ISNEWL(sp2[1]))
                sp2++;
            sp2++;
        }

        /* Add the newline */
        if ((rp - ret) + 2 >= MAX_STRING_LENGTH)
            break;
        *rp++ = '\r';
        *rp++ = '\n';
    }
    /* Close off the string */
    *rp = '\0';
    return ret;
}

/* Create a blank affect struct */
void new_affect(struct affected_type *af)
{
    int i;
    af->spell = 0;
    af->duration = 0;
    af->modifier = 0;
    af->location = APPLY_NONE;
    for (i = 0; i < AF_ARRAY_MAX; i++)
        af->bitvector[i] = 0;
}

/* Handy function to get class ID number by name (abbreviations allowed) */
int get_class_by_name(char *classname)
{
    int i;
    for (i = 0; i < NUM_CLASSES; i++)
        if (is_abbrev(classname, pc_class_types[i]))
            return (i);

    return (-1);
}

char *convert_from_tabs(char *string)
{
    static char buf[MAX_STRING_LENGTH * 8];

    strcpy(buf, string);
    parse_tab(buf);
    return (buf);
}

// strcasestr is not standard C89/C99
// https://stackoverflow.com/questions/27303062/strstr-function-like-that-ignores-upper-or-lower-case
//
// There is more efficient version at the link above.
// This version is the easiest to understand from the link.
char *stristr3(const char *haystack, const char *needle)
{
    do {
        const char *h = haystack;
        const char *n = needle;
        while (tolower((unsigned char)*h) == tolower((unsigned char)*n) && *n) {
            h++;
            n++;
        }
        if (*n == 0) {
            return (char *)haystack;
        }
    } while (*haystack++);
    return 0;
}

struct {
    int perc;
    const char *fwd;
    const char *rev;
    char clr;
    char clrmx;
} gauge_table[] = {
    {95, ">>>>>>>>>>", "<<<<<<<<<<", 'B', 'R'}, {85, ">>>>>>>>>-", "-<<<<<<<<<", 'b', 'R'},
    {75, ">>>>>>>>--", "--<<<<<<<<", 'G', 'r'}, {65, ">>>>>>>---", "---<<<<<<<", 'g', 'Y'},
    {55, ">>>>>>----", "----<<<<<<", 'g', 'y'}, {45, ">>>>>-----", "-----<<<<<", 'y', 'y'},
    {35, ">>>>------", "------<<<<", 'y', 'g'}, {25, ">>>-------", "-------<<<", 'Y', 'g'},
    {15, ">>--------", "--------<<", 'r', 'G'}, {5, ">---------", "---------<", 'R', 'b'},
    {0, "----------", "----------", 'R', 'B'},  {-1, "    ??    ", "    ??    ", 'R', 'R'},
};

/* Functions for creating gauges (meterbars) ________________________________ */
/* -- jr - Feb 01, 2001
 * gauge(buf, type, pos, total)
 * align_gauge(buf, align)
 * Gauge functions.
 */
const char *gauge(char *buf, int type, float pos, float total)
{
    static char data[64];
    float perc;
    int i;

    if (buf == NULL)
        buf = data;

    perc = pos / total * 100.0;

    for (i = 0; gauge_table[i].perc > 0; i++)
        if (perc >= gauge_table[i].perc)
            break;

    sprintf(buf, "\tW[\t%c%s\tW]\tn",
            type & 1   ? 'g'
            : type & 2 ? gauge_table[i].clrmx
                       : gauge_table[i].clr,
            type & 3 ? gauge_table[i].rev : gauge_table[i].fwd);

    return (buf);
}

const char *align_gauge(int align)
{
    if (align >= 450)
        return ("\tW[\tR-----\tG|\tB>>>>>\tW]\tn");
    if (align >= 350)
        return ("\tW[\tR-----\tG|\tB>>>>-\tW]\tn");
    if (align >= 250)
        return ("\tW[\tR-----\tG|>>>\tB--\tW]\tn");
    if (align >= 150)
        return ("\tW[\tR-----\tG|>>\tB---\tW]\tn");
    if (align >= 50)
        return ("\tW[\tR-----\tG|>\tB----\tW]\tn");
    if (align <= -450)
        return ("\tW[\tR<<<<<\tG|\tB-----\tW]\tn");
    if (align <= -350)
        return ("\tW[\tR-<<<<\tG|\tB-----\tW]\tn");
    if (align <= -250)
        return ("\tW[\tR--\tG<<<|\tB-----\tW]\tn");
    if (align <= -150)
        return ("\tW[\tR---\tG<<|\tB-----\tW]\tn");
    if (align <= -50)
        return ("\tW[\tR----\tG<|\tB-----\tW]\tn");

    return ("\tW[\tR-----\tG|\tB-----\tW]\tn");
}

/**
 * Calcula uma pontuação para os bónus (applies e affects) de um objeto.
 * Esta função age como um "identify" para o mob.
 */
int get_item_apply_score(struct char_data *ch, struct obj_data *obj)
{
    int i, total_score = 0;

    if (obj == NULL) {
        return 0;
    }

    /* 1. Avalia os applies numéricos (bónus de stats) */
    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
        if (obj->affected[i].location != APPLY_NONE) {
            switch (obj->affected[i].location) {
                case APPLY_STR:
                case APPLY_DEX:
                case APPLY_CON:
                case APPLY_INT:
                case APPLY_WIS:
                case APPLY_CHA:
                    total_score += obj->affected[i].modifier * 5;
                    break;
                case APPLY_HIT:
                case APPLY_MANA:
                case APPLY_MOVE:
                    total_score += obj->affected[i].modifier;
                    break;
                case APPLY_AC:
                    total_score -= obj->affected[i].modifier * 5;
                    break;
                case APPLY_HITROLL:
                case APPLY_DAMROLL:
                    total_score += obj->affected[i].modifier * 10;
                    break;
                default:
                    break;
            }
        }
    }

    /* 2. Avalia os bónus de afetações (AFF_) - só dá pontos se o mob ainda não tiver o bónus */

    // --- Auras Protetoras (Muito Valiosas) ---
    if (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_SANCTUARY) && !AFF_FLAGGED(ch, AFF_SANCTUARY))
        total_score += 200;

    if (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_STONESKIN) && !AFF_FLAGGED(ch, AFF_STONESKIN))
        total_score += 180;

    if (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_GLOOMSHIELD) && !AFF_FLAGGED(ch, AFF_GLOOMSHIELD))
        total_score += 150;

    // --- Escudos de Dano (Damage Shields) ---
    if ((IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_FIRESHIELD) && !AFF_FLAGGED(ch, AFF_FIRESHIELD)) ||
        (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_THISTLECOAT) && !AFF_FLAGGED(ch, AFF_THISTLECOAT)) ||
        (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_WINDWALL) && !AFF_FLAGGED(ch, AFF_WINDWALL)))
        total_score += 120;

    // --- Bónus de Alinhamento ---
    if (IS_GOOD(ch) && IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_PROTECT_EVIL) && !AFF_FLAGGED(ch, AFF_PROTECT_EVIL))
        total_score += 100;

    if (IS_EVIL(ch) && IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_PROTECT_GOOD) && !AFF_FLAGGED(ch, AFF_PROTECT_GOOD))
        total_score += 100;

    // --- Bónus Táticos (Ofensivos/Furtivos) ---
    if ((IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_INVISIBLE) && !AFF_FLAGGED(ch, AFF_INVISIBLE)) ||
        (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_SNEAK) && !AFF_FLAGGED(ch, AFF_SNEAK)) ||
        (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_HIDE) && !AFF_FLAGGED(ch, AFF_HIDE)))
        total_score += 70;

    // --- Bónus de Deteção ---
    if (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_DETECT_INVIS) && !AFF_FLAGGED(ch, AFF_DETECT_INVIS))
        total_score += 60;

    if (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_SENSE_LIFE) && !AFF_FLAGGED(ch, AFF_SENSE_LIFE))
        total_score += 60;

    // --- Utilidades ---
    if (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_INFRAVISION) && !AFF_FLAGGED(ch, AFF_INFRAVISION))
        total_score += 30;

    if (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_LIGHT) && !AFF_FLAGGED(ch, AFF_LIGHT))
        total_score += 20;

    if (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_WATERWALK) && !AFF_FLAGGED(ch, AFF_WATERWALK))
        total_score += 15;

    if (IS_SET_AR(GET_OBJ_AFFECT(obj), AFF_FLYING) && !AFF_FLAGGED(ch, AFF_FLYING))
        total_score += 40;

    return total_score;
}

/**
 * Avalia o quão "desejável" um objeto é para um determinado mob.
 * Retorna uma pontuação numérica. Quanto maior a pontuação, mais o mob quer o item.
 */
int evaluate_item_for_mob(struct char_data *ch, struct obj_data *obj)
{
    if (obj == NULL) {
        return 0;
    }

    int score = 0;

    /* --- FILTROS INICIAIS --- */
    if (!CAN_GET_OBJ(ch, obj) || !CAN_WEAR(obj, ITEM_WEAR_TAKE))
        return 0;
    if ((IS_EVIL(ch) && OBJ_FLAGGED(obj, ITEM_ANTI_EVIL)) || (IS_GOOD(ch) && OBJ_FLAGGED(obj, ITEM_ANTI_GOOD)) ||
        (IS_NEUTRAL(ch) && OBJ_FLAGGED(obj, ITEM_ANTI_NEUTRAL)))
        return 0;

    /* --- AVALIAÇÃO POR TIPO DE ITEM --- */
    switch (GET_OBJ_TYPE(obj)) {

        case ITEM_WEAPON:
        case ITEM_FIREWEAPON: {
            struct obj_data *current_weapon = GET_EQ(ch, WEAR_WIELD);
            int new_w_stats_score = get_item_apply_score(ch, obj);

            if (GET_OBJ_TYPE(obj) == ITEM_FIREWEAPON) {
                if (mob_has_ammo(ch)) {
                    score = 150 + new_w_stats_score; /* Muito desejável! */
                } else {
                    score = 50 + new_w_stats_score; /* Útil, mas precisa de munição. */
                }
            } else { /* Arma de corpo-a-corpo */
                float new_w_dam = (float)(GET_OBJ_VAL(obj, 1) * (GET_OBJ_VAL(obj, 2) + 1)) / 2.0;
                if (current_weapon == NULL || GET_OBJ_TYPE(current_weapon) == ITEM_FIREWEAPON) {
                    score = 100 + new_w_stats_score;
                } else {
                    float old_w_dam =
                        (float)(GET_OBJ_VAL(current_weapon, 1) * (GET_OBJ_VAL(current_weapon, 2) + 1)) / 2.0;
                    int old_w_stats_score = get_item_apply_score(ch, current_weapon);
                    int total_improvement =
                        ((int)(new_w_dam - old_w_dam) * 10) + (new_w_stats_score - old_w_stats_score);
                    if (total_improvement > 0)
                        score = total_improvement;
                }
            }
            break;
        }

        case ITEM_AMMO: {
            struct obj_data *fireweapon = GET_EQ(ch, WEAR_WIELD);
            if (fireweapon && GET_OBJ_TYPE(fireweapon) == ITEM_FIREWEAPON) {
                /* Se já tem a arma, a munição é muito valiosa. */
                /* A pontuação é baseada no dano potencial da munição. */
                score = 80 + (GET_OBJ_VAL(obj, 1) * GET_OBJ_VAL(obj, 2));
            } else {
                /* Sem a arma, a munição é quase inútil. */
                score = 5;
            }
            break;
        }

        case ITEM_ARMOR:
        case ITEM_WINGS: {
            int wear_pos = find_eq_pos(ch, obj, NULL);
            if (wear_pos != -1) {
                struct obj_data *current_armor = GET_EQ(ch, wear_pos);
                int new_armor_stats_score = get_item_apply_score(ch, obj) - (GET_OBJ_VAL(obj, 0) * 5);

                if (current_armor == NULL) {
                    score = 50 + new_armor_stats_score;
                } else {
                    int old_armor_stats_score =
                        get_item_apply_score(ch, current_armor) - (GET_OBJ_VAL(current_armor, 0) * 5);
                    if (new_armor_stats_score > old_armor_stats_score) {
                        score = new_armor_stats_score - old_armor_stats_score;
                    }
                }
            }
            break;
        }

        case ITEM_LIGHT: {
            bool has_light = FALSE;
            if (IS_DARK(IN_ROOM(ch)) && !IS_AFFECTED(ch, AFF_INFRAVISION)) {
                int i;
                for (i = 0; i < NUM_WEARS; i++) {
                    if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_LIGHT) {
                        has_light = TRUE;
                        break;
                    }
                }
                if (!has_light)
                    score = 75;
            }
            break;
        }

        case ITEM_POTION:
        case ITEM_SCROLL:
        case ITEM_WAND:
        case ITEM_STAFF:
            score = GET_OBJ_VAL(obj, 0) * 2;
            break;

        case ITEM_KEY:
            score = 30;
            break;

        case ITEM_BOAT: {
            bool near_water = FALSE;
            int door;
            room_rnum adjacent_room;

            for (door = 0; door < DIR_COUNT; door++) {
                if (world[IN_ROOM(ch)].dir_option[door] &&
                    (adjacent_room = world[IN_ROOM(ch)].dir_option[door]->to_room) != NOWHERE) {

                    if (world[adjacent_room].sector_type == SECT_WATER_SWIM ||
                        world[adjacent_room].sector_type == SECT_WATER_NOSWIM) {
                        near_water = TRUE;
                        break;
                    }
                }
            }
            if (near_water) {
                score = 200;
            } else {
                score = 5;
            }
            break;
        }

        case ITEM_CONTAINER: {
            int capacity_score = GET_OBJ_VAL(obj, 0) / 2; /* Pontuação base pela capacidade. */
            int need_bonus = 0;

            /* Se o inventário estiver mais de 75% cheio, o bónus de necessidade é alto. */
            if (IS_CARRYING_N(ch) > (CAN_CARRY_N(ch) * 0.75)) {
                need_bonus = 100;
            }

            score = capacity_score + need_bonus;
            break;
        }

        case ITEM_FOOD:
            if (GET_COND(ch, HUNGER) >= 0 && GET_COND(ch, HUNGER) < 10) {
                score = 40;
            }
            break;

        case ITEM_DRINKCON:
            if (GET_COND(ch, THIRST) >= 0 && GET_COND(ch, THIRST) < 10) {
                score = 40;
            }
            break;

        default:
            score = GET_OBJ_COST(obj) / 100;
            break;
    }
    return score;
}

/* Adicione estas novas funções ao ficheiro */

/**
 * Verifica se um item é o "último" do seu tipo no inventário do mob.
 * Para poções/pergaminhos, verifica a quantidade. Para varinhas/bastões, as cargas.
 * @param ch O mob que possui o item.
 * @param obj O item a ser verificado.
 * @return TRUE se for o último, FALSE caso contrário.
 */
bool is_last_consumable(struct char_data *ch, struct obj_data *obj)
{
    if (!ch || !obj)
        return TRUE; /* Segurança: se algo for nulo, não usa. */

    switch (GET_OBJ_TYPE(obj)) {
        case ITEM_POTION:
        case ITEM_SCROLL:
            /* Retorna TRUE se a contagem deste VNUM no inventário for 1 ou menos. */
            return (count_obj_in_list(GET_OBJ_VNUM(obj), ch->carrying) <= 1);

        case ITEM_WAND:
        case ITEM_STAFF:
            /* Retorna TRUE se o número de cargas restantes for 1 ou menos. */
            return (GET_OBJ_VAL(obj, 2) <= 1);

        default:
            /* Outros tipos de item não são "consumíveis" desta forma. */
            return FALSE;
    }
}

/**
 * Consome um item após o seu uso bem-sucedido.
 * Poções/pergaminhos são destruídos. Varinhas/bastões perdem uma carga.
 * @param ch O mob que usou o item.
 * @param obj O item que foi usado.
 */
void consume_item_after_use(struct char_data *ch, struct obj_data *obj)
{
    if (!ch || !obj)
        return;

    switch (GET_OBJ_TYPE(obj)) {
        case ITEM_POTION:
        case ITEM_SCROLL:
            /* Itens de uso único são extraídos do jogo. */
            extract_obj(obj);
            break;
        case ITEM_WAND:
        case ITEM_STAFF:
            /* Itens de cargas múltiplas perdem uma carga. */
            GET_OBJ_VAL(obj, 2) -= 1;
            break;
        default:
            /* Outros tipos de item não são consumidos. */
            break;
    }
}

/* Adicione esta função ao final do utils.c */

/**
 * Conta o número de ocorrências de um objeto com um VNUM específico
 * numa lista de objetos.
 * @param vnum O VNUM do objeto a ser contado.
 * @param list Um ponteiro para o início da lista de objetos (ex: ch->carrying).
 * @return O número de vezes que o objeto foi encontrado.
 */
int count_obj_in_list(obj_vnum vnum, struct obj_data *list)
{
    int count = 0;
    struct obj_data *i;

    for (i = list; i; i = i->next_content) {
        if (GET_OBJ_VNUM(i) == vnum) {
            count++;
        }
    }
    return count;
}

/*
 * ===============================================
 * WISHLIST MANAGEMENT FUNCTIONS FOR MOB AI
 * ===============================================
 */

/**
 * Adiciona um item à wishlist de um mob.
 * @param ch O mob que deseja o item
 * @param vnum VNUM do item desejado
 * @param priority Prioridade do item (score do evaluate_item_for_mob)
 */
void add_item_to_wishlist(struct char_data *ch, obj_vnum vnum, int priority)
{
    struct mob_wishlist_item *item, *prev, *current;

    if (!IS_NPC(ch) || !ch->ai_data)
        return;

    /* Verifica se o item já está na wishlist */
    if (find_item_in_wishlist(ch, vnum))
        return;

    /* Cria novo item da wishlist */
    CREATE(item, struct mob_wishlist_item, 1);
    item->vnum = vnum;
    item->priority = priority;
    item->added_time = time(0);
    item->next = NULL;

    /* Insere na lista ordenada por prioridade (maior prioridade primeiro) */
    if (!ch->ai_data->wishlist || ch->ai_data->wishlist->priority < priority) {
        item->next = ch->ai_data->wishlist;
        ch->ai_data->wishlist = item;
        return;
    }

    prev = NULL;
    current = ch->ai_data->wishlist;
    while (current && current->priority >= priority) {
        prev = current;
        current = current->next;
    }

    prev->next = item;
    item->next = current;
}

/**
 * Remove um item da wishlist de um mob.
 * @param ch O mob
 * @param vnum VNUM do item a ser removido
 */
void remove_item_from_wishlist(struct char_data *ch, obj_vnum vnum)
{
    struct mob_wishlist_item *current, *prev;

    if (!IS_NPC(ch) || !ch->ai_data || !ch->ai_data->wishlist)
        return;

    /* Se é o primeiro item */
    if (ch->ai_data->wishlist->vnum == vnum) {
        current = ch->ai_data->wishlist;
        ch->ai_data->wishlist = current->next;
        free(current);
        return;
    }

    /* Procura o item na lista */
    prev = ch->ai_data->wishlist;
    current = prev->next;
    while (current) {
        if (current->vnum == vnum) {
            prev->next = current->next;
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

/**
 * Encontra um item específico na wishlist.
 * @param ch O mob
 * @param vnum VNUM do item procurado
 * @return Ponteiro para o item ou NULL se não encontrado
 */
struct mob_wishlist_item *find_item_in_wishlist(struct char_data *ch, obj_vnum vnum)
{
    struct mob_wishlist_item *current;

    if (!IS_NPC(ch) || !ch->ai_data)
        return NULL;

    for (current = ch->ai_data->wishlist; current; current = current->next) {
        if (current->vnum == vnum) {
            return current;
        }
    }
    return NULL;
}

/**
 * Retorna o item de maior prioridade na wishlist.
 * @param ch O mob
 * @return Ponteiro para o item de maior prioridade ou NULL se lista vazia
 */
struct mob_wishlist_item *get_top_wishlist_item(struct char_data *ch)
{
    if (!IS_NPC(ch) || !ch->ai_data)
        return NULL;
    return ch->ai_data->wishlist;
}

/**
 * Limpa toda a wishlist de um mob.
 * @param ch O mob
 */
void clear_wishlist(struct char_data *ch)
{
    struct mob_wishlist_item *current, *next;

    if (!IS_NPC(ch) || !ch->ai_data)
        return;

    current = ch->ai_data->wishlist;
    while (current) {
        next = current->next;
        free(current);
        current = next;
    }
    ch->ai_data->wishlist = NULL;
}

/**
 * Observa o equipamento de um alvo durante o combate e adiciona itens interessantes à wishlist.
 * @param observer O mob que está observando
 * @param target O alvo sendo observado
 */
void observe_combat_equipment(struct char_data *observer, struct char_data *target)
{
    int i, score;
    struct obj_data *eq;

    if (!IS_NPC(observer) || !observer->ai_data || !target)
        return;

    /* Observa o equipamento do alvo */
    for (i = 0; i < NUM_WEARS; i++) {
        eq = GET_EQ(target, i);
        if (!eq)
            continue;

        /* Avalia se o item seria útil para o observador */
        score = evaluate_item_for_mob(observer, eq);

        /* Se a pontuação for muito alta (>100), adiciona à wishlist */
        if (score > 100) {
            add_item_to_wishlist(observer, GET_OBJ_VNUM(eq), score);
        }
    }
}

/**
 * Encontra um mob que pode dropar um item específico.
 * @param item_vnum VNUM do item procurado
 * @return RNUM do mob que pode dropar o item, ou NOBODY se não encontrado
 */
mob_rnum find_mob_with_item(obj_vnum item_vnum)
{
    mob_rnum mob;
    int i;

    /* Procura nos protótipos de mobs para ver quem carrega este item */
    for (mob = 0; mob <= top_of_mobt; mob++) {
        /* Verifica equipamento padrão */
        for (i = 0; i < NUM_WEARS; i++) {
            if (mob_proto[mob].equipment[i] && GET_OBJ_VNUM(mob_proto[mob].equipment[i]) == item_vnum) {
                return mob;
            }
        }

        /* Verifica inventário padrão */
        /* Note: Esta é uma implementação simplificada.
         * Um sistema mais sofisticado verificaria as chances de drop. */
    }

    return NOBODY;
}

/**
 * Encontra uma loja que vende um item específico.
 * @param item_vnum VNUM do item procurado
 * @return RNUM da loja que vende o item, ou NOTHING se não encontrada
 */
shop_rnum find_shop_selling_item(obj_vnum item_vnum)
{
    int shop, i;

    /* Procura nas lojas */
    for (shop = 0; shop <= top_shop; shop++) {
        /* Verifica se a loja produz este item */
        for (i = 0; shop_index[shop].producing[i] != NOTHING; i++) {
            if (shop_index[shop].producing[i] == item_vnum) {
                return shop;
            }
        }
    }

    return NOTHING;
}

/**
 * Verifica se um mob tem ouro suficiente para comprar um item.
 * @param ch O mob
 * @param item_vnum VNUM do item
 * @return TRUE se pode comprar, FALSE caso contrário
 */
bool mob_can_afford_item(struct char_data *ch, obj_vnum item_vnum)
{
    shop_rnum shop;
    obj_rnum obj_rnum;
    int cost;

    if (!IS_NPC(ch))
        return FALSE;

    /* Encontra a loja que vende o item */
    shop = find_shop_selling_item(item_vnum);
    if (shop == NOTHING)
        return FALSE;

    /* Calcula o custo do item */
    obj_rnum = real_object(item_vnum);
    if (obj_rnum == NOTHING)
        return FALSE;

    cost = GET_OBJ_COST(&obj_proto[obj_rnum]);
    if (cost <= 0)
        cost = 1;

    /* Aplica a margem de lucro da loja */
    cost = (int)(cost * SHOP_BUYPROFIT(shop));

    return (GET_GOLD(ch) >= cost);
}

/**
 * Check if a mob should be excluded from being a quest target.
 * Excludes summoned and charmed creatures.
 * @param mob The mob to check
 * @return TRUE if mob should be excluded from quests, FALSE otherwise
 */
bool is_mob_excluded_from_quests(struct char_data *mob)
{
    if (!IS_NPC(mob)) {
        return FALSE;
    }

    /* Check if mob is charmed/summoned (has a master) */
    if (mob->master != NULL) {
        return TRUE;
    }

    /* Check if mob is affected by charm (summoned creatures) */
    if (AFF_FLAGGED(mob, AFF_CHARM)) {
        return TRUE;
    }

    return FALSE;
}

/**
 * Count the number of mob-posted quests currently in the system.
 * @return Number of quests with the AQ_MOB_POSTED flag
 */
int count_mob_posted_quests(void)
{
    int count = 0;
    qst_rnum rnum;

    for (rnum = 0; rnum < total_quests; rnum++) {
        if (IS_SET(QST_FLAGS(rnum), AQ_MOB_POSTED)) {
            count++;
        }
    }

    return count;
}

/**
 * Check if we can add another mob-posted quest without exceeding the limit.
 * @return TRUE if we can add another quest, FALSE if at limit
 */
bool can_add_mob_posted_quest(void)
{
    const int MAX_MOB_POSTED_QUESTS = 150;
    return (count_mob_posted_quests() < MAX_MOB_POSTED_QUESTS);
}

/**
 * Faz um mob postar uma quest para obter um item.
 * Esta é uma implementação básica que simula a postagem de uma quest.
 * Em uma implementação futura, isto seria integrado com o sistema de quest boards.
 * @param ch O mob que posta a quest
 * @param item_vnum VNUM do item desejado
 * @param reward Recompensa oferecida
 */
/* Helper function to select an appropriate item from mob's inventory as reward */
obj_vnum select_mob_inventory_reward(struct char_data *ch, int difficulty)
{
    struct obj_data *obj, *best_obj = NULL;
    int best_value = 0;
    int target_value = difficulty * 100; /* Target item value based on difficulty */

    if (!IS_NPC(ch)) {
        return NOTHING;
    }

    /* Iterate through mob's inventory to find suitable reward item */
    for (obj = ch->carrying; obj; obj = obj->next_content) {
        /* Skip quest items, containers with contents, and nodrop items */
        if (OBJ_FLAGGED(obj, ITEM_QUEST) || OBJ_FLAGGED(obj, ITEM_NODROP) ||
            (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains)) {
            continue;
        }

        /* Skip essential equipment (weapons, armor the mob is wearing) */
        if (obj->worn_by == ch) {
            continue;
        }

        /* Calculate item value considering type and stats */
        int item_value = GET_OBJ_COST(obj);
        if (item_value <= 0) {
            item_value = 10; /* Minimum value for worthless items */
        }

        /* Prefer items close to target value but not too expensive */
        if (item_value <= target_value * 2 && item_value > best_value) {
            best_obj = obj;
            best_value = item_value;
        }
    }

    return best_obj ? GET_OBJ_VNUM(best_obj) : NOTHING;
}

void mob_posts_quest(struct char_data *ch, obj_vnum item_vnum, int reward)
{
    obj_rnum obj_rnum;
    char *item_name = "um item";
    mob_vnum questmaster_vnum;
    qst_vnum new_quest_vnum;
    struct aq_data *new_quest;
    int difficulty;
    int calculated_reward;
    zone_rnum mob_zone;
    char quest_name[MAX_QUEST_NAME];
    char quest_desc[MAX_QUEST_DESC];
    char quest_info[MAX_QUEST_MSG];
    char quest_done[MAX_QUEST_MSG];
    obj_vnum reward_item = NOTHING;
    ush_int reward_item_rnum;
    char *reward_item_name = "";

    if (!IS_NPC(ch) || !ch->ai_data) {
        return;
    }

    /* Check if we've reached the limit of mob-posted quests */
    if (!can_add_mob_posted_quest()) {
        return;
    }

    /* Check if there's already an active quest for this item from this mob */
    for (int i = 0; i < total_quests; i++) {
        if (QST_RETURNMOB(i) == GET_MOB_VNUM(ch) && QST_TARGET(i) == item_vnum && QST_TYPE(i) == AQ_OBJ_RETURN) {
            /* Already have an active quest for this item from this mob */
            return;
        }
    }

    /* Obtém informações sobre o item */
    obj_rnum = real_object(item_vnum);
    if (obj_rnum != NOTHING) {
        item_name = obj_proto[obj_rnum].short_description;

        /* Não posta quest para itens que não podem ser pegos */
        if (!CAN_WEAR(&obj_proto[obj_rnum], ITEM_WEAR_TAKE)) {
            log1("WISHLIST QUEST: %s tried to post quest for untakeable item %d (%s)", GET_NAME(ch), item_vnum,
                 item_name);
            remove_item_from_wishlist(ch, item_vnum);
            return;
        }
    }

    /* Encontra zona do mob */
    mob_zone = world[IN_ROOM(ch)].zone;

    /* Enhancement 5: Find questmaster with zone-based preferences */
    questmaster_vnum = find_questmaster_for_zone_enhanced(mob_zone, ch);
    if (questmaster_vnum == NOBODY) {
        /* Fallback to basic questmaster finding */
        questmaster_vnum = find_questmaster_for_zone(mob_zone);
    }
    if (questmaster_vnum == NOBODY) {
        /* Não há questmaster disponível, usa implementação anterior */
        log1("WISHLIST QUEST: No questmaster found for %s in zone %d, using old implementation", GET_NAME(ch),
             mob_zone);
        GET_GOLD(ch) -= MIN(reward, GET_GOLD(ch));
        remove_item_from_wishlist(ch, item_vnum);
        act("$n parece frustrado por não conseguir postar sua solicitação.", FALSE, ch, 0, 0, TO_ROOM);
        return;
    }

    /* Calcula dificuldade e recompensa adequada */
    difficulty = calculate_quest_difficulty(item_vnum);
    calculated_reward = calculate_quest_reward(ch, item_vnum, difficulty);

    /* Enhancement 1: Select item reward from mob's inventory */
    reward_item = select_mob_inventory_reward(ch, difficulty);

    /* Verifica se o mob tem ouro suficiente */
    if (GET_GOLD(ch) < calculated_reward) {
        calculated_reward = MAX(50, GET_GOLD(ch) / 2);
    }

    /* Cria uma nova quest */
    CREATE(new_quest, struct aq_data, 1);

    /* Gera VNUM único para a nova quest baseado na zona */
    new_quest_vnum = zone_table[mob_zone].bot + 9000 + (time(0) % 1000);
    while (real_quest(new_quest_vnum) != NOTHING) {
        new_quest_vnum++;
        if (new_quest_vnum > zone_table[mob_zone].top + 9000) {
            new_quest_vnum = zone_table[mob_zone].bot + 9000;
        }
    }

    /* Configura a quest */
    new_quest->vnum = new_quest_vnum;
    new_quest->type = AQ_OBJ_RETURN;
    new_quest->qm = questmaster_vnum;
    new_quest->target = item_vnum;
    new_quest->prereq = NOTHING;
    new_quest->flags = AQ_MOB_POSTED; /* Marca como postada por mob */
    new_quest->prev_quest = NOTHING;
    new_quest->next_quest = NOTHING;
    new_quest->func = NULL;

    /* Enhancement 6: Questpoints as rewards for high difficulty + reputation */
    int base_questpoints = calculated_reward / 10;

    /* Enhancement 6: Bonus questpoints for high difficulty quests */
    if (difficulty >= 80) {
        base_questpoints += 3; /* High difficulty bonus */
    } else if (difficulty >= 60) {
        base_questpoints += 2; /* Medium difficulty bonus */
    }

    /* Enhancement 6: Additional questpoints for requesting mob with good karma */
    if (GET_KARMA(ch) > 500) {
        base_questpoints += 2; /* High karma bonus */
    } else if (GET_KARMA(ch) > 200) {
        base_questpoints += 1; /* Good karma bonus */
    }

    /* Enhancement 6: Ensure minimum questpoints for very difficult quests */
    if (difficulty >= 90 && GET_KARMA(ch) > 300) {
        base_questpoints = MAX(base_questpoints, 5); /* Minimum 5 QP for elite quests */
    }

    /* Configura valores da quest */
    new_quest->value[0] = URANGE(1, base_questpoints, 10);         /* Enhanced Questpoints reward */
    new_quest->value[1] = 0;                                       /* Penalty */
    new_quest->value[2] = MAX(1, GET_LEVEL(ch) - 10);              /* Min level */
    new_quest->value[3] = MIN(LVL_IMMORT - 1, GET_LEVEL(ch) + 15); /* Max level */
    new_quest->value[4] = -1;                                      /* No time limit */
    new_quest->value[5] = GET_MOB_VNUM(ch);                        /* Return mob */
    new_quest->value[6] = 1;                                       /* Quantity */

    /* Determina se a quest deve ser repetível baseado no tipo de item */
    if (obj_rnum != NOTHING) {
        int obj_type = GET_OBJ_TYPE(&obj_proto[obj_rnum]);
        /* Quests para itens consumíveis (poções, pergaminhos, comida) são repetíveis */
        if (obj_type == ITEM_POTION || obj_type == ITEM_SCROLL || obj_type == ITEM_FOOD || obj_type == ITEM_DRINKCON) {
            SET_BIT(new_quest->flags, AQ_REPEATABLE);
        }
    }

    /* Configura recompensas */
    new_quest->gold_reward = calculated_reward;
    new_quest->exp_reward = calculated_reward * 2;
    new_quest->obj_reward = reward_item; /* Enhancement 1: Item from mob inventory */

    /* Cria strings da quest */
    snprintf(quest_name, sizeof(quest_name), "Buscar %s", item_name);
    snprintf(quest_desc, sizeof(quest_desc), "Buscar e trazer %s", item_name);

    /* Check if questmaster is different from requester */
    mob_rnum qm_mob_rnum = real_mobile(questmaster_vnum);
    struct char_data *qm_char = NULL;
    if (qm_mob_rnum != NOBODY) {
        /* Find questmaster in world to get their name */
        for (qm_char = character_list; qm_char; qm_char = qm_char->next) {
            if (IS_NPC(qm_char) && GET_MOB_RNUM(qm_char) == qm_mob_rnum) {
                break;
            }
        }
    }

    /* Get reward item name for quest info */
    if (reward_item != NOTHING) {
        reward_item_rnum = real_object(reward_item);
        if (reward_item_rnum != NOTHING) {
            reward_item_name = obj_proto[reward_item_rnum].short_description;
        }
    }

    if (qm_char && qm_char != ch) {
        if (reward_item != NOTHING) {
            snprintf(quest_info, sizeof(quest_info),
                     "%s está procurando por %s. Se você encontrar este item, "
                     "traga-o para mim ou diretamente para %s para receber %d moedas de ouro, "
                     "%d pontos de experiência e %s como recompensa.",
                     GET_NAME(ch), item_name, GET_NAME(ch), calculated_reward, calculated_reward * 2, reward_item_name);
        } else {
            snprintf(quest_info, sizeof(quest_info),
                     "%s está procurando por %s. Se você encontrar este item, "
                     "traga-o para mim ou diretamente para %s para receber sua recompensa.",
                     GET_NAME(ch), item_name, GET_NAME(ch));
        }
    } else {
        if (reward_item != NOTHING) {
            snprintf(quest_info, sizeof(quest_info),
                     "%s está procurando por %s. Se você encontrar este item, "
                     "traga-o de volta para %s para receber %d moedas de ouro, "
                     "%d pontos de experiência e %s como recompensa.",
                     GET_NAME(ch), item_name, GET_NAME(ch), calculated_reward, calculated_reward * 2, reward_item_name);
        } else {
            snprintf(quest_info, sizeof(quest_info),
                     "%s está procurando por %s. Se você encontrar este item, "
                     "traga-o de volta para %s para receber sua recompensa.",
                     GET_NAME(ch), item_name, GET_NAME(ch));
        }
    }
    snprintf(quest_done, sizeof(quest_done),
             "Excelente! Você trouxe exatamente o que eu precisava. "
             "Muito obrigado pela sua ajuda!");

    new_quest->name = str_udup(quest_name);
    new_quest->desc = str_udup(quest_desc);
    new_quest->info = str_udup(quest_info);
    new_quest->done = str_udup(quest_done);
    new_quest->quit = str_udup("Que pena que você desistiu desta busca.");

    /* Adiciona a quest ao sistema */
    if (add_quest(new_quest) < 0) {
        log1("SYSERR: Failed to add wishlist quest for %s (item %d)", GET_NAME(ch), item_vnum);
        free_quest(new_quest);
        return;
    }

    /* Verify the quest was added correctly */
    qst_rnum added_rnum = real_quest(new_quest_vnum);
    if (added_rnum == NOTHING) {
        log1("SYSERR: Quest %d was supposedly added but can't be found in quest table", new_quest_vnum);
        return;
    }

    log1("QUEST: Successfully added quest %d (rnum %d) to quest table, assigned to QM %d", new_quest_vnum, added_rnum,
         questmaster_vnum);

    /* Check if mob can reach questmaster, if not make it a temporary questmaster */
    make_mob_temp_questmaster_if_needed(ch, new_quest_vnum);

    /* Deduz o ouro do mob (only once) */
    GET_GOLD(ch) -= calculated_reward;

    /* Remove da wishlist temporariamente */
    remove_item_from_wishlist(ch, item_vnum);

    /* Aumenta quest_tendency por postar uma quest */
    if (ch->ai_data->genetics.quest_tendency < 100) {
        ch->ai_data->genetics.quest_tendency = MIN(100, ch->ai_data->genetics.quest_tendency + 1);
    }

    /* Mensagens para jogadores */
    act("$n escreve algo num pergaminho e o envia para o questmaster.", FALSE, ch, 0, 0, TO_ROOM);

    /* Log da ação */
    log1("WISHLIST QUEST: %s (room %d) created quest %d for item %d (%s) with QM %d, reward %d gold", GET_NAME(ch),
         GET_ROOM_VNUM(IN_ROOM(ch)), new_quest_vnum, item_vnum, item_name, questmaster_vnum, calculated_reward);

    /* Enhancement 3: Quest Persistence - Mark as persistent quest */
    /* Add a special flag to identify this as a wishlist quest that should persist */
    SET_BIT(new_quest->flags, AQ_REPEATABLE); /* Most wishlist quests should be repeatable */

    /* Enhancement 3: Quest Persistence - Save quest to disk for server restarts */
    if (save_quests(mob_zone)) {
        log1("WISHLIST QUEST: Saved quest %d to disk for persistence across restarts", new_quest_vnum);
    } else {
        log1("SYSERR: Failed to save wishlist quest %d to disk", new_quest_vnum);
    }
}

/**
 * Faz um mob postar uma quest de combate (AQ_PLAYER_KILL ou AQ_MOB_KILL_BOUNTY).
 * @param ch O mob que posta a quest
 * @param quest_type Tipo da quest (AQ_PLAYER_KILL ou AQ_MOB_KILL_BOUNTY)
 * @param target_vnum VNUM do alvo (mob vnum para bounty, NOTHING para player kill)
 * @param reward Recompensa em ouro oferecida
 */
void mob_posts_combat_quest(struct char_data *ch, int quest_type, int target_vnum, int reward)
{
    mob_vnum questmaster_vnum;
    qst_vnum new_quest_vnum;
    struct aq_data *new_quest;
    int difficulty;
    int calculated_reward;
    zone_rnum mob_zone;
    char quest_name[MAX_QUEST_NAME];
    char quest_desc[MAX_QUEST_DESC];
    char quest_info[MAX_QUEST_MSG];
    char quest_done[MAX_QUEST_MSG];
    obj_vnum reward_item = NOTHING;
    char *target_name = "alvo desconhecido";
    mob_rnum target_mob_rnum = NOBODY;

    if (!IS_NPC(ch) || !ch->ai_data) {
        return;
    }

    /* Check if we've reached the limit of mob-posted quests */
    if (!can_add_mob_posted_quest()) {
        return;
    }

    /* Validar tipo de quest */
    if (quest_type != AQ_PLAYER_KILL && quest_type != AQ_MOB_KILL_BOUNTY) {
        log1("SYSERR: Invalid combat quest type %d from %s", quest_type, GET_NAME(ch));
        return;
    }

    /* Obter nome do alvo para quests de bounty */
    if (quest_type == AQ_MOB_KILL_BOUNTY && target_vnum != NOTHING) {
        target_mob_rnum = real_mobile(target_vnum);
        if (target_mob_rnum != NOBODY) {
            target_name = mob_proto[target_mob_rnum].player.short_descr;
        }
    }

    /* Encontra zona do mob */
    mob_zone = world[IN_ROOM(ch)].zone;

    /* Encontra questmaster */
    questmaster_vnum = find_questmaster_for_zone_enhanced(mob_zone, ch);
    if (questmaster_vnum == NOBODY) {
        questmaster_vnum = find_questmaster_for_zone(mob_zone);
    }
    if (questmaster_vnum == NOBODY) {
        log1("COMBAT QUEST: No questmaster found for %s in zone %d", GET_NAME(ch), mob_zone);
        act("$n parece frustrado por não conseguir postar sua solicitação de ajuda.", FALSE, ch, 0, 0, TO_ROOM);
        return;
    }

    /* Calcula dificuldade baseada no tipo de quest */
    if (quest_type == AQ_PLAYER_KILL) {
        difficulty = 70 + rand_number(0, 20); /* Player kills são naturalmente difíceis */
    } else {
        /* Para bounty quests, dificuldade baseada no nível do alvo */
        if (target_mob_rnum != NOBODY) {
            int target_level = mob_proto[target_mob_rnum].player.level;
            difficulty = MIN(90, MAX(30, target_level * 3));
        } else {
            difficulty = 60; /* Default para alvos desconhecidos */
        }
    }

    /* Calcula recompensa adequada */
    calculated_reward = reward + (difficulty * 2);

    /* Seleciona item de recompensa do inventário do mob */
    reward_item = select_mob_inventory_reward(ch, difficulty);

    /* Verifica se o mob tem ouro suficiente */
    if (GET_GOLD(ch) < calculated_reward) {
        calculated_reward = MAX(100, GET_GOLD(ch) / 2); /* Quests de combate têm recompensa mínima maior */
    }

    /* Cria uma nova quest */
    CREATE(new_quest, struct aq_data, 1);

    /* Gera VNUM único para a nova quest */
    new_quest_vnum = zone_table[mob_zone].bot + 9000 + (time(0) % 1000);
    while (real_quest(new_quest_vnum) != NOTHING) {
        new_quest_vnum++;
        if (new_quest_vnum > zone_table[mob_zone].top + 9000) {
            new_quest_vnum = zone_table[mob_zone].bot + 9000;
        }
    }

    /* Configura a quest */
    new_quest->vnum = new_quest_vnum;
    new_quest->type = quest_type;
    new_quest->qm = questmaster_vnum;
    new_quest->target = (quest_type == AQ_MOB_KILL_BOUNTY) ? target_vnum : NOTHING;
    new_quest->prereq = NOTHING;
    new_quest->flags = AQ_MOB_POSTED; /* Marca como postada por mob */
    new_quest->prev_quest = NOTHING;
    new_quest->next_quest = NOTHING;
    new_quest->func = NULL;

    /* Configura valores da quest */
    int base_questpoints = calculated_reward / 15;

    /* Bonus para quests de combate difíceis */
    if (difficulty >= 80) {
        base_questpoints += 4;
    } else if (difficulty >= 60) {
        base_questpoints += 2;
    }

    /* Bonus para mobs com boa reputação */
    if (GET_MOB_REPUTATION(ch) > 70) {
        base_questpoints += 2;
    } else if (GET_MOB_REPUTATION(ch) > 40) {
        base_questpoints += 1;
    }

    new_quest->value[0] = URANGE(2, base_questpoints, 15);         /* Questpoints reward */
    new_quest->value[1] = calculated_reward / 4;                   /* Penalty for failure */
    new_quest->value[2] = MAX(10, GET_LEVEL(ch) - 5);              /* Min level */
    new_quest->value[3] = MIN(LVL_IMMORT - 1, GET_LEVEL(ch) + 20); /* Max level */
    new_quest->value[4] = -1;                                      /* No time limit */
    new_quest->value[5] = GET_MOB_VNUM(ch);                        /* Return mob */
    new_quest->value[6] = 1;                                       /* Quantity */

    /* Configura recompensas */
    new_quest->gold_reward = calculated_reward;
    new_quest->exp_reward = calculated_reward * 3; /* Quests de combate dão mais XP */
    new_quest->obj_reward = reward_item;

    /* Cria strings da quest baseadas no tipo */
    if (quest_type == AQ_PLAYER_KILL) {
        snprintf(quest_name, sizeof(quest_name), "Eliminar Assassinos");
        snprintf(quest_desc, sizeof(quest_desc), "%s busca vingança contra assassinos", GET_NAME(ch));
        snprintf(quest_info, sizeof(quest_info),
                 "%s foi atacado por assassinos e busca vingança. Elimine qualquer "
                 "assassino de jogadores para receber %d moedas de ouro e %d pontos de experiência.",
                 GET_NAME(ch), calculated_reward, calculated_reward * 3);
        snprintf(quest_done, sizeof(quest_done), "Excelente! Você eliminou um assassino. A justiça foi feita!");
    } else {
        snprintf(quest_name, sizeof(quest_name), "Caça %s", target_name);
        snprintf(quest_desc, sizeof(quest_desc), "%s oferece recompensa por %s", GET_NAME(ch), target_name);
        snprintf(quest_info, sizeof(quest_info),
                 "%s está oferecendo uma recompensa pela eliminação de %s. "
                 "Encontre e elimine este alvo para receber %d moedas de ouro e %d pontos de experiência.",
                 GET_NAME(ch), target_name, calculated_reward, calculated_reward * 3);
        snprintf(quest_done, sizeof(quest_done), "Fantástico! Você eliminou o alvo. Aqui está sua recompensa!");
    }

    new_quest->name = str_udup(quest_name);
    new_quest->desc = str_udup(quest_desc);
    new_quest->info = str_udup(quest_info);
    new_quest->done = str_udup(quest_done);
    new_quest->quit = str_udup("Compreensível. Esta era uma tarefa perigosa.");

    /* Adiciona a quest ao sistema */
    if (add_quest(new_quest) < 0) {
        log1("SYSERR: Failed to add combat quest for %s (type %d, target %d)", GET_NAME(ch), quest_type, target_vnum);
        free_quest(new_quest);
        return;
    }

    /* Check if mob can reach questmaster, if not make it a temporary questmaster */
    make_mob_temp_questmaster_if_needed(ch, new_quest_vnum);

    /* Deduz o ouro do mob */
    GET_GOLD(ch) -= calculated_reward;

    /* Aumenta quest_tendency por postar uma quest */
    if (ch->ai_data->genetics.quest_tendency < 100) {
        ch->ai_data->genetics.quest_tendency = MIN(100, ch->ai_data->genetics.quest_tendency + 1);
    }

    /* Aumenta ligeiramente a reputação por postar uma quest */
    if (ch->ai_data->reputation < 100) {
        ch->ai_data->reputation = MIN(100, ch->ai_data->reputation + 1);
    }

    /* Mensagens para jogadores */
    if (quest_type == AQ_PLAYER_KILL) {
        act("$n aparenta estar furioso e escreve algo num pergaminho.", FALSE, ch, 0, 0, TO_ROOM);
    } else {
        act("$n escreve um cartaz de procurado e o envia para o questmaster.", FALSE, ch, 0, 0, TO_ROOM);
    }

    /* Log da ação */
    log1("COMBAT QUEST: %s (room %d) created %s quest %d (target %d) with QM %d, reward %d gold", GET_NAME(ch),
         GET_ROOM_VNUM(IN_ROOM(ch)), (quest_type == AQ_PLAYER_KILL) ? "player kill" : "bounty", new_quest_vnum,
         target_vnum, questmaster_vnum, calculated_reward);

    /* Salva quest para persistência */
    if (save_quests(mob_zone)) {
        log1("COMBAT QUEST: Saved quest %d to disk for persistence", new_quest_vnum);
    } else {
        log1("SYSERR: Failed to save combat quest %d to disk", new_quest_vnum);
    }
}

/**
 * Faz um mob postar uma quest de exploração (AQ_OBJ_FIND, AQ_ROOM_FIND, AQ_MOB_FIND).
 * @param ch O mob que posta a quest
 * @param quest_type Tipo da quest (AQ_OBJ_FIND, AQ_ROOM_FIND, ou AQ_MOB_FIND)
 * @param target_vnum VNUM do alvo (obj, room, ou mob)
 * @param reward Recompensa em ouro oferecida
 */
void mob_posts_exploration_quest(struct char_data *ch, int quest_type, int target_vnum, int reward)
{
    mob_vnum questmaster_vnum;
    qst_vnum new_quest_vnum;
    struct aq_data *new_quest;
    int difficulty;
    int calculated_reward;
    zone_rnum mob_zone;
    char quest_name[MAX_QUEST_NAME];
    char quest_desc[MAX_QUEST_DESC];
    char quest_info[MAX_QUEST_MSG];
    char quest_done[MAX_QUEST_MSG];
    obj_vnum reward_item = NOTHING;
    char *target_name = "alvo desconhecido";
    obj_rnum target_obj_rnum = NOTHING;
    mob_rnum target_mob_rnum = NOBODY;
    room_rnum target_room_rnum = NOWHERE;

    if (!IS_NPC(ch) || !ch->ai_data) {
        return;
    }

    /* Check if we've reached the limit of mob-posted quests */
    if (!can_add_mob_posted_quest()) {
        return;
    }

    /* Validar tipo de quest */
    if (quest_type != AQ_OBJ_FIND && quest_type != AQ_ROOM_FIND && quest_type != AQ_MOB_FIND) {
        log1("SYSERR: Invalid exploration quest type %d from %s", quest_type, GET_NAME(ch));
        return;
    }

    /* Convert AQ_OBJ_FIND to AQ_OBJ_RETURN to require delivery instead of just finding */
    if (quest_type == AQ_OBJ_FIND) {
        quest_type = AQ_OBJ_RETURN;
    }

    /* Obter nome do alvo */
    if (quest_type == AQ_OBJ_RETURN && target_vnum != NOTHING) {
        target_obj_rnum = real_object(target_vnum);
        if (target_obj_rnum != NOTHING) {
            target_name = obj_proto[target_obj_rnum].short_description;

            /* Não posta quest para itens que não podem ser pegos */
            if (!CAN_WEAR(&obj_proto[target_obj_rnum], ITEM_WEAR_TAKE)) {
                log1("EXPLORATION QUEST: %s tried to post quest for untakeable item %d (%s)", GET_NAME(ch), target_vnum,
                     target_name);
                act("$n procura por alguém para ajudar, mas desiste.", FALSE, ch, 0, 0, TO_ROOM);
                return;
            }
        }
    } else if (quest_type == AQ_MOB_FIND && target_vnum != NOTHING) {
        target_mob_rnum = real_mobile(target_vnum);
        if (target_mob_rnum != NOBODY) {
            target_name = mob_proto[target_mob_rnum].player.short_descr;
        }
    } else if (quest_type == AQ_ROOM_FIND && target_vnum != NOTHING) {
        target_room_rnum = real_room(target_vnum);
        if (target_room_rnum != NOWHERE) {
            target_name = world[target_room_rnum].name;

            /* Não posta quest para salas GODROOM ou player houses, mas permite DEATH */
            if (ROOM_FLAGGED(target_room_rnum, ROOM_GODROOM) || ROOM_FLAGGED(target_room_rnum, ROOM_HOUSE)) {
                log1("EXPLORATION QUEST: %s tried to post quest for restricted room %d (%s)", GET_NAME(ch), target_vnum,
                     target_name);
                act("$n procura por alguém para ajudar, mas desiste.", FALSE, ch, 0, 0, TO_ROOM);
                return;
            }
        } else {
            target_name = "local específico";
        }
    }

    /* Encontra zona do mob */
    mob_zone = world[IN_ROOM(ch)].zone;

    /* Encontra questmaster */
    questmaster_vnum = find_questmaster_for_zone_enhanced(mob_zone, ch);
    if (questmaster_vnum == NOBODY) {
        questmaster_vnum = find_questmaster_for_zone(mob_zone);
    }
    if (questmaster_vnum == NOBODY) {
        log1("EXPLORATION QUEST: No questmaster found for %s in zone %d", GET_NAME(ch), mob_zone);
        act("$n procura por alguém para ajudar, mas não encontra ninguém.", FALSE, ch, 0, 0, TO_ROOM);
        return;
    }

    /* Calcula dificuldade baseada no tipo de quest */
    switch (quest_type) {
        case AQ_OBJ_RETURN:
            difficulty = 40 + rand_number(0, 30); /* Objetos são moderadamente difíceis */
            break;
        case AQ_ROOM_FIND:
            difficulty = 30 + rand_number(0, 25); /* Salas são menos difíceis */
            break;
        case AQ_MOB_FIND:
            if (target_mob_rnum != NOBODY) {
                int target_level = mob_proto[target_mob_rnum].player.level;
                difficulty = MIN(80, MAX(20, target_level * 2));
            } else {
                difficulty = 50; /* Default para alvos desconhecidos */
            }
            break;
        default:
            difficulty = 40;
            break;
    }

    /* Calcula recompensa adequada */
    calculated_reward = reward + (difficulty * 1.5);

    /* Seleciona item de recompensa do inventário do mob */
    reward_item = select_mob_inventory_reward(ch, difficulty);

    /* Verifica se o mob tem ouro suficiente */
    if (GET_GOLD(ch) < calculated_reward) {
        calculated_reward = MAX(75, GET_GOLD(ch) / 2);
    }

    /* Cria uma nova quest */
    CREATE(new_quest, struct aq_data, 1);

    /* Gera VNUM único para a nova quest */
    new_quest_vnum = zone_table[mob_zone].bot + 9000 + (time(0) % 1000);
    while (real_quest(new_quest_vnum) != NOTHING) {
        new_quest_vnum++;
        if (new_quest_vnum > zone_table[mob_zone].top + 9000) {
            new_quest_vnum = zone_table[mob_zone].bot + 9000;
        }
    }

    /* Configura a quest */
    new_quest->vnum = new_quest_vnum;
    new_quest->type = quest_type;
    new_quest->qm = questmaster_vnum;
    new_quest->target = target_vnum;
    new_quest->prereq = NOTHING;
    new_quest->flags = AQ_MOB_POSTED; /* Marca como postada por mob */
    new_quest->prev_quest = NOTHING;
    new_quest->next_quest = NOTHING;
    new_quest->func = NULL;

    /* Configura valores da quest */
    int base_questpoints = calculated_reward / 20;

    /* Bonus para quests de exploração difíceis */
    if (difficulty >= 70) {
        base_questpoints += 3;
    } else if (difficulty >= 50) {
        base_questpoints += 2;
    }

    /* Bonus para mobs aventureiros */
    if (GET_GENADVENTURER(ch) > 70) {
        base_questpoints += 2;
    } else if (GET_GENADVENTURER(ch) > 40) {
        base_questpoints += 1;
    }

    new_quest->value[0] = URANGE(1, base_questpoints, 12);         /* Questpoints reward */
    new_quest->value[1] = calculated_reward / 5;                   /* Penalty for failure */
    new_quest->value[2] = MAX(5, GET_LEVEL(ch) - 10);              /* Min level */
    new_quest->value[3] = MIN(LVL_IMMORT - 1, GET_LEVEL(ch) + 15); /* Max level */
    new_quest->value[4] = -1;                                      /* No time limit */
    new_quest->value[5] = GET_MOB_VNUM(ch);                        /* Return mob */
    new_quest->value[6] = 1;                                       /* Quantity */

    /* Configura recompensas */
    new_quest->gold_reward = calculated_reward;
    new_quest->exp_reward = calculated_reward * 2; /* Quests de exploração dão menos XP que combate */
    new_quest->obj_reward = reward_item;

    /* Cria strings da quest baseadas no tipo */
    switch (quest_type) {
        case AQ_OBJ_RETURN:
            snprintf(quest_name, sizeof(quest_name), "Buscar %s", target_name);
            snprintf(quest_desc, sizeof(quest_desc), "Encontrar e trazer %s", target_name);
            snprintf(quest_info, sizeof(quest_info),
                     "%s perdeu %s e precisa desesperadamente recuperá-lo. "
                     "Encontre e traga este item para receber %d moedas de ouro.",
                     GET_NAME(ch), target_name, calculated_reward);
            snprintf(quest_done, sizeof(quest_done), "Perfeito! Você encontrou o que eu estava procurando!");
            break;
        case AQ_ROOM_FIND:
            snprintf(quest_name, sizeof(quest_name), "Explorar local");
            snprintf(quest_desc, sizeof(quest_desc), "Explorar um local específico");
            snprintf(quest_info, sizeof(quest_info),
                     "%s precisa que alguém explore um local específico (%s). "
                     "Vá até lá para receber %d moedas de ouro.",
                     GET_NAME(ch), target_name, calculated_reward);
            snprintf(quest_done, sizeof(quest_done), "Excelente! Você chegou ao local que eu precisava explorar!");
            break;
        case AQ_MOB_FIND:
            snprintf(quest_name, sizeof(quest_name), "Encontrar %s", target_name);
            snprintf(quest_desc, sizeof(quest_desc), "Encontrar e falar com %s", target_name);
            snprintf(quest_info, sizeof(quest_info),
                     "%s está procurando por %s. Encontre esta pessoa para receber %d moedas de ouro.", GET_NAME(ch),
                     target_name, calculated_reward);
            snprintf(quest_done, sizeof(quest_done), "Maravilhoso! Você encontrou quem eu estava procurando!");
            break;
    }

    new_quest->name = str_udup(quest_name);
    new_quest->desc = str_udup(quest_desc);
    new_quest->info = str_udup(quest_info);
    new_quest->done = str_udup(quest_done);
    new_quest->quit = str_udup("Tudo bem. Talvez outro aventureiro possa ajudar.");

    /* Adiciona a quest ao sistema */
    if (add_quest(new_quest) < 0) {
        log1("SYSERR: Failed to add exploration quest for %s (type %d, target %d)", GET_NAME(ch), quest_type,
             target_vnum);
        free_quest(new_quest);
        return;
    }

    /* Check if mob can reach questmaster, if not make it a temporary questmaster */
    make_mob_temp_questmaster_if_needed(ch, new_quest_vnum);

    /* Deduz o ouro do mob */
    GET_GOLD(ch) -= calculated_reward;

    /* Aumenta quest_tendency por postar uma quest */
    if (ch->ai_data->genetics.quest_tendency < 100) {
        ch->ai_data->genetics.quest_tendency = MIN(100, ch->ai_data->genetics.quest_tendency + 1);
    }

    /* Mensagens para jogadores */
    act("$n desenha um mapa e escreve uma nota, enviando-a para o questmaster.", FALSE, ch, 0, 0, TO_ROOM);

    /* Log da ação */
    log1("EXPLORATION QUEST: %s (room %d) created %s quest %d (target %d) with QM %d, reward %d gold", GET_NAME(ch),
         GET_ROOM_VNUM(IN_ROOM(ch)),
         (quest_type == AQ_OBJ_RETURN)  ? "object return"
         : (quest_type == AQ_ROOM_FIND) ? "room find"
                                        : "mob find",
         new_quest_vnum, target_vnum, questmaster_vnum, calculated_reward);

    /* Salva quest para persistência */
    if (save_quests(mob_zone)) {
        log1("EXPLORATION QUEST: Saved quest %d to disk for persistence", new_quest_vnum);
    } else {
        log1("SYSERR: Failed to save exploration quest %d to disk", new_quest_vnum);
    }
}

/**
 * Faz um mob postar uma quest de proteção (AQ_MOB_SAVE, AQ_ROOM_CLEAR).
 * @param ch O mob que posta a quest
 * @param quest_type Tipo da quest (AQ_MOB_SAVE ou AQ_ROOM_CLEAR)
 * @param target_vnum VNUM do alvo (mob vnum para save, room vnum para clear)
 * @param reward Recompensa em ouro oferecida
 */
void mob_posts_protection_quest(struct char_data *ch, int quest_type, int target_vnum, int reward)
{
    mob_vnum questmaster_vnum;
    qst_vnum new_quest_vnum;
    struct aq_data *new_quest;
    int difficulty;
    int calculated_reward;
    zone_rnum mob_zone;
    char quest_name[MAX_QUEST_NAME];
    char quest_desc[MAX_QUEST_DESC];
    char quest_info[MAX_QUEST_MSG];
    char quest_done[MAX_QUEST_MSG];
    obj_vnum reward_item = NOTHING;
    char *target_name = "alvo desconhecido";
    mob_rnum target_mob_rnum = NOBODY;
    room_rnum target_room_rnum = NOWHERE;

    if (!IS_NPC(ch) || !ch->ai_data) {
        return;
    }

    /* Check if we've reached the limit of mob-posted quests */
    if (!can_add_mob_posted_quest()) {
        return;
    }

    /* Validar tipo de quest */
    if (quest_type != AQ_MOB_SAVE && quest_type != AQ_ROOM_CLEAR) {
        log1("SYSERR: Invalid protection quest type %d from %s", quest_type, GET_NAME(ch));
        return;
    }

    /* Obter nome do alvo */
    if (quest_type == AQ_MOB_SAVE && target_vnum != NOTHING) {
        target_mob_rnum = real_mobile(target_vnum);
        if (target_mob_rnum != NOBODY) {
            target_name = mob_proto[target_mob_rnum].player.short_descr;
        }
    } else if (quest_type == AQ_ROOM_CLEAR) {
        target_room_rnum = real_room(target_vnum);
        if (target_room_rnum != NOWHERE) {
            /* Não posta quest para salas GODROOM ou player houses, mas permite DEATH */
            if (ROOM_FLAGGED(target_room_rnum, ROOM_GODROOM) || ROOM_FLAGGED(target_room_rnum, ROOM_HOUSE)) {
                log1("PROTECTION QUEST: %s tried to post quest for restricted room %d", GET_NAME(ch), target_vnum);
                act("$n parece preocupado, mas não encontra ninguém para ajudar.", FALSE, ch, 0, 0, TO_ROOM);
                return;
            }
            target_name = world[target_room_rnum].name;
        } else {
            target_name = "área específica";
        }
    }

    /* Encontra zona do mob */
    mob_zone = world[IN_ROOM(ch)].zone;

    /* Encontra questmaster */
    questmaster_vnum = find_questmaster_for_zone_enhanced(mob_zone, ch);
    if (questmaster_vnum == NOBODY) {
        questmaster_vnum = find_questmaster_for_zone(mob_zone);
    }
    if (questmaster_vnum == NOBODY) {
        log1("PROTECTION QUEST: No questmaster found for %s in zone %d", GET_NAME(ch), mob_zone);
        act("$n parece preocupado, mas não encontra ninguém para ajudar.", FALSE, ch, 0, 0, TO_ROOM);
        return;
    }

    /* Calcula dificuldade baseada no tipo de quest */
    if (quest_type == AQ_MOB_SAVE) {
        if (target_mob_rnum != NOBODY) {
            int target_level = mob_proto[target_mob_rnum].player.level;
            difficulty = MIN(85, MAX(35, target_level * 2.5));
        } else {
            difficulty = 60; /* Default para alvos desconhecidos */
        }
    } else {                                  /* AQ_ROOM_CLEAR */
        difficulty = 65 + rand_number(0, 25); /* Room clear é naturalmente difícil */
    }

    /* Calcula recompensa adequada */
    calculated_reward = reward + (difficulty * 1.8);

    /* Seleciona item de recompensa do inventário do mob */
    reward_item = select_mob_inventory_reward(ch, difficulty);

    /* Verifica se o mob tem ouro suficiente */
    if (GET_GOLD(ch) < calculated_reward) {
        calculated_reward = MAX(80, GET_GOLD(ch) / 2);
    }

    /* Cria uma nova quest */
    CREATE(new_quest, struct aq_data, 1);

    /* Gera VNUM único para a nova quest */
    new_quest_vnum = zone_table[mob_zone].bot + 9000 + (time(0) % 1000);
    while (real_quest(new_quest_vnum) != NOTHING) {
        new_quest_vnum++;
        if (new_quest_vnum > zone_table[mob_zone].top + 9000) {
            new_quest_vnum = zone_table[mob_zone].bot + 9000;
        }
    }

    /* Configura a quest */
    new_quest->vnum = new_quest_vnum;
    new_quest->type = quest_type;
    new_quest->qm = questmaster_vnum;
    new_quest->target = target_vnum;
    new_quest->prereq = NOTHING;
    new_quest->flags = AQ_MOB_POSTED; /* Marca como postada por mob */
    new_quest->prev_quest = NOTHING;
    new_quest->next_quest = NOTHING;
    new_quest->func = NULL;

    /* Configura valores da quest */
    int base_questpoints = calculated_reward / 18;

    /* Bonus para quests de proteção difíceis */
    if (difficulty >= 75) {
        base_questpoints += 3;
    } else if (difficulty >= 55) {
        base_questpoints += 2;
    }

    /* Bonus para mobs corajosos */
    if (GET_GENBRAVE(ch) > 70) {
        base_questpoints += 2;
    } else if (GET_GENBRAVE(ch) > 40) {
        base_questpoints += 1;
    }

    new_quest->value[0] = URANGE(2, base_questpoints, 14);         /* Questpoints reward */
    new_quest->value[1] = calculated_reward / 4;                   /* Penalty for failure */
    new_quest->value[2] = MAX(8, GET_LEVEL(ch) - 8);               /* Min level */
    new_quest->value[3] = MIN(LVL_IMMORT - 1, GET_LEVEL(ch) + 18); /* Max level */
    new_quest->value[4] = -1;                                      /* No time limit */
    new_quest->value[5] = GET_MOB_VNUM(ch);                        /* Return mob */
    new_quest->value[6] = 1;                                       /* Quantity */

    /* Configura recompensas */
    new_quest->gold_reward = calculated_reward;
    new_quest->exp_reward = calculated_reward * 2.5; /* Quests de proteção dão XP moderado */
    new_quest->obj_reward = reward_item;

    /* Cria strings da quest baseadas no tipo */
    if (quest_type == AQ_MOB_SAVE) {
        snprintf(quest_name, sizeof(quest_name), "Salvar %s", target_name);
        snprintf(quest_desc, sizeof(quest_desc), "Proteger %s de perigos", target_name);
        snprintf(quest_info, sizeof(quest_info),
                 "%s está preocupado com a segurança de %s. "
                 "Ajude a garantir que esta pessoa esteja segura para receber %d moedas de ouro.",
                 GET_NAME(ch), target_name, calculated_reward);
        snprintf(quest_done, sizeof(quest_done), "Obrigado! Agora posso ficar tranquilo sabendo que estão seguros!");
    } else {
        snprintf(quest_name, sizeof(quest_name), "Limpar área");
        snprintf(quest_desc, sizeof(quest_desc), "Eliminar criaturas hostis");
        snprintf(quest_info, sizeof(quest_info),
                 "%s precisa que alguém limpe uma área específica (%s) de todas as criaturas hostis. "
                 "Elimine todos os inimigos da área para receber %d moedas de ouro.",
                 GET_NAME(ch), target_name, calculated_reward);
        snprintf(quest_done, sizeof(quest_done), "Perfeito! A área está limpa e segura agora!");
    }

    new_quest->name = str_udup(quest_name);
    new_quest->desc = str_udup(quest_desc);
    new_quest->info = str_udup(quest_info);
    new_quest->done = str_udup(quest_done);
    new_quest->quit = str_udup("Compreendo. É uma tarefa perigosa mesmo.");

    /* Adiciona a quest ao sistema */
    if (add_quest(new_quest) < 0) {
        log1("SYSERR: Failed to add protection quest for %s (type %d, target %d)", GET_NAME(ch), quest_type,
             target_vnum);
        free_quest(new_quest);
        return;
    }

    /* Check if mob can reach questmaster, if not make it a temporary questmaster */
    make_mob_temp_questmaster_if_needed(ch, new_quest_vnum);

    /* Deduz o ouro do mob */
    GET_GOLD(ch) -= calculated_reward;

    /* Aumenta quest_tendency por postar uma quest */
    if (ch->ai_data->genetics.quest_tendency < 100) {
        ch->ai_data->genetics.quest_tendency = MIN(100, ch->ai_data->genetics.quest_tendency + 1);
    }

    /* Mensagens para jogadores */
    if (quest_type == AQ_MOB_SAVE) {
        act("$n escreve uma carta urgente e a envia para o questmaster.", FALSE, ch, 0, 0, TO_ROOM);
    } else {
        act("$n desenha um mapa de uma área perigosa e envia para o questmaster.", FALSE, ch, 0, 0, TO_ROOM);
    }

    /* Log da ação */
    log1("PROTECTION QUEST: %s (room %d) created %s quest %d (target %d) with QM %d, reward %d gold", GET_NAME(ch),
         GET_ROOM_VNUM(IN_ROOM(ch)), (quest_type == AQ_MOB_SAVE) ? "mob save" : "room clear", new_quest_vnum,
         target_vnum, questmaster_vnum, calculated_reward);

    /* Salva quest para persistência */
    if (save_quests(mob_zone)) {
        log1("PROTECTION QUEST: Saved quest %d to disk for persistence", new_quest_vnum);
    } else {
        log1("SYSERR: Failed to save protection quest %d to disk", new_quest_vnum);
    }
}

/**
 * Faz um mob postar uma quest de kill geral (AQ_MOB_KILL).
 * Diferente de bounty quests, estas são para eliminação direta sem recompensa especial.
 * @param ch O mob que posta a quest
 * @param target_vnum VNUM do mob alvo
 * @param reward Recompensa em ouro oferecida
 */
void mob_posts_general_kill_quest(struct char_data *ch, int target_vnum, int reward)
{
    mob_vnum questmaster_vnum;
    qst_vnum new_quest_vnum;
    struct aq_data *new_quest;
    int difficulty;
    int calculated_reward;
    zone_rnum mob_zone;
    char quest_name[MAX_QUEST_NAME];
    char quest_desc[MAX_QUEST_DESC];
    char quest_info[MAX_QUEST_MSG];
    char quest_done[MAX_QUEST_MSG];
    obj_vnum reward_item = NOTHING;
    char *target_name = "criatura hostil";
    mob_rnum target_mob_rnum = NOBODY;

    if (!IS_NPC(ch) || !ch->ai_data) {
        return;
    }

    /* Check if we've reached the limit of mob-posted quests */
    if (!can_add_mob_posted_quest()) {
        return;
    }

    /* Obter nome do alvo */
    if (target_vnum != NOTHING) {
        target_mob_rnum = real_mobile(target_vnum);
        if (target_mob_rnum != NOBODY) {
            target_name = mob_proto[target_mob_rnum].player.short_descr;
        }
    }

    /* Encontra zona do mob */
    mob_zone = world[IN_ROOM(ch)].zone;

    /* Encontra questmaster */
    questmaster_vnum = find_questmaster_for_zone_enhanced(mob_zone, ch);
    if (questmaster_vnum == NOBODY) {
        questmaster_vnum = find_questmaster_for_zone(mob_zone);
    }
    if (questmaster_vnum == NOBODY) {
        log1("KILL QUEST: No questmaster found for %s in zone %d", GET_NAME(ch), mob_zone);
        act("$n parece agitado, mas não encontra ninguém para ajudar.", FALSE, ch, 0, 0, TO_ROOM);
        return;
    }

    /* Calcula dificuldade baseada no nível do alvo */
    if (target_mob_rnum != NOBODY) {
        int target_level = mob_proto[target_mob_rnum].player.level;
        difficulty = MIN(90, MAX(40, target_level * 3));
    } else {
        difficulty = 65; /* Default para alvos desconhecidos */
    }

    /* Calcula recompensa adequada */
    calculated_reward = reward + (difficulty * 2.2);

    /* Seleciona item de recompensa do inventário do mob */
    reward_item = select_mob_inventory_reward(ch, difficulty);

    /* Verifica se o mob tem ouro suficiente */
    if (GET_GOLD(ch) < calculated_reward) {
        calculated_reward = MAX(90, GET_GOLD(ch) / 2);
    }

    /* Cria uma nova quest */
    CREATE(new_quest, struct aq_data, 1);

    /* Gera VNUM único para a nova quest */
    new_quest_vnum = zone_table[mob_zone].bot + 9000 + (time(0) % 1000);
    while (real_quest(new_quest_vnum) != NOTHING) {
        new_quest_vnum++;
        if (new_quest_vnum > zone_table[mob_zone].top + 9000) {
            new_quest_vnum = zone_table[mob_zone].bot + 9000;
        }
    }

    /* Configura a quest */
    new_quest->vnum = new_quest_vnum;
    new_quest->type = AQ_MOB_KILL;
    new_quest->qm = questmaster_vnum;
    new_quest->target = target_vnum;
    new_quest->prereq = NOTHING;
    new_quest->flags = AQ_MOB_POSTED; /* Marca como postada por mob */
    new_quest->prev_quest = NOTHING;
    new_quest->next_quest = NOTHING;
    new_quest->func = NULL;

    /* Configura valores da quest */
    int base_questpoints = calculated_reward / 16;

    /* Bonus para quests de kill difíceis */
    if (difficulty >= 80) {
        base_questpoints += 4;
    } else if (difficulty >= 60) {
        base_questpoints += 2;
    }

    /* Bonus para mobs corajosos */
    if (GET_GENBRAVE(ch) > 70) {
        base_questpoints += 2;
    } else if (GET_GENBRAVE(ch) > 40) {
        base_questpoints += 1;
    }

    new_quest->value[0] = URANGE(2, base_questpoints, 15);         /* Questpoints reward */
    new_quest->value[1] = calculated_reward / 4;                   /* Penalty for failure */
    new_quest->value[2] = MAX(10, GET_LEVEL(ch) - 5);              /* Min level */
    new_quest->value[3] = MIN(LVL_IMMORT - 1, GET_LEVEL(ch) + 20); /* Max level */
    new_quest->value[4] = -1;                                      /* No time limit */
    new_quest->value[5] = GET_MOB_VNUM(ch);                        /* Return mob */
    new_quest->value[6] = 1;                                       /* Quantity */

    /* Configura recompensas */
    new_quest->gold_reward = calculated_reward;
    new_quest->exp_reward = calculated_reward * 3; /* Quests de kill dão mais XP */
    new_quest->obj_reward = reward_item;

    /* Cria strings da quest */
    snprintf(quest_name, sizeof(quest_name), "Eliminar %s", target_name);
    snprintf(quest_desc, sizeof(quest_desc), "%s quer eliminar %s", GET_NAME(ch), target_name);
    snprintf(quest_info, sizeof(quest_info),
             "%s está incomodado com %s e quer vê-lo eliminado. "
             "Encontre e elimine esta criatura para receber %d moedas de ouro e %d pontos de experiência.",
             GET_NAME(ch), target_name, calculated_reward, calculated_reward * 3);
    snprintf(quest_done, sizeof(quest_done), "Excelente trabalho! A ameaça foi eliminada!");

    new_quest->name = str_udup(quest_name);
    new_quest->desc = str_udup(quest_desc);
    new_quest->info = str_udup(quest_info);
    new_quest->done = str_udup(quest_done);
    new_quest->quit = str_udup("Entendo. É uma tarefa perigosa mesmo.");

    /* Adiciona a quest ao sistema */
    if (add_quest(new_quest) < 0) {
        log1("SYSERR: Failed to add kill quest for %s (target %d)", GET_NAME(ch), target_vnum);
        free_quest(new_quest);
        return;
    }

    /* Check if mob can reach questmaster, if not make it a temporary questmaster */
    make_mob_temp_questmaster_if_needed(ch, new_quest_vnum);

    /* Deduz o ouro do mob */
    GET_GOLD(ch) -= calculated_reward;

    /* Aumenta quest_tendency por postar uma quest */
    if (ch->ai_data->genetics.quest_tendency < 100) {
        ch->ai_data->genetics.quest_tendency = MIN(100, ch->ai_data->genetics.quest_tendency + 1);
    }

    /* Mensagens para jogadores */
    act("$n escreve um pedido de eliminação e o envia para o questmaster.", FALSE, ch, 0, 0, TO_ROOM);

    /* Log da ação */
    log1("KILL QUEST: %s (room %d) created kill quest %d (target %d) with QM %d, reward %d gold", GET_NAME(ch),
         GET_ROOM_VNUM(IN_ROOM(ch)), new_quest_vnum, target_vnum, questmaster_vnum, calculated_reward);

    /* Salva quest para persistência */
    if (save_quests(mob_zone)) {
        log1("KILL QUEST: Saved quest %d to disk for persistence", new_quest_vnum);
    } else {
        log1("SYSERR: Failed to save kill quest %d to disk", new_quest_vnum);
    }
}

/**
 * Faz um mob postar uma quest de escolta (AQ_MOB_ESCORT).
 * @param ch O mob que posta a quest
 * @param escort_mob_vnum VNUM do mob a ser escoltado
 * @param destination_room VNUM da sala de destino
 * @param reward Recompensa oferecida
 */
void mob_posts_escort_quest(struct char_data *ch, mob_vnum escort_mob_vnum, room_vnum destination_room, int reward)
{
    mob_vnum questmaster_vnum;
    qst_vnum new_quest_vnum;
    struct aq_data *new_quest;
    int difficulty;
    int calculated_reward;
    zone_rnum mob_zone;
    char quest_name[MAX_QUEST_NAME];
    char quest_desc[MAX_QUEST_DESC];
    char quest_info[MAX_QUEST_MSG];
    char quest_done[MAX_QUEST_MSG];
    obj_vnum reward_item = NOTHING;
    char *escort_name = "um viajante";
    char *dest_name = "um local seguro";
    mob_rnum escort_mob_rnum = NOBODY;
    room_rnum dest_room_rnum = NOWHERE;

    if (!IS_NPC(ch) || !ch->ai_data) {
        return;
    }

    /* Check if we've reached the limit of mob-posted quests */
    if (!can_add_mob_posted_quest()) {
        return;
    }

    /* Obter nome do mob a ser escoltado */
    if (escort_mob_vnum != NOTHING) {
        escort_mob_rnum = real_mobile(escort_mob_vnum);
        if (escort_mob_rnum != NOBODY) {
            escort_name = GET_NAME(&mob_proto[escort_mob_rnum]);
        }
    }

    /* Obter nome da sala de destino */
    if (destination_room != NOTHING) {
        dest_room_rnum = real_room(destination_room);
        if (dest_room_rnum != NOWHERE) {
            dest_name = world[dest_room_rnum].name;
        }
    }

    /* Encontra zona do mob */
    mob_zone = world[IN_ROOM(ch)].zone;

    /* Encontra questmaster */
    questmaster_vnum = find_questmaster_for_zone_enhanced(mob_zone, ch);
    if (questmaster_vnum == NOBODY) {
        return;
    }

    /* Cria uma nova quest */
    CREATE(new_quest, struct aq_data, 1);

    /* Gera VNUM único para a nova quest baseado na zona */
    new_quest_vnum = zone_table[mob_zone].bot + 9000 + (time(0) % 1000);
    while (real_quest(new_quest_vnum) != NOTHING) {
        new_quest_vnum++;
        if (new_quest_vnum > zone_table[mob_zone].top + 9000) {
            new_quest_vnum = zone_table[mob_zone].bot + 9000;
        }
    }

    /* Configura quest básica */
    /* Configura quest básica */
    new_quest->vnum = new_quest_vnum;
    new_quest->type = AQ_MOB_ESCORT;
    new_quest->target = escort_mob_vnum;
    new_quest->qm = questmaster_vnum;
    new_quest->flags = AQ_MOB_POSTED; /* Marca como postada por mob */

    /* Calcula dificuldade e recompensa */
    difficulty = MAX(1, GET_LEVEL(ch) / 10);
    calculated_reward = MAX(100, reward);

    /* Seleciona item de recompensa baseado na dificuldade */
    reward_item = select_mob_inventory_reward(ch, difficulty);

    /* Configura valores da quest */
    int base_questpoints = 10 + difficulty;

    /* Bonus para mobs corajosos */
    if (GET_GENBRAVE(ch) > 70) {
        base_questpoints += 2;
    } else if (GET_GENBRAVE(ch) > 40) {
        base_questpoints += 1;
    }

    new_quest->value[0] = URANGE(2, base_questpoints, 15);         /* Questpoints reward */
    new_quest->value[1] = calculated_reward / 4;                   /* Penalty for failure */
    new_quest->value[2] = MAX(10, GET_LEVEL(ch) - 5);              /* Min level */
    new_quest->value[3] = MIN(LVL_IMMORT - 1, GET_LEVEL(ch) + 20); /* Max level */
    new_quest->value[4] = -1;                                      /* No time limit */
    new_quest->value[5] = destination_room;                        /* Destination room in RETURNMOB field */
    new_quest->value[6] = 1;                                       /* Quantity */

    /* Configura recompensas */
    new_quest->gold_reward = calculated_reward;
    new_quest->exp_reward = calculated_reward * 2; /* Escort quests give moderate XP */
    new_quest->obj_reward = reward_item;

    /* Cria strings da quest */
    snprintf(quest_name, sizeof(quest_name), "Escoltar %s", escort_name);
    snprintf(quest_desc, sizeof(quest_desc), "%s precisa de escolta", escort_name);
    snprintf(
        quest_info, sizeof(quest_info),
        "%s precisa ser escoltado até %s com segurança. "
        "Aceite esta busca e guie o viajante até o destino para receber %d moedas de ouro e %d pontos de experiência.",
        escort_name, dest_name, calculated_reward, calculated_reward * 2);
    snprintf(quest_done, sizeof(quest_done), "Muito obrigado por me escoltar com segurança!");

    new_quest->name = str_udup(quest_name);
    new_quest->desc = str_udup(quest_desc);
    new_quest->info = str_udup(quest_info);
    new_quest->done = str_udup(quest_done);
    new_quest->quit = str_udup("Entendo. É uma jornada perigosa mesmo.");

    /* Adiciona a quest ao sistema */
    if (add_quest(new_quest) < 0) {
        log1("SYSERR: Failed to add escort quest for %s (escort mob %d)", GET_NAME(ch), escort_mob_vnum);
        free_quest(new_quest);
        return;
    }

    /* Check if mob can reach questmaster, if not make it a temporary questmaster */
    make_mob_temp_questmaster_if_needed(ch, new_quest_vnum);

    /* Deduz o ouro do mob */
    GET_GOLD(ch) -= calculated_reward;

    /* Aumenta quest_tendency por postar uma quest */
    if (ch->ai_data->genetics.quest_tendency < 100) {
        ch->ai_data->genetics.quest_tendency = MIN(100, ch->ai_data->genetics.quest_tendency + 1);
    }

    /* Mensagens para jogadores */
    act("$n escreve um pedido de escolta e o envia para o questmaster.", FALSE, ch, 0, 0, TO_ROOM);

    /* Log da ação */
    log1("ESCORT QUEST: %s (room %d) created escort quest %d (mob %d to room %d) with QM %d, reward %d gold",
         GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), new_quest_vnum, escort_mob_vnum, destination_room, questmaster_vnum,
         calculated_reward);

    /* Salva quest para persistência */
    if (save_quests(mob_zone)) {
        log1("ESCORT QUEST: Saved quest %d to disk for persistence", new_quest_vnum);
    } else {
        log1("SYSERR: Failed to save escort quest %d to disk", new_quest_vnum);
    }
}

/**
 * Encontra um questmaster acessível na zona especificada.
 * Procura questmasters que estão atualmente carregados no jogo e podem ser alcançados.
 * @param ch O mob que quer encontrar um questmaster
 * @param zone A zona para procurar
 * @return Ponteiro para o questmaster encontrado, ou NULL se nenhum for encontrado
 */
struct char_data *find_accessible_questmaster_in_zone(struct char_data *ch, zone_rnum zone)
{
    qst_rnum rnum;
    mob_rnum qm_mob_rnum;
    struct char_data *qm_char;
    room_rnum qm_room;

    if (zone == NOWHERE || zone >= top_of_zone_table) {
        return NULL;
    }

    /* Procura por questmasters carregados na zona especificada */
    for (rnum = 0; rnum < total_quests; rnum++) {
        if (QST_MASTER(rnum) != NOBODY) {
            qm_mob_rnum = real_mobile(QST_MASTER(rnum));
            if (qm_mob_rnum != NOBODY) {
                /* Procura este questmaster carregado no mundo */
                for (qm_char = character_list; qm_char; qm_char = qm_char->next) {
                    if (IS_NPC(qm_char) && GET_MOB_RNUM(qm_char) == qm_mob_rnum) {
                        qm_room = IN_ROOM(qm_char);
                        if (qm_room != NOWHERE && world[qm_room].zone == zone) {
                            /* Verifica se é acessível usando pathfinding */
                            if (find_first_step(IN_ROOM(ch), qm_room) != BFS_NO_PATH) {
                                return qm_char;
                            }
                        }
                    }
                }
            }
        }
    }

    return NULL;
}

/**
 * Encontra um questmaster adequado para a zona especificada.
 * Procura primeiro na própria zona, depois em zonas adjacentes.
 * @param zone A zona para a qual procurar um questmaster
 * @return VNUM do questmaster encontrado, ou NOBODY se nenhum for encontrado
 */
/**
 * Enhancement 5: Find questmaster for zone with alignment and proximity preferences
 * @param zone Zone to find questmaster for
 * @param requesting_mob Mob requesting the quest (for alignment considerations)
 * @return VNUM do questmaster mais apropriado, ou NOBODY se não encontrado
 */
mob_vnum find_questmaster_for_zone_enhanced(zone_rnum zone, struct char_data *requesting_mob)
{
    mob_rnum mob_rnum;
    mob_vnum best_qm = NOBODY;
    zone_rnum qm_zone;
    int best_score = -1;
    int requesting_alignment = 0; /* Default neutral */

    if (zone == NOWHERE || zone >= top_of_zone_table) {
        return NOBODY;
    }

    /* Get requesting mob's alignment for preference calculation */
    if (requesting_mob && IS_NPC(requesting_mob)) {
        requesting_alignment = GET_ALIGNMENT(requesting_mob);
    }

    /* Enhancement 5: Search for questmasters with preference scoring */
    for (mob_rnum = 0; mob_rnum <= top_of_mobt; mob_rnum++) {
        if (mob_index[mob_rnum].func == questmaster) {
            /* Try to get zone information from mob */
            qm_zone = NOWHERE;

            /* First check if mob has initial room assigned */
            if (mob_proto[mob_rnum].in_room != NOWHERE) {
                qm_zone = world[mob_proto[mob_rnum].in_room].zone;
            } else {
                /* Look for instance in world */
                struct char_data *qm_char;
                for (qm_char = character_list; qm_char; qm_char = qm_char->next) {
                    if (IS_NPC(qm_char) && GET_MOB_RNUM(qm_char) == mob_rnum) {
                        qm_zone = world[IN_ROOM(qm_char)].zone;
                        break;
                    }
                }
            }

            if (qm_zone != NOWHERE) {
                /* Calculate preference score for this questmaster */
                int score = 0;

                /* Prefer questmasters in the same zone (highest priority) */
                if (qm_zone == zone) {
                    score += 100;
                } else {
                    /* Enhancement 5: Proximity bonus - closer zones are preferred */
                    int zone_distance = abs((int)qm_zone - (int)zone);
                    score += MAX(0, 50 - zone_distance * 5); /* Max 50 points for proximity */
                }

                /* Enhancement 5: Alignment preference - similar alignments are preferred */
                if (requesting_mob) {
                    int qm_alignment = GET_ALIGNMENT(&mob_proto[mob_rnum]);
                    int alignment_diff = abs(requesting_alignment - qm_alignment);

                    /* Perfect alignment match gets bonus */
                    if (alignment_diff < 100) {
                        score += 20;
                    } else if (alignment_diff < 300) {
                        score += 10;
                    }
                    /* Opposite alignments get penalty */
                    else if (alignment_diff > 700) {
                        score -= 10;
                    }
                }

                /* Enhancement 5: Prefer questmasters in allied/friendly zones */
                /* Check if zones have similar characteristics */
                if (qm_zone != zone && qm_zone < top_of_zone_table && zone < top_of_zone_table) {
                    /* Zones with similar level ranges are considered "allied" */
                    int zone_level_diff = abs((int)zone_table[qm_zone].bot - (int)zone_table[zone].bot);
                    if (zone_level_diff < 50) {
                        score += 15; /* Allied zone bonus */
                    }
                }

                /* Update best questmaster if this one scores higher */
                if (score > best_score) {
                    best_score = score;
                    best_qm = mob_index[mob_rnum].vnum;
                }
            } else if (best_qm == NOBODY) {
                /* If no zone info but it's a questmaster, use as fallback */
                best_qm = mob_index[mob_rnum].vnum;
            }
        }
    }

    return best_qm;
}

mob_vnum find_questmaster_for_zone(zone_rnum zone)
{
    mob_rnum mob_rnum;
    mob_vnum best_qm = NOBODY;
    zone_rnum qm_zone;

    if (zone == NOWHERE || zone >= top_of_zone_table) {
        return NOBODY;
    }

    /* Primeiro tenta encontrar um questmaster na própria zona */
    for (mob_rnum = 0; mob_rnum <= top_of_mobt; mob_rnum++) {
        /* Verifica se o mob tem a função questmaster */
        if (mob_index[mob_rnum].func == questmaster) {
            /* Verifica se o mob tem uma sala inicial definida */
            if (mob_proto[mob_rnum].in_room != NOWHERE) {
                qm_zone = world[mob_proto[mob_rnum].in_room].zone;
                if (qm_zone == zone) {
                    return mob_index[mob_rnum].vnum;
                }
            }
            /* Se não tem sala inicial, verifica se existe uma instância no mundo */
            else {
                struct char_data *qm_char;
                for (qm_char = character_list; qm_char; qm_char = qm_char->next) {
                    if (IS_NPC(qm_char) && GET_MOB_RNUM(qm_char) == mob_rnum) {
                        qm_zone = world[IN_ROOM(qm_char)].zone;
                        if (qm_zone == zone) {
                            return mob_index[mob_rnum].vnum;
                        }
                    }
                }
            }
        }
    }

    /* Se não encontrou na zona, procura qualquer questmaster disponível */
    for (mob_rnum = 0; mob_rnum <= top_of_mobt; mob_rnum++) {
        if (mob_index[mob_rnum].func == questmaster) {
            /* Verifica se existe uma instância ativa no mundo */
            struct char_data *qm_char;
            for (qm_char = character_list; qm_char; qm_char = qm_char->next) {
                if (IS_NPC(qm_char) && GET_MOB_RNUM(qm_char) == mob_rnum) {
                    return mob_index[mob_rnum].vnum;
                }
            }
            /* Se não há instância ativa, mas tem questmaster function, usa mesmo assim */
            if (best_qm == NOBODY) {
                best_qm = mob_index[mob_rnum].vnum;
            }
        }
    }

    return best_qm;
}

/**
 * Calcula a dificuldade de uma quest baseada no item solicitado.
 * Leva em consideração o max load do item (da zona) e as estatísticas
 * do proprietário atual (se aplicável).
 * @param item_vnum VNUM do item para calcular dificuldade
 * @return Valor de dificuldade de 1 a 100
 */
/* Enhancement 2: Helper function to find current owner of an item */
struct char_data *find_item_owner(obj_vnum item_vnum)
{
    struct char_data *ch;
    struct obj_data *obj;

    /* Search through all characters in game */
    for (ch = character_list; ch; ch = ch->next) {
        /* Check carried items */
        for (obj = ch->carrying; obj; obj = obj->next_content) {
            if (GET_OBJ_VNUM(obj) == item_vnum) {
                return ch;
            }
        }

        /* Check equipped items */
        int i;
        for (i = 0; i < NUM_WEARS; i++) {
            if (GET_EQ(ch, i) && GET_OBJ_VNUM(GET_EQ(ch, i)) == item_vnum) {
                return ch;
            }
        }
    }

    return NULL; /* Item not found in anyone's possession */
}

int calculate_quest_difficulty(obj_vnum item_vnum)
{
    obj_rnum obj_rnum;
    int difficulty = 50; /* Base difficulty */
    struct char_data *current_owner;

    obj_rnum = real_object(item_vnum);
    if (obj_rnum == NOTHING) {
        return 75; /* Item não existe, dificuldade alta */
    }

    /* Base difficulty no nível do item */
    if (GET_OBJ_LEVEL(&obj_proto[obj_rnum]) > 0) {
        difficulty = MIN(GET_OBJ_LEVEL(&obj_proto[obj_rnum]), 100);
    }

    /* Aumenta dificuldade baseado no valor do item */
    if (GET_OBJ_COST(&obj_proto[obj_rnum]) > 10000) {
        difficulty += 20;
    } else if (GET_OBJ_COST(&obj_proto[obj_rnum]) > 5000) {
        difficulty += 10;
    }

    /* Aumenta dificuldade para itens raros/especiais */
    if (GET_OBJ_TYPE(&obj_proto[obj_rnum]) == ITEM_TREASURE || OBJ_FLAGGED(&obj_proto[obj_rnum], ITEM_MAGIC)) {
        difficulty += 15;
    }

    /* Enhancement 2: Dynamic difficulty based on current item owner's stats */
    current_owner = find_item_owner(item_vnum);
    if (current_owner) {
        /* Increase difficulty based on owner's level */
        difficulty += GET_LEVEL(current_owner) / 2;

        /* If owner is a player, additional considerations */
        if (!IS_NPC(current_owner)) {
            /* Higher difficulty if player has high stats */
            int avg_stat = (GET_STR(current_owner) + GET_DEX(current_owner) + GET_CON(current_owner) +
                            GET_INT(current_owner) + GET_WIS(current_owner) + GET_CHA(current_owner)) /
                           6;
            difficulty += (avg_stat - 10) / 2; /* Bonus for above-average stats */

            /* Increase difficulty if player has high karma (experienced player) */
            if (GET_KARMA(current_owner) > 500) {
                difficulty += 10;
            } else if (GET_KARMA(current_owner) > 200) {
                difficulty += 5;
            }
        } else {
            /* For NPC owners, consider their combat abilities */
            if (GET_HITROLL(current_owner) > 10 || GET_DAMROLL(current_owner) > 10) {
                difficulty += 10;
            }
        }
    }

    return URANGE(10, difficulty, 100);
}

/**
 * Enhancement 4: Calculate player reputation based on quest completion history
 * @param ch Player character
 * @return Reputation score (0-100, where higher is better)
 */
int calculate_player_reputation(struct char_data *ch)
{
    int reputation = 0; /* Base reputation */
    int completed_quests;
    int karma_bonus;

    if (!ch || IS_NPC(ch)) {
        return reputation;
    }

    /* Use stored reputation as base if it exists, otherwise calculate initial value */
    if (GET_PLAYER_REPUTATION(ch) > 0) {
        reputation = GET_PLAYER_REPUTATION(ch);
    } else {
        /* First time calculation - set initial reputation */

        /* Enhancement 4: Base reputation on number of completed quests */
        completed_quests = GET_NUM_QUESTS(ch);
        if (completed_quests > 0) {
            reputation += MIN(completed_quests * 2, 30); /* Max 30 points from completed quests */
        }

        /* Enhancement 4: Factor in karma (existing reputation system) */
        karma_bonus = GET_KARMA(ch) / 20;           /* Karma divided by 20 for reputation bonus */
        reputation += URANGE(-25, karma_bonus, 25); /* Karma can add/subtract up to 25 points */

        /* Enhancement 4: Level-based reputation (experienced players are more trusted) */
        if (GET_LEVEL(ch) >= 20) {
            reputation += (GET_LEVEL(ch) - 15) / 2; /* Higher level = better reputation */
        }

        reputation = URANGE(0, reputation, 100);

        /* Store the calculated reputation */
        ch->player_specials->saved.reputation = reputation;
    }

    return reputation;
}

/**
 * Modifies player reputation by a given amount with anti-exploit protection
 * @param ch Player character
 * @param amount Amount to modify reputation by (positive or negative)
 * @return TRUE if reputation was modified, FALSE if blocked by cooldown
 */
int modify_player_reputation(struct char_data *ch, int amount)
{
    time_t now;

    if (!ch || IS_NPC(ch)) {
        return FALSE;
    }

    /* Ensure reputation is initialized */
    if (GET_PLAYER_REPUTATION(ch) == 0) {
        calculate_player_reputation(ch);
    }

    /* Anti-exploit: Reputation gains (not losses) have a cooldown per player */
    if (amount > 0) {
        now = time(NULL);

        /* Check if trying to gain reputation too quickly (within 60 seconds) */
        if (ch->player_specials->saved.last_reputation_gain > 0 &&
            (now - ch->player_specials->saved.last_reputation_gain) < 60) {
            /* Cooldown active - no reputation gain */
            return FALSE;
        }

        /* Update last gain time */
        ch->player_specials->saved.last_reputation_gain = now;
    }

    /* Apply the change */
    ch->player_specials->saved.reputation = URANGE(0, ch->player_specials->saved.reputation + amount, 100);
    return TRUE;
}

/**
 * Apply class-based reputation modifier for actions aligned with class theme
 * @param ch Character performing the action
 * @param action_type Type of action being performed
 * @param target Optional target of the action (for context)
 * @return Reputation modifier (positive for class-appropriate actions)
 */
int get_class_reputation_modifier(struct char_data *ch, int action_type, struct char_data *target)
{
    int modifier = 0;

    if (!ch || IS_NPC(ch)) {
        return 0;
    }

    switch (action_type) {
        case CLASS_REP_COMBAT_KILL:
            /* Warriors gain bonus reputation for combat prowess */
            if (IS_WARRIOR(ch)) {
                modifier += 1; /* +1 extra for warriors killing in combat */
                if (target && GET_LEVEL(target) > GET_LEVEL(ch)) {
                    modifier += 1; /* Extra bonus for defeating stronger opponents */
                }
            }
            /* Rangers gain bonus for hunting (killing mobs in wilderness) */
            if (IS_RANGER(ch) && target && IS_NPC(target)) {
                modifier += 1; /* Rangers are known for hunting and tracking */
            }
            break;

        case CLASS_REP_HEALING:
            /* Clerics gain bonus reputation for healing (faith and helping) */
            if (IS_CLERIC(ch)) {
                modifier += 1; /* +1 extra for clerics healing */
            }
            /* Druids gain bonus for healing (nature's restoration) */
            if (IS_DRUID(ch)) {
                modifier += 1; /* +1 extra for druids healing */
            }
            break;

        case CLASS_REP_MAGIC_CAST:
            /* Magic users gain reputation for magical knowledge */
            if (IS_MAGIC_USER(ch)) {
                modifier += 1; /* Scholars advancing magical understanding */
            }
            break;

        case CLASS_REP_QUEST_COMPLETE:
            /* All classes get standard quest rewards, but some get bonuses */
            if (IS_RANGER(ch)) {
                modifier += 1; /* Rangers excel at tracking and quest completion */
            }
            if (IS_BARD(ch)) {
                modifier += 1; /* Bards spread tales of their achievements */
            }
            break;

        case CLASS_REP_SOCIAL_PERFORMANCE:
            /* Bards gain reputation through social interaction and performance */
            if (IS_BARD(ch)) {
                modifier += rand_number(1, 2); /* Bards are known for arts and music */
            }
            break;

        case CLASS_REP_NATURE_INTERACTION:
            /* Druids and Rangers gain reputation for nature-related actions */
            if (IS_DRUID(ch)) {
                modifier += 1; /* Druids connected to nature */
            }
            if (IS_RANGER(ch)) {
                modifier += 1; /* Rangers masters of flora */
            }
            break;

        case CLASS_REP_GENEROSITY:
            /* Clerics gain bonus for charity (faith-based giving) */
            if (IS_CLERIC(ch)) {
                modifier += 1; /* Faith encourages generosity */
            }
            /* Bards gain bonus for patronage of arts */
            if (IS_BARD(ch)) {
                modifier += 1; /* Supporting culture */
            }
            break;

        case CLASS_REP_SCHOLARLY:
            /* Magic users gain reputation for scholarly pursuits */
            if (IS_MAGIC_USER(ch)) {
                modifier += rand_number(1, 2); /* Advancing knowledge */
            }
            break;

        case CLASS_REP_FAITHFULNESS:
            /* Clerics gain reputation for acts of faith */
            if (IS_CLERIC(ch)) {
                modifier += rand_number(1, 2); /* Serving the Gods */
            }
            break;

        case CLASS_REP_STEALTH_ACTION:
            /* Thieves gain reputation for successful stealth actions */
            if (IS_THIEF(ch)) {
                modifier += rand_number(1, 2); /* Masters of stealth and cunning */
            }
            break;

        case CLASS_REP_POISONING:
            /* Thieves gain reputation for poisoning (assassin's craft) */
            if (IS_THIEF(ch)) {
                modifier += 1; /* Known for poison use */
            }
            break;

        case CLASS_REP_DARK_MAGIC:
            /* Evil magic users gain reputation for dark/necromantic magic */
            if (IS_MAGIC_USER(ch) && IS_EVIL(ch)) {
                modifier += rand_number(1, 2); /* Necromancers and dark wizards */
            }
            /* Evil clerics gain reputation for dark magic */
            if (IS_CLERIC(ch) && IS_EVIL(ch)) {
                modifier += 1; /* Fallen clerics using dark powers */
            }
            break;

        case CLASS_REP_HARM_SPELL:
            /* Evil clerics gain reputation for harmful divine magic */
            if (IS_CLERIC(ch) && IS_EVIL(ch)) {
                modifier += rand_number(1, 2); /* Evil priests spreading suffering */
            }
            /* Evil druids gain reputation for corrupting nature */
            if (IS_DRUID(ch) && IS_EVIL(ch)) {
                modifier += 1; /* Blighted druids */
            }
            break;
    }

    return modifier;
}

/**
 * Enhancement 4: Calculate quest reward with player reputation adjustment
 * @param requesting_mob Mob que está solicitando o item
 * @param item_vnum VNUM do item desejado
 * @param difficulty Dificuldade calculada da quest
 * @param player Player taking the quest (NULL if general quest)
 * @return Valor em ouro da recompensa
 */
int calculate_quest_reward_with_reputation(struct char_data *requesting_mob, obj_vnum item_vnum, int difficulty,
                                           struct char_data *player)
{
    int base_reward;
    int final_reward;
    int reputation_modifier = 100; /* Default 100% (no change) */

    /* Calculate base reward using existing function */
    base_reward = calculate_quest_reward(requesting_mob, item_vnum, difficulty);

    /* Enhancement 4: Adjust reward based on player reputation */
    if (player && !IS_NPC(player)) {
        int reputation = calculate_player_reputation(player);

        /* High reputation (80+) gets 20% bonus */
        if (reputation >= 80) {
            reputation_modifier = 120;
        }
        /* Good reputation (60-79) gets 10% bonus */
        else if (reputation >= 60) {
            reputation_modifier = 110;
        }
        /* Poor reputation (0-39) gets 10% penalty */
        else if (reputation < 40) {
            reputation_modifier = 90;
        }
        /* Average reputation (40-59) gets no modifier */
    }

    /* Apply reputation modifier */
    final_reward = (base_reward * reputation_modifier) / 100;

    return final_reward;
}

/**
 * Calcula a recompensa apropriada para uma quest baseada no mob solicitante,
 * item desejado e dificuldade da quest.
 * @param requesting_mob Mob que está solicitando o item
 * @param item_vnum VNUM do item desejado
 * @param difficulty Dificuldade calculada da quest
 * @return Valor em ouro da recompensa
 */
int calculate_quest_reward(struct char_data *requesting_mob, obj_vnum item_vnum, int difficulty)
{
    int base_reward = 100;
    int final_reward;
    obj_rnum obj_rnum;

    if (!requesting_mob || !IS_NPC(requesting_mob)) {
        return base_reward;
    }

    /* Base reward baseado no nível do mob solicitante */
    base_reward = GET_LEVEL(requesting_mob) * 10;

    /* Ajusta baseado na dificuldade */
    base_reward = (base_reward * difficulty) / 50;

    /* Ajusta baseado no valor do item */
    obj_rnum = real_object(item_vnum);
    if (obj_rnum != NOTHING) {
        int item_value = GET_OBJ_COST(&obj_proto[obj_rnum]);
        if (item_value > 0) {
            base_reward = MAX(base_reward, item_value / 2);
        }
    }

    /* Garante que o mob tem ouro suficiente para pagar */
    final_reward = MIN(base_reward, GET_GOLD(requesting_mob) / 2);

    /* Mínimo e máximo para evitar valores extremos */
    return URANGE(50, final_reward, 5000);
}

/**
 * Verifica se uma quest é uma wishlist quest baseado no VNUM.
 * Wishlist quests são criadas com VNUMs específicos na faixa da zona + 9000.
 * @param quest_vnum VNUM da quest a verificar
 * @return TRUE se for uma wishlist quest, FALSE caso contrário
 */
int is_wishlist_quest(qst_vnum quest_vnum)
{
    zone_rnum zone;

    /* Verifica todas as zonas para ver se o VNUM está na faixa de wishlist quests */
    for (zone = 0; zone <= top_of_zone_table; zone++) {
        if (quest_vnum >= zone_table[zone].bot + 9000 && quest_vnum <= zone_table[zone].top + 9000) {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Remove uma wishlist quest completada do sistema se ela não for repetível.
 * Isto evita acumulação de quests temporárias no sistema.
 * @param quest_vnum VNUM da quest completada a limpar
 */
void cleanup_completed_wishlist_quest(qst_vnum quest_vnum)
{
    qst_rnum rnum;

    if (!is_wishlist_quest(quest_vnum)) {
        return; /* Não é uma wishlist quest */
    }

    rnum = real_quest(quest_vnum);
    if (rnum == NOTHING) {
        return; /* Quest não existe */
    }

    /* Se a quest não é repetível, remove-a do sistema */
    if (!IS_SET(QST_FLAGS(rnum), AQ_REPEATABLE)) {
        log1("WISHLIST QUEST: Cleaning up completed non-repeatable quest %d", quest_vnum);
        delete_quest(rnum);
    }
}

/**
 * Gets the current stoneskin points for a character.
 * Returns 0 if the character doesn't have stoneskin active.
 * @param ch The character to check
 * @return Number of stoneskin points, or 0 if no stoneskin
 */
int get_stoneskin_points(struct char_data *ch)
{
    struct affected_type *af;

    if (!ch || !AFF_FLAGGED(ch, AFF_STONESKIN))
        return 0;

    for (af = ch->affected; af; af = af->next) {
        if (af->spell == SPELL_STONESKIN) {
            return af->modifier;
        }
    }

    return 0;
}

/**
 * Sets the stoneskin points for a character.
 * If points <= 0, removes the stoneskin effect entirely.
 * @param ch The character to modify
 * @param points New number of stoneskin points
 */
void set_stoneskin_points(struct char_data *ch, int points)
{
    struct affected_type *af;

    if (!ch)
        return;

    for (af = ch->affected; af; af = af->next) {
        if (af->spell == SPELL_STONESKIN) {
            if (points <= 0) {
                affect_remove(ch, af);
                return;
            }
            af->modifier = MIN(points, 168); /* Max 168 points as per help */
            return;
        }
    }
}

/**
 * Reduces stoneskin points by the specified amount.
 * If points reach 0 or below, removes the stoneskin effect.
 * @param ch The character to modify
 * @param reduction Amount to reduce points by
 * @return TRUE if stoneskin was removed, FALSE if still active
 */
bool reduce_stoneskin_points(struct char_data *ch, int reduction)
{
    struct affected_type *af;

    if (!ch || !AFF_FLAGGED(ch, AFF_STONESKIN))
        return FALSE;

    for (af = ch->affected; af; af = af->next) {
        if (af->spell == SPELL_STONESKIN) {
            int old_points = af->modifier;
            af->modifier -= reduction;

            if (af->modifier <= 0) {
                affect_remove(ch, af);
                return TRUE;
            }

            /* Adjust duration proportionally when points are consumed
             * If we had X points with Y duration, and lose some points,
             * the duration should decrease proportionally to maintain
             * the hours-per-point ratio */
            if (old_points > 0 && af->duration > 0) {
                af->duration = (af->duration * af->modifier) / old_points;
            }

            return FALSE;
        }
    }

    return FALSE;
}

/**
 * Applies stoneskin protection to incoming damage.
 * If the character has stoneskin active, it absorbs the damage and reduces points.
 * @param ch The character with potential stoneskin protection
 * @param dam Pointer to the damage value to be modified
 * @return TRUE if stoneskin absorbed the damage, FALSE otherwise
 */
bool apply_stoneskin_protection(struct char_data *ch, int *dam)
{
    if (!ch || !dam || *dam <= 0 || !AFF_FLAGGED(ch, AFF_STONESKIN))
        return FALSE;

    /* Stoneskin absorbs damage and loses 1 point */
    if (reduce_stoneskin_points(ch, 1)) {
        /* Stoneskin was removed (no more points) */
        act("A proteção de sua pele se desfaz completamente!", FALSE, ch, 0, 0, TO_CHAR);
        act("A pele dura de $n volta ao normal.", FALSE, ch, 0, 0, TO_ROOM);
    } else {
        /* Still has points left */
        act("Sua pele dura absorve o impacto!", FALSE, ch, 0, 0, TO_CHAR);
        act("A pele dura de $n absorve o golpe.", FALSE, ch, 0, 0, TO_ROOM);
    }
    *dam = 0; /* no damage when using stoneskin */
    return TRUE;
}

/**
 * Removes all occurrences of a substring from a string.
 * Modifies the original string in-place.
 * @param str The string to modify
 * @param substr The substring to remove
 */
void remove_from_string(char *str, const char *substr)
{
    char *pos, *src, *dst;
    size_t substr_len;

    if (!str || !substr || !*substr)
        return;

    substr_len = strlen(substr);
    src = dst = str;

    while ((pos = strstr(src, substr)) != NULL) {
        /* Copy everything before the match */
        while (src < pos) {
            *dst++ = *src++;
        }
        /* Skip the match */
        src += substr_len;
    }

    /* Copy the rest of the string */
    while (*src) {
        *dst++ = *src++;
    }
    *dst = '\0';
}

/**
 * Removes trailing whitespace from a string and returns a new string.
 * The caller is responsible for freeing the returned string.
 * @param str The string to trim
 * @return A new string with trailing whitespace removed
 */
char *right_trim_whitespace(const char *str)
{
    char *result;
    size_t len;
    int end;

    if (!str)
        return NULL;

    len = strlen(str);
    if (len == 0) {
        result = malloc(1);
        if (result)
            result[0] = '\0';
        return result;
    }

    /* Find the last non-whitespace character */
    end = len - 1;
    while (end >= 0 && isspace(str[end])) {
        end--;
    }

    /* Allocate memory for the trimmed string */
    result = malloc(end + 2);
    if (!result)
        return NULL;

    /* Copy the non-whitespace part */
    if (end >= 0) {
        strncpy(result, str, end + 1);
        result[end + 1] = '\0';
    } else {
        result[0] = '\0';
    }

    return result;
}

/** Function to get mob skill level based on level and skill type.
 * Mobs don't have saved skills like players, so we calculate their skill
 * based on their level to ensure they can use resource gathering commands.
 * @param ch The mob character.
 * @param skill_num The skill number to get level for.
 * @return The calculated skill level for the mob. */
int get_mob_skill(struct char_data *ch, int skill_num)
{
    if (!IS_MOB(ch))
        return 0;

    /* Base skill level equals mob level, ensuring reasonable success rates */
    int base_skill = GET_LEVEL(ch);

    /* Apply some variation based on skill type and mob genetics if available */
    if (ch->ai_data) {
        switch (skill_num) {
            case SKILL_MINE:
                /* Mining benefits from brave mobs (physical labor) */
                if (ch->ai_data->genetics.brave_prevalence > 50)
                    base_skill += 10;
                if (ch->ai_data->genetics.loot_tendency > 70)
                    base_skill += 5;
                break;
            case SKILL_FISHING:
                /* Fishing benefits from patient, wise mobs */
                if (ch->ai_data->genetics.wimpy_tendency < 30) /* Patient mobs */
                    base_skill += 10;
                if (ch->ai_data->genetics.use_tendency > 50)
                    base_skill += 5;
                break;
            case SKILL_FORAGE:
                /* Foraging benefits from wise, roaming mobs */
                if (ch->ai_data->genetics.roam_tendency > 50)
                    base_skill += 10;
                if (ch->ai_data->genetics.use_tendency > 50)
                    base_skill += 5;
                break;
            case SKILL_EAVESDROP:
                /* Eavesdropping benefits from intelligent, social mobs */
                if (ch->ai_data->genetics.trade_tendency > 50)
                    base_skill += 10;
                if (ch->ai_data->genetics.quest_tendency > 50)
                    base_skill += 5;
                break;
            default:
                /* For other skills, use base level */
                break;
        }
    }

    /* Cap skill at reasonable limits */
    return MIN(base_skill, 95);
}

/**
 * Adjust a mob's emotion by a specified amount, keeping it within 0-100 bounds
 * @param mob The mob whose emotion to adjust
 * @param emotion_ptr Pointer to the emotion value
 * @param amount Amount to adjust (positive or negative)
 */
static void adjust_emotion(struct char_data *mob, int *emotion_ptr, int amount)
{
    if (!mob || !IS_NPC(mob) || !mob->ai_data || !emotion_ptr)
        return;

    *emotion_ptr = URANGE(0, *emotion_ptr + amount, 100);
}

/**
 * Update mob emotions based on being attacked
 * @param mob The mob being attacked
 * @param attacker The character attacking the mob
 */
void update_mob_emotion_attacked(struct char_data *mob, struct char_data *attacker)
{
    if (!mob || !IS_NPC(mob) || !mob->ai_data || !CONFIG_MOB_CONTEXTUAL_SOCIALS)
        return;

    /* Being attacked increases fear and anger */
    adjust_emotion(mob, &mob->ai_data->emotion_fear, rand_number(5, 15));
    adjust_emotion(mob, &mob->ai_data->emotion_anger, rand_number(10, 20));

    /* Decreases happiness and trust */
    adjust_emotion(mob, &mob->ai_data->emotion_happiness, -rand_number(5, 15));
    adjust_emotion(mob, &mob->ai_data->emotion_trust, -rand_number(10, 20));

    /* If attacked by someone with high reputation, increase fear more */
    if (attacker && GET_REPUTATION(attacker) >= 60) {
        adjust_emotion(mob, &mob->ai_data->emotion_fear, rand_number(5, 10));
    }

    /* If mob is brave, fear increases less and courage might increase */
    if (mob->ai_data->genetics.brave_prevalence > 50) {
        adjust_emotion(mob, &mob->ai_data->emotion_fear, -rand_number(3, 8));
        adjust_emotion(mob, &mob->ai_data->emotion_courage, rand_number(5, 10));
    }
}

/**
 * Update mob emotions based on successfully attacking
 * @param mob The mob doing the attacking
 * @param victim The victim being attacked
 */
void update_mob_emotion_attacking(struct char_data *mob, struct char_data *victim)
{
    if (!mob || !IS_NPC(mob) || !mob->ai_data || !CONFIG_MOB_CONTEXTUAL_SOCIALS)
        return;

    /* Attacking increases courage and decreases fear */
    adjust_emotion(mob, &mob->ai_data->emotion_courage, rand_number(2, 5));
    adjust_emotion(mob, &mob->ai_data->emotion_fear, -rand_number(2, 5));

    /* If mob is evil, increase anger and decrease compassion */
    if (IS_EVIL(mob)) {
        adjust_emotion(mob, &mob->ai_data->emotion_anger, rand_number(3, 8));
        adjust_emotion(mob, &mob->ai_data->emotion_compassion, -rand_number(2, 5));
    }

    /* Killing good-aligned victims increases pride for evil mobs */
    if (IS_EVIL(mob) && victim && IS_GOOD(victim)) {
        adjust_emotion(mob, &mob->ai_data->emotion_pride, rand_number(5, 10));
    }
}

/**
 * Update mob emotions based on receiving healing
 * @param mob The mob being healed
 * @param healer The character healing the mob
 */
void update_mob_emotion_healed(struct char_data *mob, struct char_data *healer)
{
    if (!mob || !IS_NPC(mob) || !mob->ai_data || !CONFIG_MOB_CONTEXTUAL_SOCIALS)
        return;

    /* Being healed increases happiness, trust, and friendship */
    adjust_emotion(mob, &mob->ai_data->emotion_happiness, rand_number(10, 20));
    adjust_emotion(mob, &mob->ai_data->emotion_trust, rand_number(10, 15));
    adjust_emotion(mob, &mob->ai_data->emotion_friendship, rand_number(10, 15));

    /* Decreases fear and anger */
    adjust_emotion(mob, &mob->ai_data->emotion_fear, -rand_number(5, 10));
    adjust_emotion(mob, &mob->ai_data->emotion_anger, -rand_number(5, 10));

    /* Increases love if healer has high reputation */
    if (healer && GET_REPUTATION(healer) >= 60) {
        adjust_emotion(mob, &mob->ai_data->emotion_love, rand_number(5, 15));
    }
}

/**
 * Update mob emotions based on witnessing death of an ally
 * @param mob The mob witnessing the death
 * @param dead_ally The ally who died
 */
void update_mob_emotion_ally_died(struct char_data *mob, struct char_data *dead_ally)
{
    if (!mob || !IS_NPC(mob) || !mob->ai_data || !CONFIG_MOB_CONTEXTUAL_SOCIALS)
        return;

    /* Witnessing ally death increases sadness, fear, and anger */
    adjust_emotion(mob, &mob->ai_data->emotion_sadness, rand_number(15, 25));
    adjust_emotion(mob, &mob->ai_data->emotion_fear, rand_number(10, 20));
    adjust_emotion(mob, &mob->ai_data->emotion_anger, rand_number(10, 20));

    /* Decreases happiness */
    adjust_emotion(mob, &mob->ai_data->emotion_happiness, -rand_number(15, 25));

    /* If mob has high loyalty, sadness and anger increase more */
    if (mob->ai_data->emotion_loyalty > 60) {
        adjust_emotion(mob, &mob->ai_data->emotion_sadness, rand_number(10, 15));
        adjust_emotion(mob, &mob->ai_data->emotion_anger, rand_number(10, 15));
    }
}

/**
 * Update mob emotions based on receiving a gift/trade
 * @param mob The mob receiving the item
 * @param giver The character giving the item
 */
void update_mob_emotion_received_item(struct char_data *mob, struct char_data *giver)
{
    if (!mob || !IS_NPC(mob) || !mob->ai_data || !CONFIG_MOB_CONTEXTUAL_SOCIALS)
        return;

    /* Receiving items increases happiness, trust, and friendship */
    adjust_emotion(mob, &mob->ai_data->emotion_happiness, rand_number(10, 20));
    adjust_emotion(mob, &mob->ai_data->emotion_trust, rand_number(10, 15));
    adjust_emotion(mob, &mob->ai_data->emotion_friendship, rand_number(10, 15));

    /* If mob has high greed, happiness increases more */
    if (mob->ai_data->emotion_greed > 60) {
        adjust_emotion(mob, &mob->ai_data->emotion_happiness, rand_number(10, 20));
    }
}

/**
 * Update mob emotions over time (passive decay/stabilization)
 * Call this periodically for emotional regulation
 * @param mob The mob whose emotions to regulate
 */
void update_mob_emotion_passive(struct char_data *mob)
{
    if (!mob || !IS_NPC(mob) || !mob->ai_data || !CONFIG_MOB_CONTEXTUAL_SOCIALS)
        return;

    /* Emotions gradually return toward neutral baseline (50) or trait-based values */
    /* Extreme emotions (very high or very low) decay faster */

    /* Fear decays toward wimpy_tendency baseline */
    int fear_baseline = mob->ai_data->genetics.wimpy_tendency / 2;
    if (mob->ai_data->emotion_fear > fear_baseline) {
        adjust_emotion(mob, &mob->ai_data->emotion_fear, -rand_number(1, 3));
    } else if (mob->ai_data->emotion_fear < fear_baseline) {
        adjust_emotion(mob, &mob->ai_data->emotion_fear, rand_number(1, 2));
    }

    /* Anger decays toward alignment-based baseline */
    int anger_baseline = IS_EVIL(mob) ? 35 : (IS_GOOD(mob) ? 15 : 25);
    if (mob->ai_data->emotion_anger > anger_baseline) {
        adjust_emotion(mob, &mob->ai_data->emotion_anger, -rand_number(1, 3));
    } else if (mob->ai_data->emotion_anger < anger_baseline) {
        adjust_emotion(mob, &mob->ai_data->emotion_anger, rand_number(1, 2));
    }

    /* Happiness returns toward alignment-based baseline */
    int happiness_baseline = IS_GOOD(mob) ? 40 : (IS_EVIL(mob) ? 15 : 30);
    if (mob->ai_data->emotion_happiness > happiness_baseline) {
        adjust_emotion(mob, &mob->ai_data->emotion_happiness, -rand_number(1, 2));
    } else if (mob->ai_data->emotion_happiness < happiness_baseline) {
        adjust_emotion(mob, &mob->ai_data->emotion_happiness, rand_number(1, 3));
    }

    /* Sadness gradually decreases (unless reinforced by events) */
    if (mob->ai_data->emotion_sadness > 10) {
        adjust_emotion(mob, &mob->ai_data->emotion_sadness, -rand_number(1, 3));
    }
}

/**
 * Make a mob mourn the death of another character
 * Performs mourning socials and adjusts emotions based on relationship
 * @param mob The mob doing the mourning
 * @param deceased The character who died
 */
void mob_mourn_death(struct char_data *mob, struct char_data *deceased)
{
    const char *mourning_socials[] = {"cry", "sob", "weep", "mourn", NULL};
    const char *angry_mourning[] = {"scream", "rage", "howl", NULL};
    int social_index;
    int cmd_num;
    const char *social_to_use;
    bool is_close_relationship = FALSE;

    if (!mob || !deceased || !IS_NPC(mob) || !mob->ai_data || !CONFIG_MOB_CONTEXTUAL_SOCIALS)
        return;

    /* Don't mourn enemies or those of opposing alignment */
    if (IS_GOOD(mob) && IS_EVIL(deceased))
        return;
    if (IS_EVIL(mob) && IS_GOOD(deceased))
        return;

    /* Determine relationship strength */
    /* High friendship (60+), high love (50+), or high loyalty (60+) indicates close relationship */
    if (mob->ai_data->emotion_friendship >= 60 || mob->ai_data->emotion_love >= 50 ||
        mob->ai_data->emotion_loyalty >= 60) {
        is_close_relationship = TRUE;
    }

    /* Group members are considered close */
    if (GROUP(mob) && GROUP(deceased) && GROUP(mob) == GROUP(deceased)) {
        is_close_relationship = TRUE;
    }

    /* Update emotions for witnessing death */
    update_mob_emotion_ally_died(mob, deceased);

    /* If not a close relationship and low compassion, mob might not mourn visibly */
    if (!is_close_relationship && mob->ai_data->emotion_compassion < 30) {
        /* Chance to mourn silently (no social) */
        if (rand_number(1, 100) > 40)
            return;
    }

    /* Determine mourning behavior based on emotions */
    /* High anger mobs express grief with anger */
    if (mob->ai_data->emotion_anger >= 60) {
        social_index = rand_number(0, 2); /* Choose from angry_mourning array (3 elements: indices 0-2) */
        social_to_use = angry_mourning[social_index];
    } else {
        /* Normal mourning */
        social_index = rand_number(0, 3); /* Choose from mourning_socials array (4 elements: indices 0-3) */
        social_to_use = mourning_socials[social_index];
    }

    /* Find the social command number */
    for (cmd_num = 0; *complete_cmd_info[cmd_num].command != '\n'; cmd_num++) {
        if (!strcmp(complete_cmd_info[cmd_num].command, social_to_use))
            break;
    }

    if (*complete_cmd_info[cmd_num].command != '\n') {
        /* Perform the mourning social */
        do_action(mob, "", cmd_num, 0);

        /* Safety check: do_action can trigger DG scripts which may cause extraction */
        if (MOB_FLAGGED(mob, MOB_NOTDEADYET) || PLR_FLAGGED(mob, PLR_NOTDEADYET))
            return;

        /* Validate mob->ai_data still exists after potential extraction */
        if (!mob->ai_data)
            return;

        /* For close relationships, might say something */
        if (is_close_relationship && rand_number(1, 100) <= 50) {
            char say_buf[MAX_STRING_LENGTH];

            if (mob->ai_data->emotion_love >= 60) {
                snprintf(say_buf, sizeof(say_buf), "Não! %s era tudo para mim!", GET_NAME(deceased));
            } else if (mob->ai_data->emotion_friendship >= 70) {
                snprintf(say_buf, sizeof(say_buf), "%s era meu amigo!", GET_NAME(deceased));
            } else if (mob->ai_data->emotion_loyalty >= 70) {
                snprintf(say_buf, sizeof(say_buf), "Meu companheiro %s caiu!", GET_NAME(deceased));
            } else if (mob->ai_data->emotion_anger >= 60) {
                snprintf(say_buf, sizeof(say_buf), "Vou vingar %s!", GET_NAME(deceased));
            } else {
                snprintf(say_buf, sizeof(say_buf), "Descanse em paz, %s.", GET_NAME(deceased));
            }

            do_say(mob, say_buf, 0, 0);

            /* Safety check: do_say can trigger DG scripts */
            if (MOB_FLAGGED(mob, MOB_NOTDEADYET) || PLR_FLAGGED(mob, PLR_NOTDEADYET))
                return;

            /* Validate ai_data again */
            if (!mob->ai_data)
                return;
        }
    }

    /* For very close relationships with high love, mob might become vengeful or despondent */
    if (is_close_relationship && mob->ai_data) {
        if (mob->ai_data->emotion_love >= 70) {
            /* Extreme grief response */
            adjust_emotion(mob, &mob->ai_data->emotion_sadness, rand_number(20, 35));
            adjust_emotion(mob, &mob->ai_data->emotion_anger, rand_number(15, 30));
            adjust_emotion(mob, &mob->ai_data->emotion_happiness, -rand_number(25, 40));

            /* Mob might flee in grief if wimpy tendency is high */
            if (mob->ai_data->genetics.wimpy_tendency > 60 && rand_number(1, 100) <= 40) {
                do_flee(mob, "", 0, 0);
                /* Safety check: do_flee can trigger extraction */
                if (MOB_FLAGGED(mob, MOB_NOTDEADYET) || PLR_FLAGGED(mob, PLR_NOTDEADYET))
                    return;
            }
        } else if (mob->ai_data->emotion_friendship >= 70) {
            /* Strong friendship loss */
            adjust_emotion(mob, &mob->ai_data->emotion_sadness, rand_number(15, 25));
            adjust_emotion(mob, &mob->ai_data->emotion_anger, rand_number(10, 20));
        }
    }
}

/**
 * Update mob emotions based on receiving a social/emote from a player
 * @param mob The mob receiving the social
 * @param actor The character performing the social
 * @param social_name The name of the social command
 */
void update_mob_emotion_from_social(struct char_data *mob, struct char_data *actor, const char *social_name)
{
    /* Positive socials that increase friendship, happiness, trust */
    const char *positive_socials[] = {"bow",    "smile",     "nod",      "wave",   "applaud", "comfort", "pat",
                                      "thank",  "salute",    "hug",      "cuddle", "kiss",    "wink",    "grin",
                                      "giggle", "chuckle",   "cheer",    "praise", "admire",  "respect", "welcome",
                                      "greet",  "handshake", "highfive", NULL};

    /* Negative socials that increase anger, decrease trust/friendship */
    const char *negative_socials[] = {"frown",   "glare",    "spit",  "sneer", "accuse", "scorn",   "mock",
                                      "insult",  "taunt",    "scoff", "curse", "ignore", "dismiss", "threaten",
                                      "slap",    "punch",    "kick",  "shove", "stare",  "eye",     "jeer",
                                      "snicker", "ridicule", "scowl", NULL};

    /* Neutral/curious socials that increase curiosity */
    const char *neutral_socials[] = {"ponder", "shrug", "peer", "look", "examine", "watch", "observe", NULL};

    /* Fearful socials that the actor shows - might increase mob's courage/pride */
    const char *fearful_socials[] = {"cower", "tremble", "whimper", "beg", "plead", NULL};

    /* Blocked socials - extremely inappropriate (sexual/extreme violence) */
    const char *blocked_socials[] = {"sex",     "seduce",    "fondle", "grope", "french",
                                     "despine", "shiskabob", "vice",   NULL};

    /* Disgusting socials - trigger disgust and anger */
    const char *disgusting_socials[] = {"earlick", "licks", "pant", "moan", "moon", "booger", "drool", "puke", NULL};

    /* Violent socials - trigger pain, fear, anger */
    const char *violent_socials[] = {"needle", "shock", "whip", "spank", "vampire", "haircut", NULL};

    /* Humiliating socials - trigger shame and humiliation */
    const char *humiliating_socials[] = {"suckit-up", "wedgie", "noogie", NULL};

    int i;
    bool is_positive = FALSE;
    bool is_negative = FALSE;
    bool is_neutral = FALSE;
    bool is_fearful = FALSE;
    bool is_blocked = FALSE;
    bool is_disgusting = FALSE;
    bool is_violent = FALSE;
    bool is_humiliating = FALSE;
    int player_reputation;

    if (!mob || !actor || !IS_NPC(mob) || !mob->ai_data || !social_name || !*social_name ||
        !CONFIG_MOB_CONTEXTUAL_SOCIALS)
        return;

    player_reputation = GET_REPUTATION(actor);

    /* Check for blocked socials first - highest priority */
    for (i = 0; blocked_socials[i] != NULL; i++) {
        if (!strcmp(social_name, blocked_socials[i])) {
            is_blocked = TRUE;
            break;
        }
    }

    /* If blocked, handle with extreme response and exit */
    if (is_blocked) {
        /* Extreme anger, disgust, and fear response */
        adjust_emotion(mob, &mob->ai_data->emotion_anger, rand_number(30, 50));
        adjust_emotion(mob, &mob->ai_data->emotion_disgust, rand_number(40, 60));
        adjust_emotion(mob, &mob->ai_data->emotion_horror, rand_number(20, 40));
        adjust_emotion(mob, &mob->ai_data->emotion_trust, -rand_number(40, 60));
        adjust_emotion(mob, &mob->ai_data->emotion_friendship, -rand_number(35, 55));

        /* Mob might attack or flee based on courage */
        if (mob->ai_data->emotion_courage >= 50 && mob->ai_data->emotion_anger >= 70) {
            /* High courage - might attack the offender */
            if (rand_number(1, 100) <= 40 && !FIGHTING(mob)) {
                act("$n está extremamente ofendido e ataca!", FALSE, mob, 0, actor, TO_ROOM);
                set_fighting(mob, actor);
            }
        } else if (mob->ai_data->emotion_fear >= 40) {
            /* Low courage - flee in fear/disgust */
            adjust_emotion(mob, &mob->ai_data->emotion_fear, rand_number(20, 40));
            act("$n recua horrorizado!", FALSE, mob, 0, actor, TO_ROOM);
        }
        return; /* Exit - blocked social handled */
    }

    /* Categorize the social */
    for (i = 0; positive_socials[i] != NULL; i++) {
        if (!strcmp(social_name, positive_socials[i])) {
            is_positive = TRUE;
            break;
        }
    }

    if (!is_positive) {
        for (i = 0; disgusting_socials[i] != NULL; i++) {
            if (!strcmp(social_name, disgusting_socials[i])) {
                is_disgusting = TRUE;
                break;
            }
        }
    }

    if (!is_positive && !is_disgusting) {
        for (i = 0; violent_socials[i] != NULL; i++) {
            if (!strcmp(social_name, violent_socials[i])) {
                is_violent = TRUE;
                break;
            }
        }
    }

    if (!is_positive && !is_disgusting && !is_violent) {
        for (i = 0; humiliating_socials[i] != NULL; i++) {
            if (!strcmp(social_name, humiliating_socials[i])) {
                is_humiliating = TRUE;
                break;
            }
        }
    }

    if (!is_positive && !is_disgusting && !is_violent && !is_humiliating) {
        for (i = 0; negative_socials[i] != NULL; i++) {
            if (!strcmp(social_name, negative_socials[i])) {
                is_negative = TRUE;
                break;
            }
        }
    }

    if (!is_positive && !is_disgusting && !is_violent && !is_humiliating && !is_negative) {
        for (i = 0; neutral_socials[i] != NULL; i++) {
            if (!strcmp(social_name, neutral_socials[i])) {
                is_neutral = TRUE;
                break;
            }
        }
    }

    if (!is_positive && !is_disgusting && !is_violent && !is_humiliating && !is_negative && !is_neutral) {
        for (i = 0; fearful_socials[i] != NULL; i++) {
            if (!strcmp(social_name, fearful_socials[i])) {
                is_fearful = TRUE;
                break;
            }
        }
    }

    /* Apply emotion changes based on social type */
    if (is_disgusting) {
        /* Disgusting socials trigger disgust, anger, decrease trust/friendship */
        adjust_emotion(mob, &mob->ai_data->emotion_disgust, rand_number(15, 30));
        adjust_emotion(mob, &mob->ai_data->emotion_anger, rand_number(10, 25));
        adjust_emotion(mob, &mob->ai_data->emotion_trust, -rand_number(15, 30));
        adjust_emotion(mob, &mob->ai_data->emotion_friendship, -rand_number(10, 20));

        /* Decrease happiness */
        adjust_emotion(mob, &mob->ai_data->emotion_happiness, -rand_number(10, 20));

        /* Mob might respond with disgust */
        if (mob->ai_data->emotion_disgust >= 50 && rand_number(1, 100) <= 40) {
            act("$n olha para você com nojo.", FALSE, mob, 0, actor, TO_VICT);
            act("$n olha para $N com nojo.", FALSE, mob, 0, actor, TO_NOTVICT);
        }
    } else if (is_violent) {
        /* Violent socials trigger pain, fear, anger */
        adjust_emotion(mob, &mob->ai_data->emotion_pain, rand_number(20, 40));
        adjust_emotion(mob, &mob->ai_data->emotion_anger, rand_number(15, 35));
        adjust_emotion(mob, &mob->ai_data->emotion_fear, rand_number(10, 25));
        adjust_emotion(mob, &mob->ai_data->emotion_trust, -rand_number(20, 35));
        adjust_emotion(mob, &mob->ai_data->emotion_friendship, -rand_number(15, 30));

        /* Wimpy mobs react with more fear */
        if (mob->ai_data->genetics.wimpy_tendency > 50) {
            adjust_emotion(mob, &mob->ai_data->emotion_fear, rand_number(10, 20));
        }

        /* Brave mobs react with more anger */
        if (mob->ai_data->genetics.brave_prevalence > 60) {
            adjust_emotion(mob, &mob->ai_data->emotion_anger, rand_number(10, 20));
            adjust_emotion(mob, &mob->ai_data->emotion_courage, rand_number(5, 10));
        }

        /* Mob might respond aggressively if angry enough */
        if (mob->ai_data->emotion_anger >= 60 && mob->ai_data->emotion_courage >= 40 && rand_number(1, 100) <= 30) {
            act("$n rosna ameaçadoramente para você!", FALSE, mob, 0, actor, TO_VICT);
            act("$n rosna ameaçadoramente para $N!", FALSE, mob, 0, actor, TO_NOTVICT);
        }
    } else if (is_humiliating) {
        /* Humiliating socials trigger shame, humiliation, anger */
        adjust_emotion(mob, &mob->ai_data->emotion_humiliation, rand_number(15, 30));
        adjust_emotion(mob, &mob->ai_data->emotion_shame, rand_number(15, 25));
        adjust_emotion(mob, &mob->ai_data->emotion_anger, rand_number(10, 20));
        adjust_emotion(mob, &mob->ai_data->emotion_trust, -rand_number(20, 35));
        adjust_emotion(mob, &mob->ai_data->emotion_friendship, -rand_number(15, 25));

        /* Decrease pride and self-esteem */
        adjust_emotion(mob, &mob->ai_data->emotion_pride, -rand_number(10, 20));

        /* High pride mobs react with more anger to humiliation */
        if (mob->ai_data->emotion_pride >= 60) {
            adjust_emotion(mob, &mob->ai_data->emotion_anger, rand_number(15, 25));
        }
    } else if (is_positive) {
        /* Positive socials increase happiness, friendship, trust */
        adjust_emotion(mob, &mob->ai_data->emotion_happiness, rand_number(5, 15));
        adjust_emotion(mob, &mob->ai_data->emotion_friendship, rand_number(5, 12));
        adjust_emotion(mob, &mob->ai_data->emotion_trust, rand_number(3, 10));

        /* Decrease anger and fear */
        adjust_emotion(mob, &mob->ai_data->emotion_anger, -rand_number(3, 8));
        adjust_emotion(mob, &mob->ai_data->emotion_fear, -rand_number(2, 6));

        /* High reputation actors have stronger positive impact */
        if (player_reputation >= 60) {
            adjust_emotion(mob, &mob->ai_data->emotion_friendship, rand_number(5, 10));
            adjust_emotion(mob, &mob->ai_data->emotion_trust, rand_number(5, 10));
            /* Might increase love for very high reputation */
            if (player_reputation >= 80) {
                adjust_emotion(mob, &mob->ai_data->emotion_love, rand_number(3, 8));
            }
        }

        /* Compassionate mobs respond more to kindness */
        if (mob->ai_data->emotion_compassion >= 60) {
            adjust_emotion(mob, &mob->ai_data->emotion_happiness, rand_number(5, 10));
        }
    } else if (is_negative) {
        /* Negative socials increase anger, decrease trust/friendship */
        adjust_emotion(mob, &mob->ai_data->emotion_anger, rand_number(8, 20));
        adjust_emotion(mob, &mob->ai_data->emotion_trust, -rand_number(10, 20));
        adjust_emotion(mob, &mob->ai_data->emotion_friendship, -rand_number(8, 15));

        /* Decrease happiness */
        adjust_emotion(mob, &mob->ai_data->emotion_happiness, -rand_number(5, 15));

        /* May increase fear if mob is wimpy */
        if (mob->ai_data->genetics.wimpy_tendency > 50) {
            adjust_emotion(mob, &mob->ai_data->emotion_fear, rand_number(5, 15));
        }

        /* May increase pride/courage if mob is brave */
        if (mob->ai_data->genetics.brave_prevalence > 60) {
            adjust_emotion(mob, &mob->ai_data->emotion_pride, rand_number(5, 10));
            adjust_emotion(mob, &mob->ai_data->emotion_courage, rand_number(3, 8));
        }

        /* Low reputation actors cause more anger */
        if (player_reputation < 30) {
            adjust_emotion(mob, &mob->ai_data->emotion_anger, rand_number(5, 10));
        }

        /* Might trigger envy if social implies superiority */
        if (!strcmp(social_name, "mock") || !strcmp(social_name, "taunt") || !strcmp(social_name, "scoff")) {
            adjust_emotion(mob, &mob->ai_data->emotion_envy, rand_number(5, 15));
        }
    } else if (is_neutral) {
        /* Neutral socials slightly increase curiosity */
        adjust_emotion(mob, &mob->ai_data->emotion_curiosity, rand_number(3, 8));

        /* Might slightly increase friendship if mob is already friendly */
        if (mob->ai_data->emotion_friendship >= 50) {
            adjust_emotion(mob, &mob->ai_data->emotion_friendship, rand_number(2, 5));
        }
    } else if (is_fearful) {
        /* Actor showing fear increases mob's courage and pride */
        adjust_emotion(mob, &mob->ai_data->emotion_courage, rand_number(5, 15));
        adjust_emotion(mob, &mob->ai_data->emotion_pride, rand_number(5, 12));

        /* Decreases mob's own fear */
        adjust_emotion(mob, &mob->ai_data->emotion_fear, -rand_number(5, 12));

        /* If mob is evil and actor is begging, might increase mob's sadistic pleasure */
        if (IS_EVIL(mob) && (!strcmp(social_name, "beg") || !strcmp(social_name, "plead"))) {
            adjust_emotion(mob, &mob->ai_data->emotion_happiness, rand_number(5, 15));
        }
    }

    /* Context-dependent socials - massage and rose */
    /* Massage: positive if trusted, disgusting if not */
    if (!strcmp(social_name, "massage")) {
        if (mob->ai_data->emotion_trust >= 50 && mob->ai_data->emotion_friendship >= 40) {
            /* Trusted source - relaxing */
            adjust_emotion(mob, &mob->ai_data->emotion_happiness, rand_number(10, 20));
            adjust_emotion(mob, &mob->ai_data->emotion_trust, rand_number(5, 10));
        } else {
            /* Untrusted source - uncomfortable/disgusting */
            adjust_emotion(mob, &mob->ai_data->emotion_disgust, rand_number(10, 20));
            adjust_emotion(mob, &mob->ai_data->emotion_anger, rand_number(5, 15));
            adjust_emotion(mob, &mob->ai_data->emotion_trust, -rand_number(10, 20));
        }
    }

    /* Rose: romantic gesture - positive if receptive */
    if (!strcmp(social_name, "rose")) {
        if (mob->ai_data->emotion_friendship >= 60 || mob->ai_data->emotion_love >= 40) {
            /* Receptive - romantic */
            adjust_emotion(mob, &mob->ai_data->emotion_love, rand_number(5, 15));
            adjust_emotion(mob, &mob->ai_data->emotion_happiness, rand_number(5, 10));
            adjust_emotion(mob, &mob->ai_data->emotion_friendship, rand_number(5, 10));
        } else if (mob->ai_data->emotion_trust < 30) {
            /* Not receptive/suspicious */
            adjust_emotion(mob, &mob->ai_data->emotion_curiosity, rand_number(5, 10));
        }
    }

    /* Alignment interaction - opposite alignments intensify negative responses */
    if (is_negative) {
        if ((IS_GOOD(mob) && IS_EVIL(actor)) || (IS_EVIL(mob) && IS_GOOD(actor))) {
            adjust_emotion(mob, &mob->ai_data->emotion_anger, rand_number(5, 10));
            adjust_emotion(mob, &mob->ai_data->emotion_trust, -rand_number(5, 10));
        }
    }

    /* Same alignment intensifies positive responses */
    if (is_positive) {
        if ((IS_GOOD(mob) && IS_GOOD(actor)) || (IS_EVIL(mob) && IS_EVIL(actor))) {
            adjust_emotion(mob, &mob->ai_data->emotion_friendship, rand_number(3, 8));
            adjust_emotion(mob, &mob->ai_data->emotion_loyalty, rand_number(2, 6));
        }
    }

    /* Mob might respond with a social back if emotions are strong enough */
    if (rand_number(1, 100) <= 30) { /* 30% chance - CONFIG check already done at function entry */
        /* Reciprocate positive socials if happy/friendly */
        if (is_positive && mob->ai_data->emotion_happiness >= 50 && mob->ai_data->emotion_friendship >= 40) {
            /* Mob might smile or nod back */
            if (rand_number(1, 100) <= 50) {
                act("$n sorri para você.", FALSE, mob, 0, actor, TO_VICT);
                /* Safety check: act() can trigger DG scripts which may cause extraction */
                if (MOB_FLAGGED(mob, MOB_NOTDEADYET) || PLR_FLAGGED(mob, PLR_NOTDEADYET))
                    return;
                if (!actor || MOB_FLAGGED(actor, MOB_NOTDEADYET) || PLR_FLAGGED(actor, PLR_NOTDEADYET))
                    return;

                act("$n sorri para $N.", FALSE, mob, 0, actor, TO_NOTVICT);
                /* Safety check again after second act() */
                if (MOB_FLAGGED(mob, MOB_NOTDEADYET) || PLR_FLAGGED(mob, PLR_NOTDEADYET))
                    return;
                if (!actor || MOB_FLAGGED(actor, MOB_NOTDEADYET) || PLR_FLAGGED(actor, PLR_NOTDEADYET))
                    return;
            }
        }
        /* Respond to negative socials with anger if mob is brave/angry */
        else if (is_negative && mob->ai_data->emotion_anger >= 60 && mob->ai_data->emotion_courage >= 40) {
            /* Mob might glare or growl back */
            if (rand_number(1, 100) <= 50) {
                act("$n olha furiosamente para você!", FALSE, mob, 0, actor, TO_VICT);
                /* Safety check: act() can trigger DG scripts which may cause extraction */
                if (MOB_FLAGGED(mob, MOB_NOTDEADYET) || PLR_FLAGGED(mob, PLR_NOTDEADYET))
                    return;
                if (!actor || MOB_FLAGGED(actor, MOB_NOTDEADYET) || PLR_FLAGGED(actor, PLR_NOTDEADYET))
                    return;

                act("$n olha furiosamente para $N!", FALSE, mob, 0, actor, TO_NOTVICT);
                /* Safety check again after second act() */
                if (MOB_FLAGGED(mob, MOB_NOTDEADYET) || PLR_FLAGGED(mob, PLR_NOTDEADYET))
                    return;
                if (!actor || MOB_FLAGGED(actor, MOB_NOTDEADYET) || PLR_FLAGGED(actor, PLR_NOTDEADYET))
                    return;
            }
        }
    }
}
