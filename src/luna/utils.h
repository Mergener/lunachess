#ifndef LUNA_UTILS_H
#define LUNA_UTILS_H

#include <string>
#include <sstream>
#include <functional>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <type_traits>

#include "types.h"

namespace lunachess::utils {

template <typename T>
std::string toString(const T& val) {
    std::stringstream stream;
    stream << val;
    return stream.str();
}

i64 random(i64 minInclusive, i64 maxExclusive);
ui64 random(ui64 minInclusive, ui64 maxExclusive);
i32 random(i32 minInclusive, i32 maxExclusive);
ui32 random(ui32 minInclusive, ui32 maxExclusive);
float random(float min, float max);
double random(double min, double max);

inline float randomFloat01() { return random(0.0f, 1.0f); }
inline double randomDouble01() { return random(0.0, 1.0); }

inline bool randomBool() {
    return random(0, 2);
}

inline bool randomChance(int chancePct) {
    return random(1, 101) <= chancePct;
}

inline bool randomChance(float chancePct) {
    float rnd = random(1.0f, 100.0f);

    return rnd <= chancePct;
}

inline void writeToFile(std::filesystem::path path, const std::string& data) {
    std::ofstream stream(path);
    stream.exceptions(std::ofstream::badbit | std::ofstream::failbit);
    stream << data;
}

inline std::string readFromFile(std::filesystem::path path) {
    std::ifstream stream(path);
    stream.exceptions(std::ifstream::badbit);
    std::stringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

template <typename T> int sign(T val) {
    return (T(0) < val) - (val < T(0));
}

std::string randomUUID();

template <typename TIter, typename TCompar>
void insertionSort(TIter begin, TIter end, TCompar comp) {
    if (begin >= end) {
        return;
    }

    for (TIter i = begin + 1; i != end; ++i) {
        typename std::iterator_traits<TIter>::value_type key = *i;
        TIter j = i - 1;

        while (j >= begin && comp(key, *j)) {
            *(j + 1) = std::move(*j);
            --j;
        }

        *(j + 1) = std::move(key);
    }
}

struct Chunk {
    int firstIdx;
    int lastIdx; // inclusive
};

template <typename T>
std::vector<Chunk> splitIntoChunks(const std::vector<T>& v, int nChunks) {
    std::vector<Chunk> chunks;
    int size = v.size();
    if (size == 0) {
        return chunks;
    }
    int chunkSize = size / nChunks;
    int remaining = size % nChunks;

    int startIdx = 0;
    for (int i = 0; i < nChunks; ++i) {
        int currentChunkSize = chunkSize + (i < remaining ? 1 : 0);
        int endIdx = startIdx + currentChunkSize - 1;
        chunks.push_back({startIdx, endIdx});
        startIdx = endIdx + 1;
    }

    return chunks;
}

} // lunachess::utils

#endif // LUNA_UTILS_H
