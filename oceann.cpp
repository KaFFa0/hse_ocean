#include <iostream>
#include <vector>
#include <memory>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <thread>

class Ocean;
class Object;
class Action;
class Prey;
class Predator;
class Stone;
class Reef;
class ApexPredator;

using Cell = Object*;

struct Neighbourhood {
    std::vector<Cell> cells;
    int radius;
};

class Object {
public:
    char sym;

    virtual ~Object() = default;
    virtual Action* tick(const Neighbourhood& neighbourhood, Ocean& ocean) = 0;

    char getSym() const {
        return sym;
    }
};

class Action {
public:
    Action(Object* obj) : obj(obj) {}
    virtual ~Action() = default;
    bool operator()(Ocean& ocean) { return apply(ocean); }

protected:
    Cell obj;

private:
    virtual bool apply(Ocean& ocean) const = 0;
};

class NoAction : public Action {
public:
    NoAction(Object* obj) : Action(obj) {}
    bool apply(Ocean& ocean) const override { return true; }
};

class Move : public Action {
public:
    enum class Direction { N, S, W, E };
    Move(Object* obj, int count, Direction direction) : Action(obj), count(count), direction(direction) {}
    bool apply(Ocean& ocean) const override;

private:
    int count;
    Direction direction;
};

class Stone : public Object {
public:
    int age;
    int age_to_reef = 8;

    Stone() {
        age = 0;
        sym = '#';
    }

    Action* tick(const Neighbourhood& neighbourhood, Ocean& ocean) override;
};

class Reef : public Object {
public:
    int age;
    int age_to_stone = 4;

    Reef() { sym = 'R'; }

    
    Action* tick(const Neighbourhood& neighbourhood, Ocean& ocean) override;
};

class Prey : public Object {
public:
    unsigned int age;
    unsigned int mature_age = 2;
    unsigned int max_age = 7;
    int speed = 1;
    bool adult_prey_nearby;
    bool isdead;
    bool reef_nearby;

    Prey() {
        age = 0;
        adult_prey_nearby = false;
        reef_nearby = false;
        isdead = false;
        sym = 'F';
    }

    void die() {
        isdead = true;
    }

    void move(Ocean& ocean, Move::Direction direction);

    Action* tick(const Neighbourhood& neighbourhood, Ocean& ocean) override;
};

class Predator : public Object {
public:
    unsigned int age;
    unsigned int mature_age = 5;
    unsigned int max_age = 15;
    int speed = 1;
    int hunger = 0;
    bool adult_predator_nearby;
    bool isdead;

    Predator() {
        age = 0;
        adult_predator_nearby = false;
        isdead = false;
        sym = 'P';
    }

    void die() {
        isdead = true;
    }

    void move(Ocean& ocean, Move::Direction direction);

    Action* tick(const Neighbourhood& neighbourhood, Ocean& ocean) override;
};

class ApexPredator : public Object {
public:
    unsigned int age;
    unsigned int mature_age = 10;
    unsigned int max_age = 18;
    int speed = 1;
    int hunger = 0;
    bool adult_ap_nearby;
    bool isdead;

    ApexPredator() {
        age = 0;
        adult_ap_nearby = false;
        isdead = false;
        sym = 'A';
    }

    void die() {
        isdead = true;
    }

    void move(Ocean& ocean, Move::Direction direction);

    Action* tick(const Neighbourhood& neighbourhood, Ocean& ocean) override;
};

class Ocean {
public:
    Ocean(int rows, int cols) : rows(rows), cols(cols), data(rows * cols) {
        std::srand(std::time(nullptr));
        for (int i = 0; i < data.size(); ++i) {
            int randomChance = std::rand() % 100;

            if (randomChance < 50) {
                data[i] = nullptr;
            } else if (randomChance < 75) {
                data[i] = new Prey();
            } else if (randomChance < 90) {
                data[i] = new Predator();
            } else if (randomChance < 95) {
                data[i] = new Stone();
            } else if (randomChance < 98) {
                data[i] = new Reef();
            } 
            else {
                data[i] = new ApexPredator();
            }
        }
    }

