#include <iostream>
#include <ostream>
#include <queue>
#include <thread>

#include "pthread_bruteforce.h"
#include "calc_md5.h"

struct ThreadData {
    std::string md5_hash;
    std::string alphabet;
    int max_length;
    std::string prefix;
    pthread_t pthread;
};

static bool found {false};
static std::string result;

// Рекурсивная функция для перебора всех возможных строк заданной длины
static void brute_force(const std::string &md5_hash, const std::string &alphabet, std::string &current, int result_length) {
    if (found) return;

    if (current.size() == result_length) {
        std::string hash = calculate_md5(current);
        if (hash == md5_hash) {
            found = true;
            result = current;
        }
        return;
    }

    for (char c : alphabet) {
        if (found) break;
        current.push_back(c);
        brute_force(md5_hash, alphabet, current, result_length);
        current.pop_back();
    }
}

// pthread routine
void* thread_func(void* arg) {
    const auto* data = static_cast<ThreadData*>(arg);
    std::string current = data->prefix;
    brute_force(data->md5_hash, data->alphabet, current, data->max_length);
    return nullptr;
}

std::string pthread_md5_crack(const std::string &md5_hash, const std::string &alphabet, int result_length) {
    size_t num_cores = std::thread::hardware_concurrency();
    if (num_cores == 0) {
        std::cout << "Не удалось определить количество ядер." << std::endl;
        num_cores = 2;
    } else {
        std::cout << "Количество ядер процессора: " << num_cores << std::endl;
    }
    std::queue<ThreadData> thread_data;

    // prepare the queue
    size_t max_threads = num_cores;
    if (max_threads > alphabet.size()) {
        max_threads = alphabet.size();
    }

    int thread_run = 0;
    for (const auto c: alphabet) {
        ThreadData td {md5_hash, alphabet, result_length, std::string(1, c)};
        pthread_create(&td.pthread, nullptr, thread_func, &td);
        thread_data.push(td);
        thread_run++;
        if (thread_run >= max_threads) {
            td = thread_data.front();
            thread_data.pop();
            pthread_join(td.pthread, nullptr);
            thread_run--;
        }
        if (found) {
            break;
        }
    }

    // Wait for other threads completion
    while (not thread_data.empty()) {
        pthread_join(thread_data.front().pthread, nullptr);
        thread_data.pop();
    }

    return result;
}
