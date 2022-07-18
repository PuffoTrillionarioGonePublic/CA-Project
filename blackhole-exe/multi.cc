/**
 * Read the csv indicated as first argument
 * Same as single.cc but multi thread
 */

#include "../modules/CPP-csv-parser/csv.hh"
#include "../modules/CPP-math-utils/correlation.hh"
#include "../modules/CPP-math-utils/couple.hh"
#include "../modules/CPP-math-utils/convertions.hh"

#include <condition_variable>
#include <type_traits>
#include <algorithm>
#include <iostream>
#include <optional>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <map>

using namespace std;

using data_type = double;

[[noreturn]] void help(const std::string& cmd, int exit_status = EXIT_FAILURE) {
    std::cerr << "Usage:\n";
    std::cerr << '\t' << cmd << " input.csv [chunk_size]\n";
    std::cerr << "\t\t" << "input.csv: file containing time series\n";
    std::cerr << "\t\t" << "chunk_size: how many lines per chunk\n";
    exit(exit_status);
}

using chunk_type = std::vector<std::vector<data_type>>;

/**
 * Return a collection of vectors containing the data in the next
 * line_count rows of the csv.
 */
std::optional<chunk_type> get_chunk(csv::reader& r, std::size_t line_count) {
    chunk_type ans(r.column_count());
    for (auto &v : ans) {
        v.reserve(line_count);
    }
    auto counter = line_count;
    while (r.can_read() && counter--) {
        auto line = r.getline();
        for (size_t i = 0; i < line.size(); ++i) {
            ans[i].push_back(math::convertions::ston<data_type>(line.data()[i]));
        }
    }
    if (counter == line_count) {
        return std::nullopt;
    }
    return ans;
}

using result_container_type = std::map<std::invoke_result_t<decltype(&math::sets::couple::as_pair), math::sets::couple>, math::statistics::pcc_partial<data_type>>;

auto allocate_result_container(std::size_t ts_count) {
    result_container_type ans;
    math::sets::couple c(static_cast<int>(ts_count));
    do {
        ans[c.as_pair()];
        ++c;
    } while (!c.finished());
    return ans;
}

void evaluate(result_container_type& results, const std::vector<std::vector<data_type>>& dataset) {
    math::sets::couple c(static_cast<int>(dataset.size()));
    do {
        //results[c.as_pair()] += math::statistics::pearson_correlation_coefficient(dataset[c.first()], dataset[c.second()]);
        ++c;
    } while (!c.finished());
}

void print_results(const result_container_type& results) {
    for (const auto &val : results) {
        std::cout << "(" << val.first.first << ","
            << val.first.second << ") " << val.second.compute() << "\n";
    }
    std::cout.flush();
}

// Used at end of all thread to merge results
void operator +=(result_container_type& p1, const result_container_type& p2) {
    if (p1.size() != p2.size()) {
        throw std::runtime_error("");
    }
    for (const auto &val : p2) {
        p1[val.first] += val.second;
    }
}

// mutex to access queue of chunks to be processed
std::mutex mutex;
// maximum number of chunks waiting into the queue
// producer is paused here when the queue reach this size
volatile constexpr std::size_t queue_limit{10000};
// queue containing data to analize
std::queue<chunk_type> chunk_queue;

// if true the producer is waiting for submit
volatile bool is_producer_waiting{};
// here the producer will wait until new data could be submitted
std::condition_variable waiting_producer;

// number of consumers waiting for data
volatile int count_waiting_consumers{};
// here consumers will wait
std::condition_variable waiting_consumers;

// producer has completed its work?
volatile bool end_of_data_reached{};

// here workers will store their results
std::vector<result_container_type> results;

// called by workers before terminating
void store_worker_result(result_container_type&& partial_res) {
    static std::mutex res_mutex;
    std::unique_lock<decltype(res_mutex)> lock(res_mutex);
    results.push_back(std::move(partial_res));
}

