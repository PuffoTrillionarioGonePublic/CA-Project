#ifndef CA_PROJ_SRC_CUDACORRELATIONCALCULATOR_H_
#define CA_PROJ_SRC_CUDACORRELATIONCALCULATOR_H_
#include "util.h"
#include "CorrelationCalculator.h"

struct CudaCorrelationCalculator : public CorrelationCalculator {
  std::any impl_;
  CudaCorrelationCalculator(size_t chunk_size, std::unique_ptr<csv::reader> r, FPMode m = kUseDouble);
  EvaluatedGenericResultContainer evaluate_all() override;
};

#endif