    void tick() {
        for (auto& cell : data) {
            if (cell) {
                auto action = cell->tick(generate_neighbourhood(cell), *this);
                if (action) {
                    (*action)(*this);
                    delete action;
                }
            }
        }

        for (int i = 0; i < data.size(); ++i) {
            if (data[i] && data[i]->getSym() == 'F' && static_cast<Prey*>(data[i])->isdead) {
                delete data[i];
                data[i] = nullptr;
            }
            if (data[i] && data[i]->getSym() == 'P' && static_cast<Predator*>(data[i])->isdead) {
                delete data[i];
                data[i] = nullptr;
            }
            if (data[i] && data[i]->getSym() == 'A' && static_cast<ApexPredator*>(data[i])->isdead) {
                delete data[i];
                data[i] = nullptr;
            }
        }
    }

    Neighbourhood generate_neighbourhood(Cell cell) {
        Neighbourhood neighbourhood;
        neighbourhood.cells.resize(4, nullptr);
        int index = 0;
        for (int i = 0; i < data.size(); ++i) {
            if (data[i] == cell) {
                index = i;
                break;
            }
        }

        int row = index / cols;
        int col = index % cols;

        neighbourhood.cells[0] = data[((row - 1 + rows) % rows) * cols + col]; // North
        neighbourhood.cells[1] = data[((row + 1) % rows) * cols + col];        // South
        neighbourhood.cells[2] = data[row * cols + (col - 1 + cols) % cols];   // West
        neighbourhood.cells[3] = data[row * cols + (col + 1) % cols];          // East

        return neighbourhood;
    }

    void display() {
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                if (data[r * cols + c]) {
                    std::cout << data[r * cols + c]->getSym();
                } else {
                    std::cout << ".";
                }
            }
            std::cout << std::endl;
        }
    }

    int rows, cols;
    std::vector<Cell> data;
};

bool Move::apply(Ocean& ocean) const {
    int index = 0;
    for (int i = 0; i < ocean.data.size(); ++i) {
        if (ocean.data[i] == obj) {
            index = i;
            break;
        }
    }

    int row = index / ocean.cols;
    int col = index % ocean.cols;
    int new_row = row;
    int new_col = col;

    switch (direction) {
        case Direction::N: new_row = (row - count + ocean.rows) % ocean.rows; break;
        case Direction::S: new_row = (row + count + ocean.rows) % ocean.rows; break;
        case Direction::W: new_col = (col - count + ocean.cols) % ocean.cols; break;
        case Direction::E: new_col = (col + count + ocean.cols) % ocean.cols; break;
    }

    int new_index = new_row * ocean.cols + new_col;
    if (ocean.data[new_index] == nullptr) {
        ocean.data[new_index] = obj;
        ocean.data[index] = nullptr;
    }

    return true;
}

void Prey::move(Ocean& ocean, Move::Direction direction) {
    Move* move = new Move(this, speed, direction);
    move->apply(ocean);
    delete move;
}

void Predator::move(Ocean& ocean, Move::Direction direction) {
    Move* move = new Move(this, speed, direction);
    move->apply(ocean);
    delete move;
}

void ApexPredator::move(Ocean& ocean, Move::Direction direction) {
    Move* move = new Move(this, speed, direction);
    move->apply(ocean);
    delete move;
}

Action* Stone::tick(const Neighbourhood& neighbourhood,Ocean& ocean) {
        age++;
        if (age >= age_to_reef) {
            int index = 0;
            for (int j = 0; j < ocean.data.size(); ++j) {
                if (ocean.data[j] == this) {
                    index = j;
                    break;
                }
            }

            ocean.data[index] = nullptr;
            ocean.data[index] = new Reef;
            return new NoAction(this);

        }
        return new NoAction(this);
}

Action* Reef::tick(const Neighbourhood& neighbourhood,Ocean& ocean) {
        age++;
        if (age >= age_to_stone) {
            int index = 0;
            for (int j = 0; j < ocean.data.size(); ++j) {
                if (ocean.data[j] == this) {
                    index = j;
                    break;
                }
            }

            ocean.data[index] = nullptr;
            ocean.data[index] = new Stone;
            return new NoAction(this);

        }
        return new NoAction(this);
}

