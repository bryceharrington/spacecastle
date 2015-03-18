/* spacecastle - A vector graphics space shooter game
 *
 * Copyright © 2015 Bryce Harrington
 *
 * Spacecastle is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Spacecastle is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Spacecastle.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __SCORE_H__
#define __SCORE_H__

#include <stdio.h>

class Score {
 public:
  Score();
  ~Score();

  bool record(int amt, int level, const char *who);
  bool write(FILE *fp);
  bool read(FILE *fp);
  int  amount() const { return _amount; }
  const char *to_string();
  bool from_string(const char *str);

 private:
  int         _amount;
  int         level;
  struct tm  *time;
  char        initials[4];
  char       *_str_rep;
};

class HighScores {
public:
  HighScores();
  ~HighScores();

  static const int MAX_SCORES = 100;

  int insert(const Score& s);
  bool load(const char *scores_path);
  bool save(const char *scores_path);
  const Score &get(int pos);

private:
  int   num_scores;
  Score scores[HighScores::MAX_SCORES];
};

#endif // __SCORE_H__

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-basic-offset:2
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=2:tabstop=8:softtabstop=2:fileencoding=utf-8:textwidth=99 :
