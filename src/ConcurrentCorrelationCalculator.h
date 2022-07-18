#ifndef CA_PROJ_SRC_CONCURRENTCORRELATIONCALCULATOR_H_
#define CA_PROJ_SRC_CONCURRENTCORRELATIONCALCULATOR_H_
#include "CorrelationCalculator.h"
#include "util.h"

struct ConcurrentCorrelationCalculator : public CorrelationCalculator {

  std::any impl_;

  ConcurrentCorrelationCalculator(size_t chunk_size,
                                  std::unique_ptr<csv::reader> r,
                                  FPMode m = kUseDouble);

  EvaluatedGenericResultContainer evaluate_all() override;

};

#endif
