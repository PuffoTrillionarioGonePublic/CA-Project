/**
 * Read the csv indicated as first argument
 *
 */

#include "../modules/CPP-csv-parser/csv.hh"
#include "../modules/CPP-math-utils/correlation.hh"
#include "../modules/CPP-math-utils/couple.hh"

#include <type_traits>
#include <algorithm>
#include <iostream>
#include <optional>
#include <cstdlib>
#include <fstream>
#include <string>
#include <map>

using namespace std;

using data_type = float;

[[noreturn]] void help(const std::string& cmd, int exit_status = EXIT_FAILURE) {
    std::cerr << "Usage:\n";
    std::cerr << '\t' << cmd << " input.csv\n";
    std::cerr << "\t\t" << "input.csv: file containing time series\n";
    std::cerr << "\t\t" << "chunk_size: how many lines per chunk\n";
    exit(exit_status);
}

/**
 * Return a collection of vectors containing the data in the next
 * line_count rows of the csv.
 */
std::optional<std::vector<std::vector<data_type>>> get_chunk(csv::reader& r, std::size_t line_count) {
    std::vector<std::vector<data_type>> ans(r.column_count());
    for (auto &v : ans) {
        v.reserve(line_count);
    }
    auto counter = line_count;
    while (r.can_read() && counter--) {
        auto line = r.getline();
        for (size_t i = 0; i < line.size(); ++i) {
            ans[i].push_back(std::stod(line.data()[i]));
        }
    }
    if (counter == line_count) {
        return std::nullopt;
    }
    return ans;
}

std::optional<std::vector<std::vector<data_type>>> get_chunk_rows(csv::reader& r, std::size_t line_count) {
    std::vector<std::vector<data_type>> ans;
    // at most line_count
    ans.reserve(line_count);
    auto counter = line_count;
    while (r.can_read() && counter--) {
        std::vector<data_type> row;
        auto line = r.getline();
        row.reserve(line.size());
        for (const auto &d : line.data()) {
            row.push_back(std::stod(d));
        }
        ans.push_back(std::move(row));
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

void evaluate(math::statistics::multicolumn_pcc_accumulator<data_type>& raccumulator, const std::vector<std::vector<data_type>>& dataset_rows) {
    for (const auto& row : dataset_rows) {
        //raccumulator.accumulate(row);
    }
}

void print_results(const result_container_type& results) {
    for (const auto &val : results) {
        std::cout << "(" << val.first.first << ","
            << val.first.second << ") " << val.second.compute() << "\n";
    }
    std::cout.flush();
}

void print_results(const std::map<std::pair<int,int>,data_type>& results) {
    for (const auto &val : results) {
        std::cout << "(" << val.first.first << ","
            << val.first.second << ") " << val.second << "\n";
    }
    std::cout.flush();
}

int main(int argc, char const *argv[])
{
    // By default read at most chunk_size
    // lines from the dataset
    std::size_t chunk_size = 1000;
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
    math::statistics::multicolumn_pcc_accumulator<data_type> accumulator(ts_count);

    while (true) {
        auto chunk = get_chunk_rows(r, chunk_size);
        if (!chunk.has_value()) {
            break;
        }
        evaluate(accumulator, chunk.value());
    }

    print_results(accumulator.results());

    return 0;
}

