// Headers included by Visual Studio, unnecessary

#include "../modules/CPP-csv-parser/csv.hh"
#include "../modules/CPP-math-utils/convertions.hh"
#include <math.h>
#include "CudaCorrelationCalculator.h"
#include <stdio.h>
#include <string>
#include <utility>
#include <optional>



#define COUPLE_NUMBER(n) ((n-1)*(n)/2)
#define MIN(x, y) ((x) < (y) ? (x) : (y))

std::vector<double> cuda_evaluate_double(csv::reader& r, std::size_t chunk_size);
std::vector<float> cuda_evaluate_float(csv::reader& r, std::size_t chunk_size);

// cast result to a common format
template <typename T>
EvaluatedResultContainer<T> cast_results(const int n, const std::vector<T> &hres) {
    EvaluatedResultContainer<T> ans;
    int p = 0;
    for (int i{}; i != n - 1; ++i) {
        for (int j{i + 1}; j != n; ++j) {
            ans[std::make_pair(i,j)] = hres[p++];
        }
    }
    return ans;
}

template <typename U>
EvaluatedResultContainer<U> ev_all(csv::reader& r, std::size_t chunk_size) {
    return EvaluatedResultContainer<U>();
}

template <>
EvaluatedResultContainer<float> ev_all(csv::reader& r, std::size_t chunk_size) {
    int n = (int)r.column_count();
    std::vector<float> res = cuda_evaluate_float(r, chunk_size);
    return cast_results<float>(n, res);
}

template <>
EvaluatedResultContainer<double> ev_all(csv::reader& r, std::size_t chunk_size) {
    int n = (int)r.column_count();
    std::vector<double> res = cuda_evaluate_double(r, chunk_size);
    return cast_results<double>(n, res);
}

template <typename T>
struct CudaCorrelationCalculatorImpl {

	static constexpr uint blocks = 16, threads_per_block = 1024;
	size_t chunk_size_;
	std::unique_ptr<csv::reader> csv_reader_;

	CudaCorrelationCalculatorImpl(size_t chunk_size, std::unique_ptr<csv::reader> r) {
		chunk_size_ = chunk_size;
		csv_reader_ = std::move(r);
	}

	EvaluatedResultContainer<T> evaluate_all() {
		return ev_all<T>(*csv_reader_, chunk_size_);
	}
};


//CudaCorrelationCalculator::CudaCorrelationCalculator(size_t chunk_size, CorrelationCalculator::FPMode m) {
CudaCorrelationCalculator::CudaCorrelationCalculator(size_t chunk_size, std::unique_ptr<csv::reader> r, FPMode m) {
	if (m == CorrelationCalculator::FPMode::kUseDouble) {
		impl_ = std::make_shared<CudaCorrelationCalculatorImpl<double>>(chunk_size, std::move(r));
	} else {
		impl_ = std::make_shared<CudaCorrelationCalculatorImpl<float>>(chunk_size, std::move(r));
	}
}

EvaluatedGenericResultContainer CudaCorrelationCalculator::evaluate_all() {
	auto *ptr = std::any_cast<std::shared_ptr<CudaCorrelationCalculatorImpl<float>>>(&impl_);
	if (ptr != nullptr) {
		return (*ptr)->evaluate_all();
	} else {
		auto *p = std::any_cast<std::shared_ptr<CudaCorrelationCalculatorImpl<double>>>(&impl_);
		if (!p) {
			throw std::runtime_error{"can't evaluate"};
		}
		return (*p)->evaluate_all();
	}
}
