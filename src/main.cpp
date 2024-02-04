#include <chrono>
#include <mutex>
#include <thread>
#include <cstring>
#include <iostream>
#include <vector>
#include <filesystem>
#include <map>
#include <unordered_map>
#include "MemoryMapped.h"

#ifdef _MSC_VER
#include <Windows.h>
#endif

#ifndef CSV_FILE
#define CSV_FILE "C:\\Users\\Anatolii.Klots\\Downloads\\1brc-main\\1brc-main\\measurements.txt"
#endif // !CSV_FILE

struct string_vector_t
{
    const unsigned char* start;
    unsigned int size;

    bool operator == (string_vector_t const& other) const
    {
        return memcmp(start, other.start, size) == 0;
    }

    bool operator < (string_vector_t const& other) const
    {
        return memcmp(start, other.start, size) < 0;
    }
};

std::ostream& operator<<(std::ostream& os, const string_vector_t& string)
{
    for (unsigned i=0; i < string.size; ++i)
    {
        os << string.start[i];
    }
    return os;
}

template<>
struct std::hash<string_vector_t> {

	using argument_type = string_vector_t;
	using result_type = size_t;

	size_t operator()(const string_vector_t& r) const noexcept
	{
		constexpr std::hash<unsigned char> start_hash;
		constexpr std::hash<unsigned int> size_hash;

        auto result = size_hash(r.size);

        for (unsigned i=0; i < r.size; i++)
        {
            result ^= start_hash(r.start[i]);
        }

        return result;
	}
};

struct city_info_t {

    using iterator = std::vector<float>::iterator;
    using const_iterator = std::vector<float>::const_iterator;

    iterator begin() { return _values.begin(); }
    iterator end() { return _values.end(); }

    const_iterator begin() const { return _values.begin(); }
    const_iterator end() const { return _values.end(); }
    const_iterator cbegin() const { return _values.cbegin(); }
    const_iterator cend() const { return _values.cend(); }

    void add_temp(float temp) {

        _values.push_back(temp);

        if (temp > _max) {
            _max = temp;
        }
        if (temp < _min) {
            _min = temp;
        }
    }

    float get_mid() const {

        if (_values.size() == 1) {
            return _values[0];
        }
        if (_values.size() > 1)
        {
            auto mid = _values.size() / 2;
            return _values[mid];
        }

        return -1000;
    }

    inline int min_temp() const {
        return _min;
    }

    inline int max_temp() const {
        return _max;
    }

private:
    int _min = 1000;
    int _max = -1000;
    std::vector<float>  _values;
};

std::ostream& operator<<(std::ostream& os, const city_info_t& info)
{
    return os
        << info.min_temp() << "/"
        << info.get_mid() << "/"
        << info.max_temp();
}

const auto processor_count = std::thread::hardware_concurrency();

std::vector<std::thread> threads(processor_count);

std::unordered_map<string_vector_t, city_info_t> _cities;

std::mutex cities_mutex;

static unsigned long file_size(const char* filename) {
    std::filesystem::path p { filename };
    return std::filesystem::file_size(p);
}

static int copy_until(const unsigned char* input, char* output, const char stop_character = '\n', const int input_pos = 0) {

    auto read = 0;

    while (true)
    {
        auto data = input[input_pos + read];

        if (data == stop_character || data == NULL)
        {
            return read;
        }

        output[read] = data;
        read++;
    }

    return read;
}

static int get_pos(const unsigned char* input, const unsigned char character, const int input_pos = 0) 
{
    auto read = 0;

    while (true)
    {
        auto data = input[input_pos + read];

        if (data == character || data == NULL)
        {
            return read;
        }
        read++;
    }
    return read;
}

static void run_work(const unsigned char * input, long start, const long offset) {

    std::unordered_map<string_vector_t, city_info_t> cities;

    char city_temp[10];

    int read = 0;
    int block_position = 0;

    if (start != 0) {
        start += get_pos(input, '\n', start) + 1;
    }

    while (true)
    {
        memset(city_temp, 0, sizeof(city_temp));

        string_vector_t city_name { input + start + block_position, 0 };

        read = get_pos(input, ';', start + block_position);
       // read = copy_until(input, city_name, ';', start + block_position);
        if (read == 0)
        {
            break;
        }

        city_name.size = read;

        block_position += read + 1;

        read = copy_until(input, city_temp, '\n', start + block_position);
        if (read == 0)
        {
            break;
        }

        block_position += read + 1;
;
        cities[city_name].add_temp(atof(city_temp));

        if (block_position >= offset) {
            break;
        }
    }

    cities_mutex.lock();

    for (const auto& pair : cities) {
        auto& city = _cities[pair.first];
        for (const auto temp : pair.second) {
            city.add_temp(temp);
        }
    }

    cities_mutex.unlock();
}

static void print_results()
{
    std::cout << "{";

    auto i = _cities.size();

    // Sort 'unordered_map' by iterating over a new sorted 'map'
    const std::map ordered_cities(_cities.begin(), _cities.end());

    for (auto const& pair : ordered_cities) {
        std::cout << pair.first << "=" << pair.second;
        if (--i > 0)
            std::cout << ", ";
    }

    std::cout << "}\n";
}

int main(int argc, char* argv[])
{
#ifdef _MSC_VER
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

 //   std::vector<string_vector_t> test;

	//const auto test_string =  reinterpret_cast<const unsigned char*>("some test string!");

 //   const auto test_string2 = reinterpret_cast<const unsigned char*>("some test string!");

 //   string_vector_t vector1 { test_string, sizeof(test_string)};
 //   string_vector_t vector2 { test_string2, sizeof(test_string2) };

 //   auto r = vector1 == vector2;

    //if (argc <= 1) {
    //    std::cout << "A file path needs to be specified." << std::endl;
    //    return 1;
    //}

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    MemoryMapped mapped_file(CSV_FILE, MemoryMapped::WholeFile, MemoryMapped::SequentialScan);

    if (!mapped_file.isValid()) {
        std::cout << "Can't open the file\n";
        return 1;
    }

    auto size_per_processor = file_size(CSV_FILE) / processor_count;

    for (int i = 0; i < threads.size(); i++) {
	    threads[i] = std::thread(run_work, mapped_file.getData(), i * size_per_processor, size_per_processor);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    print_results();

    mapped_file.close();
    
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    std::cout << "Time elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "s" << std::endl;

    return 0;
}