// submit a chunk to the workers
void sumbit(chunk_type&& val) {
    std::unique_lock<decltype(::mutex)> lock(::mutex);
    // if queue is full pause producer
    while (chunk_queue.size() == queue_limit) {
        is_producer_waiting = true;
        waiting_producer.wait(lock, [&](){ return chunk_queue.size() != queue_limit; });
    }
    is_producer_waiting = false;
    chunk_queue.emplace(std::move(val));
    // if any consumer is waiting wake them up!
    if (count_waiting_consumers) {
        waiting_consumers.notify_all();
    }
}

// called by producer when the whole datased has been processed
void mark_dataset_as_fully_read() {
    std::unique_lock<decltype(::mutex)> lock(::mutex);
    end_of_data_reached = true;
    waiting_consumers.notify_all();
}

// get a chunk, used by workers
std::optional<chunk_type> acquire() {
    std::unique_lock<decltype(::mutex)> lock(::mutex);

    if (chunk_queue.empty())
    {
        if (!end_of_data_reached) {
            ++count_waiting_consumers;
            waiting_consumers.wait(lock, [&](){ return !chunk_queue.empty() || end_of_data_reached; });
            --count_waiting_consumers;
        }
        if (chunk_queue.empty() && end_of_data_reached) {
            // no more data will be supplied
            return std::nullopt;
        }
    }

    // if queue is empty pause
    chunk_type ans(std::move(chunk_queue.front()));
    chunk_queue.pop();

    // possibly wake consumer
    return ans;
}

std::vector<std::thread> workers;
// Spawn many workers as hardware concurrency
void spawn_workers(std::size_t ts_count) {
    auto n = std::thread::hardware_concurrency();
    for (decltype(n) i{}; i != n; ++i) {
        workers.emplace_back([ts_count](){
            // now per thread
            auto result_container = allocate_result_container(ts_count);
            // loop until end of data
            while (true) {
                // try to get data
                auto chunk = acquire();
                if (!chunk.has_value()) {
                    break;
                }
                // evaluate if possible
                evaluate(result_container, chunk.value());
            }
            // store results
            store_worker_result(std::move(result_container));
        });
    }
}

// called by main thread at the end to merge results
// join workers and get results
auto join_all() {
    for (auto& w : ::workers)
    {
        // join workers one by one
        w.join();
    }
    auto ans = std::move(results.front());
    for (decltype(results.size()) i{1}; i!=results.size(); ++i) {
        ans += results[i];
    }
    // all data have been consumed
    results.clear();
    return ans;
}

int main(int argc, char const *argv[])
{
    // By default read at most chunk_size
    std::size_t chunk_size = 500;
    // lines from the dataset
    if (argc < 2 || (argc >= 2 && argv[1] == "--help"s)) {
        help(argv[0], !(argc >= 2 && argv[1] == "--help"s));
    }
    if (argc >= 3) {
        try {
            auto tmp = std::stol(argv[2]);
            if (tmp <= 0) {
                throw std::exception();
            }
            chunk_size = tmp;
        } catch(const std::exception& e) {
            std::cerr << "chunk_size must be a positive integer, found '" << argv[2] << "'" << '\n';
            help(argv[0], EXIT_FAILURE);
        }
    }
    std::string input(argv[1]);
    std::ifstream fin(input);
    if (!fin.good()) {
        std::cerr << "Failed to open file '" << input << "'\n";
        help(argv[0], EXIT_FAILURE);
    }

    csv::reader r(fin);
    auto ts_count = r.column_count();

    // since this allocation the map containing
    // the results will never change its size,
    // so it is safe to access distinct elements
    // from different thread because no ops will
    // be performed on its structure
    //
    // Currently replicated per thread
    //auto result_container = allocate_result_container(ts_count);

    spawn_workers(ts_count);
    while (true) {
        auto chunk = get_chunk(r, chunk_size);
        if (!chunk.has_value()) {
            break;
        }
        // submit to workers
        sumbit(std::move(chunk.value()));
    }
    // all work completed
    mark_dataset_as_fully_read();

    print_results(join_all());

    return 0;
}

