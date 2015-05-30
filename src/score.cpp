#include "score.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

/* ----------------------------------------------------------------------
 * Score
 * ---------------------------------------------------------------------- */
Score::Score(void)
{
    _amount = 0;
    _level = 0;
    _initials[0] = '\0';
    _str_rep = NULL;
}

Score::~Score(void)
{
    printf("~Score()\n");
    if (_str_rep)
        delete _str_rep;
}

bool
Score::record(int amt, int lvl, const char *who) {
    if (amt < 0 || lvl < 0)
        return false;
    int j = 0;
    for (int i=0; i < strlen(who); i++) {
        char c = who[i];
        /* Ignore non-printable characters */
        if (c == ' ')
            c = '_';
        else if (c < 33 || c > 126)
            continue;
        _initials[j++] = c;
        if (j+1 >= MAX_INITIALS_STR)
            break;
    }
    _initials[j] = '\0';
    time_t now = time(NULL);
    if (!localtime_r(&now, &_timestamp)) {
        perror("localtime_r");
        return false;
    }
    _amount = amt;
    _level = lvl;

    return true;
}

const char*
Score::to_string(void) {
    // TODO: Better name?
    char date_str[80];

    if (_amount == 0) {
        return NULL;
    } else if (_str_rep == NULL) {
        _str_rep = (char *)malloc(256);
        if (strftime(date_str, sizeof(date_str), "%F.%T", &_timestamp) == 0) {
            fprintf(stderr, "strftime could not format time\n");
            return NULL;
        }
        snprintf(_str_rep, 256, "%s %s %d %d\n", date_str, _initials, _level, _amount);
    }
    return _str_rep;
}

bool
Score::from_string(const char *str) {
    int n, i, j;
    char *date_str;
    char *initials_str;
    char *amount_str;

    // printf("%s\n", str);
    n = sscanf(str, "%ms %ms %d %d", &date_str, &initials_str, &_level, &_amount);
    if (n == 4) {
        if ( strptime(date_str, "%F.%T", &_timestamp) == NULL) {
	    free(date_str);
	    return false;
	}
        free(date_str);

	j = 0;
        for (i=0; i<strlen(initials_str); i++) {
	    _initials[j++] = initials_str[i];
	    if (j+1 >= MAX_INITIALS_STR)
		break;
	}
    } else if (errno != 0) {
        perror("scanf");
        return false;
    } else {
        // fprintf(stderr, "Score could not be parsed: %s\n", str);
        return false;
    }
    return true;
}

// TODO: Do I even need this?
bool
Score::write(FILE *fp) {
    const char *s = to_string();
    if (!s)
        return false;
    fprintf(fp, "%s", to_string());
    return true;
}

bool
Score::read(FILE *fp) {
    int n;
    char *line = NULL;
    size_t len = 0;

    n = getline(&line, &len, fp);
    if (n > 0)
        from_string(line);
    free(line);

    return true;
}

/* ----------------------------------------------------------------------
 * High Scores
 * ---------------------------------------------------------------------- */
HighScores::HighScores(void)
{
}

HighScores::~HighScores()
{
}

/**
 * Inserts score in numerical score with highest score first.
 * Shuffles scores downward.
 *
 * Returns distance from the top
 */
int
HighScores::insert(const Score& new_score)
{
    int dist = -1;
    Score a, b;
    bool inserted = false;
    for (int i=0; i<HighScores::MAX_SCORES; i++) {
        if (new_score.amount() > scores[i].amount()) {
            if (!inserted) {
                a = scores[i];
                scores[i] = new_score;
                dist = i;
                inserted = true;
            } else {
                b = scores[i];
                scores[i] = a;
                a = b;

                // Check if there's no more scores and return early
                if (b.amount() <= 0)
                    break;
            }
        }
    }
    return dist;
}

bool
HighScores::load(const char *scores_path)
{
    int i = 0;

    FILE *fp = fopen(scores_path, "r");
    if (!fp)
        return false;

    while (fp && (i<HighScores::MAX_SCORES))
        scores[i++].read(fp);

    fclose(fp);
    return true;
}

bool
HighScores::save(const char *scores_path)
{
    FILE *fp = fopen(scores_path, "w");
    for (int i=0; i<num_scores; i++) {
        scores[i].write(fp);
    }
    fclose(fp);
}

const Score&
HighScores::get(int pos)
{
    if (pos > num_scores)
        pos = num_scores - 1;
    return scores[pos];
}
