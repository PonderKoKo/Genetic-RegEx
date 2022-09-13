#include <iostream>
#include <vector>
#include <regex>
#include <random>
#include <cassert>

using std::vector, std::pair, std::string;

const unsigned int POPULATION_SIZE {50};
const unsigned int NUM_GENERATIONS {1000};
const int BIT_FLIP_PROB {100};
const int ADD_GROUP_PROB {100};
const int REM_GROUP_PROB {100};
const int LIMIT_FLIP_PROB {50};
const int DEFAULT_NUM_GROUPS {5};

string BEST_KNOWN_REGEX;
int BEST_KNOWN_LENGTH;
int MAXIMUM_SCORE;

vector<pair<string,int>> words;

// TODO Deal with non-latin characters
// TODO Encourage shorter regular expressions

struct {
    std::minstd_rand gen{std::random_device()()};
    std::uniform_int_distribution<> distribution{0, 999};
    bool chance (int probability) {
        return distribution(gen) < probability;
    }
    int range(int min, int max) {
        assert(min < max);
        std::uniform_int_distribution<> alt_distribution{min, max-1};
        return alt_distribution(gen);
    }
} rng;

class RegEx {
    class char_group {
        std::bitset<26> accepted {(1 << 26) - 1};
        int min {1}, max {10};
        bool nolimit {false};
        // char_group(bitset<26> a, int mi, int ma) : accepted(a), min(mi), max(ma) {}
    public:
        // char_group() : accepted{static_cast<unsigned long long>(rng.range(0, 1 << 26))} { }

        void mutate() {
            if (rng.chance(BIT_FLIP_PROB))
                accepted.flip(rng.range(0, 26));
            if (rng.chance(LIMIT_FLIP_PROB))
                nolimit = !nolimit;
            // TODO Make not ugly, allowing staying the same
            min += rng.chance(500) ? 1 : -1;
            max += rng.chance(500) ? 1 : -1;
            min = std::max(min, 0);
            max = std::max(max, min);
        }

        friend std::ostream& operator<< (std::ostream& stream, const char_group& g) {
            g.printCharacterClass(stream);
            g.printQuantifier(stream);
            return stream;
        }
    private:
        void printCharacterClass(std::ostream& stream) const {
            // TODO Add support for ranges of letters
            // TODO Fix bug where single character classes still somehow print []
            if (accepted.all())
                stream << '.';
            else {
                if (accepted.count() > 1)
                    stream << '[';
                bool inclusion = accepted.count() <= 13;
                if (!inclusion)
                    stream << '^';
                for (int i = 0; i < 25; ++i)
                    if (accepted[i] == inclusion)
                        stream << static_cast<char>('a' + i);
                if (accepted.count() > 1)
                    stream << ']';
            }
        }

        void printQuantifier(std::ostream& stream) const {

            if (nolimit) {
                if (min == 0)
                    stream << '*';
                else if (min == 1)
                    stream << '+';
                else
                    stream << '{' << min << ",}";
            } else if (min == max) {
                if (min != 1)
                    stream << '{' << min << '}';
            } else if (min == 0 && max == 1) {
                stream << '?';
            } else {
                stream << '{' << min << ',' << max << '}';
            }
        }
    };

    [[nodiscard]] int matching_score() const {
        int fitness = 0;
        for (const auto& [word, reward] : words) {
            try {
                if (regex_match(word, this->to_regex()))
                    fitness += reward;
            } catch (std::regex_error& e){
                std::cerr << this->to_string() << std::endl;
                throw e;
            }
        }
        return fitness;
    }

    [[nodiscard]] int length() const {
        return static_cast<int>(this->to_string().size());
    }

    vector<char_group> groups; // const in spirit, but ... copy-assignment
    explicit RegEx(vector<char_group>& g) : groups{g}, matching_score_(matching_score()), length_(length()) { }
public:
    int matching_score_, length_;
    RegEx() : groups(DEFAULT_NUM_GROUPS), matching_score_(matching_score()), length_(length()) {} //TODO Fix ugly repetition

    [[nodiscard]] RegEx mutate() const {
        vector<char_group> mutated(groups);
        for (auto& group : mutated)
            group.mutate();
        if (mutated.size() > 1 && rng.chance(REM_GROUP_PROB))
            mutated.erase(mutated.begin() + rng.range(0, static_cast<int>(mutated.size())));
        if (rng.chance(ADD_GROUP_PROB))
            mutated.emplace_back();
        return RegEx{mutated};
    }

    RegEx operator %(const RegEx& other) const {
        int split = rng.range(1, 1 + static_cast<int>(std::min(this->groups.size(), other.groups.size())));
        vector<char_group> crossed;
        crossed.reserve(other.groups.size());
        for (int i = 0; i < split; i++)
            crossed.push_back(this->groups[i]);
        for (int i = split; i < static_cast<int>(other.groups.size()); i++)
            crossed.push_back(other.groups[i]);
        return RegEx{crossed};
    }

    bool operator <(const RegEx& other) const {
        return std::make_pair(this->matching_score_, -this->length_) < std::make_pair(other.matching_score_, -other.length_);
    }

    [[nodiscard]] string to_string() const {
        std::stringstream out;
        out << *this;
        return out.str();
    }

    friend std::ostream& operator<< (std::ostream& stream, const RegEx& r) {
        for (const auto& group : r.groups)
            stream << group;
        return stream;
    }

    [[nodiscard]] std::regex to_regex() const {
        return std::regex{this->to_string(), std::regex_constants::optimize};
    }
};

using Population = std::array<RegEx, POPULATION_SIZE>;

void next_generation(Population& pop, RegEx& best_achieved) {
    std::sort(pop.begin(), pop.end());
    best_achieved = std::max(best_achieved, pop.back());
    for (int weak = 0, strong = POPULATION_SIZE - 1; weak < strong; weak++, strong--)
        pop[weak] = pop[strong] % pop[strong - 1];
    for (auto& ind : pop)
        ind = ind.mutate();
    pop[0] = best_achieved; // Hacky way to keep the best regex as is.
}

void read_testcase() {
    int word_count;
    std::cin >> word_count;
    MAXIMUM_SCORE = 0;
    for (int i = 0; i < word_count; ++i) {
        string word;
        int weight;
        std::cin >> word >> weight;
        words.emplace_back(word, weight);
        if (weight > 0)
            MAXIMUM_SCORE += weight;
    }
    std::cin >> BEST_KNOWN_REGEX >> BEST_KNOWN_LENGTH;
}

void interactive_test(std::istream& in, std::ostream& out, const std::regex& r) {
    string query;
    do {
        in >> query;
        out << std::regex_match(query, r);
    } while (!query.empty());
}

int main() {

    read_testcase();
    Population pop;
    RegEx best_achieved;
    for (unsigned int i = 0; i < NUM_GENERATIONS; i++)
        next_generation(pop, best_achieved);
    std::cout << best_achieved << " # " << best_achieved.matching_score_ << '/' << MAXIMUM_SCORE << " | " << best_achieved.length_ << '/' << BEST_KNOWN_LENGTH << std::endl;
    interactive_test(std::cin, std::cout, best_achieved.to_regex());
}
