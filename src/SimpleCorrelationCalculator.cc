#include "SimpleCorrelationCalculator.h"

template<typename T>
struct CorrelationCalculatorImpl {

  ResultContainer<T> allocate_result_container(std::size_t ts_count) {
    ResultContainer<T> ans;
    math::sets::couple c(static_cast<int>(ts_count));
    do {
      ans[c.as_pair()];
      ++c;
    } while (!c.finished());
    return ans;
  }

  void evaluate(ResultContainer<T> &results, const Vec<Vec<T>> &dataset) {
    math::sets::couple c(static_cast<int>(dataset.size()));
    do {
      results[c.as_pair()] += math::statistics::pearson_correlation_coefficient(dataset[c.first()],
                                                                                dataset[c.second()]);
      ++c;
    } while (!c.finished());
  }

  const size_t kChunkSize;
  std::unique_ptr<csv::reader> csv_reader_;

  CorrelationCalculatorImpl(size_t chunk_size, std::unique_ptr<csv::reader> csv_reader)
      : kChunkSize(chunk_size), csv_reader_{std::move(csv_reader)} {}

  EvaluatedResultContainer<T> evaluate_all() {

    auto ts_count = csv_reader_->column_count();
    ResultContainer<T> result_container = allocate_result_container(ts_count);

    for (;;) {
      auto chunk = get_chunk<T>(*csv_reader_, kChunkSize);
      if (!chunk.has_value()) {
        break;
      }
      evaluate(result_container, chunk.value());
    }
    return evaluate_result_container(result_container);
  }

};

SimpleCorrelationCalculator::SimpleCorrelationCalculator(size_t chunk_size,
                                                         std::unique_ptr<csv::reader> r,
                                                         CorrelationCalculator::FPMode m) {
  if (m == kUseDouble) {
    auto a = std::make_shared<CorrelationCalculatorImpl<double>>(chunk_size, std::move(r));
    impl_ = std::move(a);
  } else {
    impl_ = std::make_shared<CorrelationCalculatorImpl<float>>(chunk_size, std::move(r));
  }
}

EvaluatedGenericResultContainer SimpleCorrelationCalculator::evaluate_all() {
  auto *ptr = std::any_cast<std::shared_ptr<CorrelationCalculatorImpl<float>>>(&impl_);
  if (ptr != nullptr) {
    return (*ptr)->evaluate_all();
  } else {
    auto *p = std::any_cast<std::shared_ptr<CorrelationCalculatorImpl<double>>>(&impl_);
    if (!p) {
      throw std::runtime_error{"can't evaluate"};
    }
    return (*p)->evaluate_all();
  }
}
