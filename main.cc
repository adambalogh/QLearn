#include <array>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <map>
#include <string>
#include <vector>
#include <unistd.h>

#include <ncurses.h>

typedef enum { UP, DOWN, LEFT, RIGHT, NUM_DIRECTIONS } Direction;

class QLearn {
 public:
  typedef std::pair<std::string, Direction> KeyType;

  QLearn(double epsilon, double alpha, double gamma)
      : epsilon_(100), alpha_(alpha), gamma_(gamma) {
    std::srand(std::time(0));
  }

  double GetQ(std::string state, Direction action) {
    return q[std::make_pair(state, action)];
  }

  double Learn(std::string state, Direction action, double reward,
               std::string state2) {
    auto key = std::make_pair(state, action);
    double maxqnew = 0;
    for (int i = 0; i < NUM_DIRECTIONS; i++) {
      auto v = GetQ(state2, static_cast<Direction>(i));
      maxqnew = std::max(v, maxqnew);
    }
    q[key] = q[key] + 0.3 * (reward + 0.8 * maxqnew - q[key]);
    return 0;
  }

  Direction ChooseAction(std::string state) {
    auto rand = std::rand() % 1000;
    if (rand <= epsilon_) {
      auto choice = std::rand() % 4;
      return static_cast<Direction>(choice);
    }

    std::vector<double> q(4);
    for (int i = 0; i < 4; i++) {
      q[i] = GetQ(state, static_cast<Direction>(i));
    }
    auto max_i = 0;
    for (int i = 0; i < 4; i++) {
      if (q[i] > q[max_i]) {
        max_i = i;
      }
    }

    std::vector<int> indexes;
    for (int i = 0; i < 4; i++) {
      if (q[i] == q[max_i]) {
        indexes.push_back(i);
      }
    }
    auto i = std::rand() % indexes.size();
    return static_cast<Direction>(indexes[i]);
  }

 private:
  std::map<KeyType, double> q;
  double epsilon_;
  double alpha_;
  double gamma_;
};

struct Player {
  int x;
  int y;
  Player(int x, int y) : x(x), y(y) {}
};

struct Food {
  int x;
  int y;
  Food(int x, int y) : x(x), y(y) {}
};

class Board {
 public:
  static const int height = 10;
  static const int width = 10;

  Board() : player_(Player{0, 0}), food_(MakeFood()) { UpdateCells(); }

  bool Move(Direction d) {
    int new_x = player_.x;
    int new_y = player_.y;
    switch (d) {
      case UP:
        new_x--;
        break;
      case DOWN:
        new_x++;
        break;
      case LEFT:
        new_y--;
        break;
      case RIGHT:
        new_y++;
        break;
    }
    if (new_x < 0 || new_y < 0 || new_x >= height || new_y >= width) {
      score_ -= 100;
      return false;
    }
    player_.x = new_x;
    player_.y = new_y;

    if (player_.x == food_.x && player_.y == food_.y) {
      score_ += 100;
      food_ = MakeFood();
    }
    score_ -= 3;

    UpdateCells();
    return true;
  }

  const auto& cells() const { return cells_; }
  int score() const { return score_; }

  void ResetCells() {
    std::array<int, width> row;
    row.fill(0);
    cells_.fill(row);
  }

  void UpdateCells() {
    ResetCells();
    cells_[player_.x][player_.y] = 1;
    cells_[food_.x][food_.y] = 2;
  }

  Food MakeFood() {
    int x;
    int y;
    do {
      x = std::rand() % height;
      y = std::rand() % width;
    } while (x == player_.x && y == food_.y);
    return Food{x, y};
  };

  std::array<std::array<int, width>, height> cells_;
  Player player_;
  Food food_;
  int score_{0};
};

void DrawBoard(const Board& board) {
  attron(COLOR_PAIR(1));
  const auto& cells = board.cells();

  for (int j = 0; j < cells.at(0).size(); ++j) {
    mvwprintw(stdscr, 0, 1 * j, "---");
  }
  for (int i = 0; i < cells.size(); ++i) {
    mvwprintw(stdscr, 1 + i, 0, "|");
    for (int j = 0; j < cells.at(0).size(); ++j) {
      if (cells.at(i).at(j) == 2) {
        mvwprintw(stdscr, 1 + i, 1 + 1 * j, "X");
      } else if (cells.at(i).at(j) == 1) {
        mvwprintw(stdscr, 1 + i, 1 + 1 * j, "O");
      } else {
        mvwprintw(stdscr, 1 + i, 1 + 1 * j, " ");
      }
      mvwprintw(stdscr, 1 + i, 1 * cells.at(0).size() + 1, "|");
    }
  }
  for (int j = 0; j < cells.at(0).size(); ++j) {
    mvwprintw(stdscr, 1 + cells.size(), 1 * j, "---");
  }

  mvwprintw(stdscr, 2 + cells.size(), 0, std::to_string(board.score()).c_str());
  wrefresh(stdscr);

  attroff(COLOR_PAIR(1));
}

void Draw(const Board& b) {
  auto c = b.cells();
  for (auto r : c) {
    for (auto i : r) {
      std::cout << i << " ";
    }
    std::cout << std::endl;
  }
  std::cout << "================" << std::endl;
}

std::string GetState(Player p, Food f) {
  if (p.x == f.x) {
    if (p.y > f.y) {
      return "LEFT";
    } else {
      return "RIGHT";
    }
  }
  if (p.x > f.x) {
    if (p.y == f.y) {
      return "UP";
    }
    if (p.y > f.y) {
      return "UPLEFT";
    } else {
      return "UPRIGHT";
    }
  } else {
    if (p.y == f.y) {
      return "DOWN";
    }
    if (p.y > f.y) {
      return "DOWNLEFT";
    } else {
      return "DOWNRIGHT";
    }
  }
}

int main() {
  initscr();
  clear();
  keypad(stdscr, TRUE);

  std::srand(std::time(0));

  Board b;
  QLearn q{0.05, 0.1, 0.9};

  int previous_score = 0;
  std::string previous_state;
  bool stop = false;
  std::string state;
  while (true) {
    previous_state = GetState(b.player_, b.food_);
    auto d = q.ChooseAction(previous_state);
    b.Move(d);
    state = GetState(b.player_, b.food_);
    auto reward = b.score() - previous_score;
    if (stop || previous_score < 100000) {
      q.Learn(previous_state, d, reward, state);
      stop = true;
    }
    previous_score = b.score();
    DrawBoard(b);
    usleep(50000);
  }
}