Action* Prey::tick(const Neighbourhood& neighbourhood, Ocean& ocean) {
    age++;
    if (age > max_age) {
        die();
        return new NoAction(this);
    }

    bool reef_nearby = false;
    bool predator_nearby = false;
    Move::Direction move_direction = Move::Direction::N;

    for (int i = 0; i < neighbourhood.cells.size(); ++i) {
        if (neighbourhood.cells[i] != nullptr && (neighbourhood.cells[i]->getSym() == 'P' || neighbourhood.cells[i]->getSym() == 'A')) {
            predator_nearby = true;
            switch (i) {
                case 0: move_direction = Move::Direction::S; break;
                case 1: move_direction = Move::Direction::N; break;
                case 2: move_direction = Move::Direction::E; break;
                case 3: move_direction = Move::Direction::W; break;
            }
            break;
        }
    }

    for (int i = 0; i < neighbourhood.cells.size(); ++i) {
        if (neighbourhood.cells[i] != nullptr && (neighbourhood.cells[i]->getSym() == 'R')) {
            reef_nearby = true;
        }
    }

    if (predator_nearby) {
        return new Move(this, speed, move_direction);
    }

    if (age >= mature_age && !predator_nearby) {
        for (auto cell : neighbourhood.cells) {
            if (cell != nullptr && cell->getSym() == 'F') {
                Prey* other_prey = static_cast<Prey*>(cell);
                if (other_prey->age >= mature_age) {
                    for (int i = 0; i < neighbourhood.cells.size(); ++i) {
                        if (neighbourhood.cells[i] == nullptr) {
                            int index = 0;
                            for (int j = 0; j < ocean.data.size(); ++j) {
                                if (ocean.data[j] == this) {
                                    index = j;
                                    break;
                                }
                            }

                            int new_index = (i == 0) ? ((index / ocean.cols) - 1 + ocean.rows) % ocean.rows * ocean.cols + (index % ocean.cols) :
                                         (i == 1) ? ((index / ocean.cols) + 1 + ocean.rows) % ocean.rows * ocean.cols + (index % ocean.cols) :
                                         (i == 2) ? (index / ocean.cols) * ocean.cols + ((index % ocean.cols) - 1 + ocean.cols) % ocean.cols :
                                                    (index / ocean.cols) * ocean.cols + ((index % ocean.cols) + 1) % ocean.cols;
                            ocean.data[new_index] = new Prey();
                            break;
                        }
                    }
                }
                break;
            }
        }
    }

    int randomChance = std::rand() % 4;
    switch (randomChance) {
        case 0: return new Move(this, speed, Move::Direction::N);
        case 1: return new Move(this, speed, Move::Direction::E);
        case 2: return new Move(this, speed, Move::Direction::S);
        case 3: return new Move(this, speed, Move::Direction::W);
    }

    return new NoAction(this);
}

Action* Predator::tick(const Neighbourhood& neighbourhood, Ocean& ocean) {
    age++;
    if (age > max_age || hunger > 6) {
        die();
        return new NoAction(this);
    }

    for (auto cell : neighbourhood.cells) {
        if (cell != nullptr && cell->getSym() == 'F') {
            Prey* prey = static_cast<Prey*>(cell);
            if (!prey->isdead && !prey->reef_nearby) {
                prey->die();
                hunger = 0; 
                speed = 1;
                return new NoAction(this);
            }
        }
    }

    if (age >= mature_age && hunger <= 5) {
        for (auto cell : neighbourhood.cells) {
            if (cell != nullptr && cell->getSym() == 'P') {
                Predator* other_predator = static_cast<Predator*>(cell);
                if (other_predator->age >= mature_age && other_predator->hunger <= 5) {
                    for (int i = 0; i < neighbourhood.cells.size(); ++i) {
                        if (neighbourhood.cells[i] == nullptr) {
                            int index = 0;
                            for (int j = 0; j < ocean.data.size(); ++j) {
                                if (ocean.data[j] == this) {
                                    index = j;
                                    break;
                                }
                            }

                            int new_index = (i == 0) ? ((index / ocean.cols) - 1 + ocean.rows) % ocean.rows * ocean.cols + (index % ocean.cols) :
                                         (i == 1) ? ((index / ocean.cols) + 1 + ocean.rows) % ocean.rows * ocean.cols + (index % ocean.cols) :
                                         (i == 2) ? (index / ocean.cols) * ocean.cols + ((index % ocean.cols) - 1 + ocean.cols) % ocean.cols :
                                                    (index / ocean.cols) * ocean.cols + ((index % ocean.cols) + 1) % ocean.cols;
                            ocean.data[new_index] = new Predator();
                            break;
                        }
                    }
                }
                break;
            }
        }
    }

    if (hunger == 2) {
        speed++;
    }

    hunger++;
    int randomChance = std::rand() % 4;
    switch (randomChance) {
        case 0: return new Move(this, speed, Move::Direction::N);
        case 1: return new Move(this, speed, Move::Direction::E);
        case 2: return new Move(this, speed, Move::Direction::S);
        case 3: return new Move(this, speed, Move::Direction::W);
    }

    return new NoAction(this);
}

