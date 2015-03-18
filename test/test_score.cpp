#include "score.h"

#include <assert.h>

void
test_score_init()
{
    Score score;

    assert( score.amount() == 0.0 );
}

void
test_score_record()
{
    Score score;

    assert( score.record(1, 1, "a") );
    assert( score.amount() == 1 );
}

void
test_highscore_init()
{
    HighScores high_scores;
    Score score;

    score = high_scores.get(0);

    assert( score.amount() == 0.0 );
}

int
main() {
    test_score_init();
    test_highscore_init();

    return 0;
}
