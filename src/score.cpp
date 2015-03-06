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
    level = 0;
    initials[0] = '\0';
    time = NULL;
}

bool
Score::record(int amt, int lvl, const char *who) {
    _amount = amt;
    level = lvl;
    strncpy(initials, who, 4);
    time = localtime(NULL);
    return true;
}

bool
Score::write(FILE *fp) {
    char date_str[80];
    time_t t;
    struct tm *tmp;

    t = mktime(NULL);
    tmp = localtime(&t);
    if (tmp == NULL) {
        perror("localtime");
        return false;
    }
    if (strftime(date_str, sizeof(date_str), "", tmp) == 0) {
        fprintf(stderr, "strftime could not format time\n");
        return false;
    }
    fprintf(fp, "%s %s %d %d\n", date_str, initials, level, _amount);
    return true;
}

bool
Score::read(FILE *fp) {
    int n;
    char *date_str;
    char *initials_str;
    char *amount_str;

    n = fscanf(fp, "%ms %4ms %d %d\n", &date_str, &initials_str, &level, &_amount);
    // TODO: handle initials_str
    if (n == 4) {
        // TODO: Process date_str
        free(date_str);
    } else if (errno != 0) {
        perror("scanf");
        return false;
    } else {
        fprintf(stderr, "Scores file could not be parsed\n");
        return false;
    }
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