Action* ApexPredator::tick(const Neighbourhood& neighbourhood, Ocean& ocean) {
    age++;
    if (age > max_age || hunger >= 7) {
        die();
        return new NoAction(this);
    }

    for (auto cell : neighbourhood.cells) {
        if (hunger >= 5) {
            if (cell != nullptr && cell->getSym() == 'F') {
                Prey* prey = static_cast<Prey*>(cell);
                if (!prey->isdead && !prey->reef_nearby) {
                    prey->die();
                    hunger = 0; 
                    speed = 1;
                    return new NoAction(this);
                }
            }
            if (cell != nullptr && cell->getSym() == 'P') {
                Predator* pred = static_cast<Predator*>(cell);
                if (!pred->isdead) {
                    pred->die();
                    hunger = 0; 
                    speed = 1;
                    return new NoAction(this);
                }
            }
        } else {
            if (cell != nullptr && cell->getSym() == 'F') {
                Prey* prey = static_cast<Prey*>(cell);
                if (!prey->isdead && !prey->reef_nearby) {
                    prey->die();
                    hunger = 0; 
                    speed = 1;
                    return new NoAction(this);
                }
            }
        }
    }

    if (age >= mature_age && hunger <= 3) {
        for (auto cell : neighbourhood.cells) {
            if (cell != nullptr && cell->getSym() == 'A') {
                ApexPredator* other_ap = static_cast<ApexPredator*>(cell);
                if (other_ap->age >= mature_age && other_ap->hunger <= 3) {
                    for (int i = 0; i < neighbourhood.cells.size(); ++i) {
                        if (neighbourhood.cells[i] == nullptr) {
                            int index = 0;
                            for (int j = 0; j < ocean.data.size(); ++j) {
                                if (ocean.data[j] == this) {
                                    index = j;
                                    break;
                                }
                            }

                            int new_index = (i == 0) ? ((index / ocean.cols) - 1 + ocean.rows) % ocean.rows * ocean.cols + (index % ocean.cols) :
                                         (i == 1) ? ((index / ocean.cols) + 1 + ocean.rows) % ocean.rows * ocean.cols + (index % ocean.cols) :
                                         (i == 2) ? (index / ocean.cols) * ocean.cols + ((index % ocean.cols) - 1 + ocean.cols) % ocean.cols :
                                                    (index / ocean.cols) * ocean.cols + ((index % ocean.cols) + 1) % ocean.cols;
                            ocean.data[new_index] = new ApexPredator();
                            break;
                        }
                    }
                }
                break;
            }
        }
    }

    if (hunger == 2) {
        speed++;
    }
    if (hunger == 5) {
        speed++;
    }

    hunger++;
    int randomChance = std::rand() % 4;
    switch (randomChance) {
        case 0: return new Move(this, speed, Move::Direction::N);
        case 1: return new Move(this, speed, Move::Direction::E);
        case 2: return new Move(this, speed, Move::Direction::S);
        case 3: return new Move(this, speed, Move::Direction::W);
    }

    return new NoAction(this);
}

int main() {
    int rows, cols;
    std::cin >> rows >> cols;

    Ocean ocean(rows, cols);

    int k = 0;

    while (true) {
        system("clear");
        ocean.display();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        ocean.tick();
    }

    return 0;
}
