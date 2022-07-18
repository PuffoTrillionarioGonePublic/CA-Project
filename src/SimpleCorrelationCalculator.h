#ifndef CA_PROJ_SRC_SIMPLECORRELATIONCALCULATOR_H_
#define CA_PROJ_SRC_SIMPLECORRELATIONCALCULATOR_H_
#include "CorrelationCalculator.h"
#include "util.h"


struct SimpleCorrelationCalculator : public CorrelationCalculator {
  std::any impl_;
  SimpleCorrelationCalculator(size_t chunk_size,
                              std::unique_ptr<csv::reader> r,
                              FPMode m = kUseDouble);

  EvaluatedGenericResultContainer evaluate_all() override;

};

#endif //CA_PROJ_SRC_SIMPLECORRELATIONCALCULATOR_H_
