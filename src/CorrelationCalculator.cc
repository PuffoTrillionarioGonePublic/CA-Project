#include "CorrelationCalculator.h"
#include "SimpleCorrelationCalculator.h"
#include "ConcurrentCorrelationCalculator.h"
#include "CudaCorrelationCalculator.h"

auto CorrelationCalculator::create_or_throw(const std::string &s,
                                            size_t chunk_size,
                                            std::unique_ptr<csv::reader> r,
                                            FPMode m) -> std::unique_ptr<
    CorrelationCalculator> {
  if (s == "simple") {
    return std::make_unique<SimpleCorrelationCalculator>(chunk_size, std::move(r), m);
  } else if (s == "concurrent") {
    return std::make_unique<ConcurrentCorrelationCalculator>(chunk_size, std::move(r), m);
  } else if (s == "cuda") {
    return std::make_unique<CudaCorrelationCalculator>(chunk_size, std::move(r), m);
  } else {
    throw std::runtime_error("invalid correlation calculator");
  }
}