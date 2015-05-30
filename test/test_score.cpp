#include "score.h"

#include <assert.h>
#include <string.h>

void
test_score_init()
{
    Score score;

    assert( score.amount() == 0.0 );
    assert( score.to_string() == NULL );
}

void
test_score_record()
{
    Score score;

    assert( score.record(1, 1, "a") );
    assert( score.amount() == 1 );
    assert( score.to_string() != NULL );
    assert( strlen(score.to_string()) > 0 );
    //printf( "%s\n", score.to_string() );
}

void
test_string_conversion_basic()
{
    Score score;

    // Blank scores can't be stringified
    assert( score.to_string() == NULL );

    // Basic string
    score.record(1, 1, "x");
    assert( score.to_string()[20] == 'x' );
    assert( score.to_string()[22] == '1' );
    assert( score.to_string()[24] == '1' );

    // Once a score has been recorded, it can't be changed
    score.record(2, 2, "y");
    assert( score.to_string()[20] == 'x' );
    assert( score.to_string()[22] == '1' );
    assert( score.to_string()[24] == '1' );
}

void
test_string_conversion_typical()
{
    Score score;
    // Typical string
    score.record(1000, 10, "abc");
    //printf( "%s\n", score.to_string() );
    assert( score.to_string()[20] == 'a' );
    assert( score.to_string()[24] == '1' );
    assert( score.to_string()[27] == '1' );
}

void
test_string_conversion_advanced()
{
    Score score;
    // Advanced string
    score.record(1000000, 1000, "Zoë");
    //printf( "%c\n", score.to_string()[35] );
    assert( score.to_string()[20] == 'Z' );
    assert( score.to_string()[23] == '1' );
    assert( score.to_string()[28] == '1' );
    assert( score.to_string()[34] == '0' );
    assert( strlen(score.to_string()) == 36);
}

void
test_string_conversion_truncate()
{
    Score score;

    // Advanced string
    score.record(1, 1, "2 long 2 show");
    //printf( "%s\n", score.to_string() );
    //printf( "%c\n", score.to_string()[23] );
    assert( score.to_string()[20] == '2' );
    assert( score.to_string()[21] == '_' );
    assert( score.to_string()[22] == 'l' );
    assert( score.to_string()[24] == '1' );
}

void
test_parse_valid_strings()
{
    Score score;
    const char *s = "2015-04-22.20:47:35 2_l 1 1";

    assert( score.from_string(s) );
    assert( score.to_string() != NULL );
    // printf( "-> %s\n", score.to_string() );
}

void
test_parse_invalid_strings()
{
    Score score;
    const char *empty_string = "";
    const char *invalid_date = "2015-13-33.22:47:35 2_l 1 1";
    const char *invalid_time = "2015-11-23.25:77:75 2_l 1 1";
    const char *invalid_format = "2015-May-02 10:00:00 123 x x";

    assert( ! score.from_string(empty_string) );
    assert( ! score.from_string(invalid_date) );
    assert( ! score.from_string(invalid_time) );
    assert( ! score.from_string(invalid_format) );
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
    test_score_record();
    test_string_conversion_basic();
    test_string_conversion_typical();
    test_string_conversion_advanced();
    test_string_conversion_truncate();
    test_parse_valid_strings();
    test_parse_invalid_strings();

    return 0;
}
