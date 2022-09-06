#include <iostream>
#include <vector>
#include <regex>
#include <random>
#include <cassert>

using std::vector, std::pair, std::string, std::cin, std::cout, std::regex, std::bitset;

const unsigned int POPULATION_SIZE {100};
const unsigned int NUM_GENERATIONS {1000};
const int LENGTH_PENALTY {-5};
const int BIT_FLIP_PROB {5};

// TODO Deal with non-latin characters
// TODO Make use of +,*,?
// TODO Encourage shorter everythings
// TODO Initialize population
// TODO Implement Crossover

struct {
    std::mt19937 gen{std::random_device()()};
    std::uniform_int_distribution<> distribution{0, 999};
    bool chance (int probability) {
        return distribution(gen) < probability;
    }
    int range(int min, int max) {
        assert(min < max);
        std::uniform_int_distribution<> alt_distribution{min, max};
        return alt_distribution(gen);
    }
} rng;

struct RegEx {
    static vector<pair<string,int>> words;

    struct char_group {
        bitset<26> accepted;
        int min, max;

        void mutate() {
            for (int i = 0; i < 25; ++i)
                if (rng.chance(BIT_FLIP_PROB))
                    accepted[i] = !accepted[i];
            // TODO Make not ugly
            min += rng.chance(500) ? 1 : -1;
            max += rng.chance(500) ? 1 : -1;
            min = std::max(min, 0);
            max = std::max(max, min);
        }
    };

    void mutate() {
        for (auto& group : groups)
            group.mutate();
        // TODO Potentially add a new group
    }

    RegEx operator %(const RegEx& other) {}

    int fitness() const {
        int fitness = this->length();
        for (const auto& [word, reward] : words)
            if (regex_match(word, this->to_regex()))
                fitness += reward;
        return fitness;
    }

    int length() const {
        return static_cast<int>(this->to_string().size());
    }

    string to_string() const {
        std::stringstream out;
        for (const auto& [accepted, min, max] : groups) { // TODO also try with ^, use a-z notation
            out << '[';
            for (int i = 0; i < 25; ++i)
                if (accepted[i])
                    out << std::to_string('a' + i);
            out << ']';
            out << '{' << min << ',' << max << '}';
        }
        return out.str();
    }

    regex to_regex() const {
        return regex{this->to_string(), std::regex_constants::optimize};
    }

    vector<char_group> groups;
};

void init_words() {
    int word_count;
    cin >> word_count;
    for (int i = 0; i < word_count; ++i) {
        string word;
        int weight;
        cin >> word >> weight;
        RegEx::words.emplace_back(word, weight);
    }
}

void next_generation(vector<RegEx>& population) {

}

int main() {
    init_words();
    vector<RegEx> population(POPULATION_SIZE);
}