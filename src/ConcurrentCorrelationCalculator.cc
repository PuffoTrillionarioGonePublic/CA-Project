#include "ConcurrentCorrelationCalculator.h"


template<typename T>
struct ConcurrentCorrelationCalculatorImpl : std::enable_shared_from_this<
    ConcurrentCorrelationCalculatorImpl<T>> {

  std::mutex mutex_;
  static constexpr std::size_t kQueueLimit = 1000;
  std::queue<Vec<Vec<T>>> chunk_queue_;
  std::condition_variable waiting_producer_;
  int count_waiting_consumers_{};
  std::condition_variable waiting_consumers_;
  bool end_of_data_reached_{};
  Vec<ResultContainer<T>> results_;
  Vec<std::thread> workers{};

  void concat_result_container(ResultContainer<T> &p1, const ResultContainer<T> &p2) {
    if (p1.size() != p2.size()) {
      throw std::runtime_error("");
    }
    for (const auto &val : p2) {
      p1[val.first] += val.second;
    }
  }

  static ResultContainer<T> allocate_result_container(std::size_t ts_count) {
    ResultContainer<T> ans;
    math::sets::couple c(static_cast<int>(ts_count));
    do {
      ans[c.as_pair()];
      ++c;
    } while (!c.finished());
    return ans;
  }

  void evaluate(ResultContainer<T> &results,
                const Vec<Vec<T>> &dataset) {
    math::sets::couple c(static_cast<int>(dataset.size()));
    do {
      results[c.as_pair()] += math::statistics::pearson_correlation_coefficient(dataset[c.first()],
                                                                                dataset[c.second()]);
      ++c;
    } while (!c.finished());
  }

  void store_worker_result(const ResultContainer<T> &partial_res) {
    static std::mutex res_mutex;
    std::unique_lock lock(res_mutex);
    results_.push_back(partial_res);
  }

  void sumbit(Vec<Vec<T>> val) {
    std::unique_lock lock(mutex_);
    while (chunk_queue_.size() == kQueueLimit) {
      waiting_producer_.wait(lock, [&]() { return chunk_queue_.size() != kQueueLimit; });
    }
    chunk_queue_.emplace(val);
    if (count_waiting_consumers_) {
      waiting_consumers_.notify_all();
    }
  }

  void mark_dataset_as_fully_read() {
    std::unique_lock lock{mutex_};
    end_of_data_reached_ = true;
    waiting_consumers_.notify_all();
  }

  std::optional<Vec<Vec<T>>> acquire() {
    std::unique_lock lock{mutex_};

    if (chunk_queue_.empty()) {
      if (!end_of_data_reached_) {
        ++count_waiting_consumers_;
        waiting_consumers_.wait(lock,
                                [&]() { return !chunk_queue_.empty() || end_of_data_reached_; });
        --count_waiting_consumers_;
      }
      if (chunk_queue_.empty() && end_of_data_reached_) {
        return std::nullopt;
      }
    }

    Vec<Vec<T>> ans = chunk_queue_.front();
    chunk_queue_.pop();

    return ans;
  }

  void spawn_workers(std::size_t ts_count) {
    size_t n = std::thread::hardware_concurrency();
    auto self = std::enable_shared_from_this<ConcurrentCorrelationCalculatorImpl<T>>
    ::shared_from_this();
    for (size_t i = 0; i < n; i++) {
      workers.emplace_back([ts_count, self]() {
        auto result_container = allocate_result_container(ts_count);
        for (;;) {
          auto chunk = self->acquire();
          if (!chunk.has_value()) {
            break;
          }
          self->evaluate(result_container, chunk.value());
        }
        self->store_worker_result(result_container);
      });
    }
  }

  ResultContainer<T> join_all() {
    for (auto &w : workers) {
      w.join();
    }
    auto ans = results_.front();
    for (size_t i = 1; i < results_.size(); i++) {
      concat_result_container(ans, results_[i]);
    }
    results_.clear();
    return ans;
  }

  std::unique_ptr<csv::reader> csv_reader_;
  const size_t chunk_size_;
  ConcurrentCorrelationCalculatorImpl(size_t chunk_size,
                                      std::unique_ptr<csv::reader> r
  ) : chunk_size_{chunk_size},
      csv_reader_{std::move(r)} {}

  static auto create(size_t chunk_size,
                     std::unique_ptr<csv::reader> r) -> std::shared_ptr<
      ConcurrentCorrelationCalculatorImpl> {
    return std::make_shared<ConcurrentCorrelationCalculatorImpl>(chunk_size, std::move(r));
  }

  EvaluatedResultContainer<T> evaluate_all() {
    size_t ts_count = csv_reader_->column_count();

    spawn_workers(ts_count);
    for (;;) {
      auto chunk = get_chunk<T>(*csv_reader_, chunk_size_);
      if (!chunk.has_value()) {
        break;
      }
      sumbit(chunk.value());
    }
    mark_dataset_as_fully_read();
    return evaluate_result_container(join_all());
  }
};

ConcurrentCorrelationCalculator::ConcurrentCorrelationCalculator(size_t chunk_size,
                                                                 std::unique_ptr<csv::reader> r,
                                                                 CorrelationCalculator::FPMode m) {
  if (m == kUseDouble) {
    impl_ = ConcurrentCorrelationCalculatorImpl<double>::create(chunk_size, std::move(r));
  } else {
    impl_ = ConcurrentCorrelationCalculatorImpl<float>::create(chunk_size, std::move(r));
  }
}

EvaluatedGenericResultContainer ConcurrentCorrelationCalculator::evaluate_all() {
  auto *ptr = std::any_cast<std::shared_ptr<ConcurrentCorrelationCalculatorImpl<float>>>(&impl_);
  if (ptr != nullptr) {
    return (*ptr)->evaluate_all();
  } else {
    auto *p = std::any_cast<std::shared_ptr<ConcurrentCorrelationCalculatorImpl<double>>>(&impl_);
    if (!p) {
      throw std::runtime_error{"can't evaluate"};
    }
    return (*p)->evaluate_all();
  }
}
