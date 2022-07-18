#ifndef CA_PROJ_SRC_CORRELATIONCALCULATOR_H_
#define CA_PROJ_SRC_CORRELATIONCALCULATOR_H_
#include "util.h"


struct CorrelationCalculator {
  enum FPMode {
    kUseFloat,
    kUseDouble
  };

  static auto create_or_throw(const std::string &s,
                              size_t chunk_size,
                              std::unique_ptr<csv::reader> r,
                              FPMode m = kUseDouble) -> std::unique_ptr<CorrelationCalculator>;

  virtual EvaluatedGenericResultContainer evaluate_all() = 0;
  virtual ~CorrelationCalculator() = default;
};


#endif //CA_PROJ_SRC_CORRELATIONCALCULATOR_H_
