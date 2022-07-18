#ifndef CA_PROJ_SRC_UTIL_H_
#define CA_PROJ_SRC_UTIL_H_
#include "../modules/CPP-csv-parser/csv.hh"
#include "../modules/CPP-math-utils/correlation.hh"
#include "../modules/CPP-math-utils/couple.hh"
#include "../modules/CPP-math-utils/convertions.hh"

#include <any>
#include <type_traits>
#include <variant>
#include <iostream>
#include <optional>
#include <cstdlib>
#include <fstream>
#include <string>
#include <map>
#include <condition_variable>
#include <memory>
#include <thread>
#include <mutex>
#include <queue>

using namespace std::literals;
template<typename T, typename A = std::allocator<T>>
using Vec = std::vector<T, A>;

template<typename T>
using ResultContainer = std::map<std::invoke_result_t<decltype(&math::sets::couple::as_pair),
                                                      math::sets::couple>,
                                 math::statistics::pcc_partial<T>>;
template<typename T>
using EvaluatedResultContainer = std::map<std::invoke_result_t<decltype(&math::sets::couple::as_pair),
                                                               math::sets::couple>, T>;

using GenericResultContainer = std::variant<ResultContainer<float>, ResultContainer<double>>;
using EvaluatedGenericResultContainer = std::variant<EvaluatedResultContainer<float>, EvaluatedResultContainer<double>>;

template <typename T>
inline EvaluatedResultContainer<T> evaluate_result_container(const ResultContainer<T> &results) {
  EvaluatedResultContainer<T> ans;
  for (const auto &val : results) {
    ans[{val.first.first, val.first.second}] = val.second.compute();
  }
  return ans;
}



inline void print_results(const EvaluatedGenericResultContainer &results) {
  auto *ptr = std::get_if<0>(&results);
  if (ptr != nullptr) {
    for (const auto &val : *ptr) {
      std::cout << "(" << val.first.first << ","
                << val.first.second << ") " << val.second << "\n";
    }
    std::cout.flush();
  } else {
    for (const auto &val : *std::get_if<1>(&results)) {
      std::cout << "(" << val.first.first << ","
                << val.first.second << ") " << val.second << "\n";
    }
    std::cout.flush();
  }

}

template<typename T>
inline std::optional<Vec<Vec<T>>> get_chunk(csv::reader &r,
                                     std::size_t line_count) {
  Vec<Vec<T>> ans(r.column_count());
  for (auto &v : ans) {
    v.reserve(line_count);
  }
  auto counter = line_count;
  while (r.can_read() && counter--) {
    auto line = r.getline();
    for (size_t i = 0; i < line.size(); ++i) {
      ans[i].push_back(math::convertions::ston<T>(line.data()[i]));
    }
  }
  if (counter == line_count) {
    return std::nullopt;
  }
  return ans;
}

#endif //CA_PROJ_SRC_UTIL_H_
