// Headers included by Visual Studio, unnecessary
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#include "../modules/CPP-csv-parser/csv.hh"
#include "../modules/CPP-math-utils/convertions.hh"
#include <math.h>

#include <stdio.h>
#include <istream>
#include <fstream>
#include <string>
#include <utility>
#include <optional>
#include <vector>

#include <thrust/device_vector.h>
#include <thrust/host_vector.h>

using namespace std;

using T = double;

// Macro can be used anywhere, regardeless of CUDA support for C++ code
#define COUPLE_NUMBER(n) ((n-1)*(n)/2)

// MIN macro
#define MIN(x,y) ((x) < (y) ? (x) : (y))


struct Pair {
    int first = 0, second = 0;
};

template <typename T>
struct PCC_Partial {
    long long count{};
    T sum_1{};
    T sum_2{};
    T sum_1_squared{};
    T sum_2_squared{};
    T sum_prod{};
};


// Cuda seems not supporting classes, this function was the core
// of the math::sets::couple class so I extracted it to
//  https://github.com/Tredici/CPP-math-utils/blob/b3b3f844d51b1014e20d329009461b8ac74ef21d/couple.hh#L18
__device__ Pair pair(int n, int i) {
    // candidate supposing al pairs are ok
    Pair p{ i / n, i % n };

    // first column and no overflow?
    if (p.first == 0 && p.second + 1 < n) {
        p.second += 1;
        return p;
    }
    else if (p.first == 0 && p.second + 1 == n) {
        p.first = 1; p.second = 2;
        return p;
    }

    // reduce problem with recursion
    // [0,1] for new base will be
    // translated to [p[0], p[0]+1]
    Pair base{ p.first, p.first };
    // all points in the triangle
    // marked by [p[0], p[0]]
    // must be ignored, others must be
    // counted
    auto remaining = i - ((n - 1) * p.first - (p.first - 1) * p.first / 2);
    auto p2 = ::pair(n - p.first, remaining);
    p2.first += base.first, p2.second += base.second;
    return p2;
}

// Implement the same op without using recursion
__device__  Pair fast_pair(const int n, const int p)
{
    // closed form formula from: https://stackoverflow.com/questions/21331385/indexing-the-unordered-pairs-of-a-set
    const int to_square = 2 * n - 1;
    const int square = to_square * to_square;
    // floorf
    //  https://docs.nvidia.com/cuda/cuda-math-api/group__CUDA__MATH__SINGLE.html#group__CUDA__MATH__SINGLE
    const int x = floorf((to_square - sqrt(float(square - 8 * p))) / 2);
    const int y = p - (2 * n - x - 3) * x / 2 + 1;
    return Pair{ x, y };
}


__device__ Pair inc(int n, Pair couple) {
    if (++couple.second == n) {
        couple.first += 1;
        couple.second = couple.first + 1;
        return couple;
    }
    return couple;
}


__global__ void addKernel(int *c, const int *a, const int *b)
{
    int i = threadIdx.x;
    c[i] = a[i] + b[i];
}

// Return a vector that will hold partial results
template <typename T>
auto allocate_partial_container(int ts_count) {
    thrust::device_vector<PCC_Partial<T>> ans(COUPLE_NUMBER(ts_count));
    cudaMemset(thrust::raw_pointer_cast(&ans[0]), 0, sizeof(PCC_Partial<T>) * ans.size());
    return ans;
}

template <typename T>
auto allocate_result_container(int ts_count) {
    thrust::device_vector<T> ans(COUPLE_NUMBER(ts_count));
    cudaMemset(thrust::raw_pointer_cast(&ans[0]), 0, sizeof(T) * ans.size());
    return ans;
}

template <typename T>
__device__ void operator+=(PCC_Partial<T>& p1, const PCC_Partial<T>& p2) {
    p1.count += p2.count;
    p1.sum_1 += p2.sum_1;
    p1.sum_2 += p2.sum_2;
    p1.sum_1_squared += p2.sum_1_squared;
    p1.sum_2_squared += p2.sum_2_squared;
    p1.sum_prod += p2.sum_prod;
}

// comput partial pcc on two time series
template <typename T>
__device__ void calculate_pcc(PCC_Partial<T>* partial, const T* v1, const T* v2, int length) {
    PCC_Partial<T> ans{};
    for (int i{}; i != length; ++i) {
        const auto v_1 = v1[i];
        const auto v_2 = v2[i];
        ans.sum_1 += v_1;
        ans.sum_2 += v_2;
        ans.sum_1_squared += v_1 * v_1;
        ans.sum_2_squared += v_2 * v_2;
        ans.sum_prod += v_1 * v_2;
    }
    ans.count = length;
    *partial += ans;
}

template <typename T>
__global__ void evaluate(PCC_Partial<T>* partial, T** chunk, int length, int lines) {
    int limit = COUPLE_NUMBER(length);
    // to linearize blocks
    auto id = threadIdx.x + blockDim.x * blockIdx.x;
    auto poolsz = (gridDim.x ? gridDim.x : 1) * blockDim.x;
    // columns per thread
    auto cpt = limit / poolsz + (limit % poolsz != 0);
    // more thread than items? Might happet if columns are too few
    if (cpt == 0) {
        if (id < limit) {
            auto i = id;
            //auto couple = ::pair(length, i);
            auto couple = ::fast_pair(length, i);
            calculate_pcc(&partial[i], chunk[couple.first], chunk[couple.second], lines);
        }
    }
    // else at least one columnt per thread
    else {
        // for each pair assigned to this thread
        auto beginning = cpt * id;
        auto end = MIN(beginning + cpt, limit);
        //auto couple = ::pair(length, beginning);
        auto couple = ::fast_pair(length, beginning);
        while (beginning < end) {
            calculate_pcc(&partial[beginning], chunk[couple.first], chunk[couple.second], lines);
            // next pair
            couple = ::inc(length, couple);
            ++beginning;
        }
    }
}

template <typename T>
__device__ T compute(const PCC_Partial<T>& pcc) {
    if (pcc.count == 0) {
        return 0;
    }
    const auto num = (pcc.sum_prod - (pcc.sum_1 * pcc.sum_2) / pcc.count);
    const auto den = (pcc.sum_1_squared - (pcc.sum_1 * pcc.sum_1 / pcc.count)) * (pcc.sum_2_squared - (pcc.sum_2 * pcc.sum_2 / pcc.count));
    // check for div by 0
    return den ? num / sqrt(den) : 0;
}

// calculate results element by element
template <typename T>
__global__ void compute_results(int n, T* res, const PCC_Partial<T>* partials) {
    int limit = COUPLE_NUMBER(n);
    // pairs of columns per thread
    auto cpt = limit / blockDim.x + (limit % blockDim.x != 0);
    // more thread than items? Might happet if columns are too few
    if (cpt == 0) {
        auto i = threadIdx.x;
        if (i < limit) {
            res[i] = compute(partials[i]);
        }
    }
    // else at least one columnt per thread
    else {
        // for each pair assigned to this thread
        const auto beginning = cpt * threadIdx.x;
        const auto end = MIN(beginning + cpt, limit);
        // compute final result
        for (int i = beginning; i < end; ++i) {
            res[i] = compute(partials[i]);
        }
    }
}

void print_results(const int n, const thrust::host_vector<T>& hres) {
    int p = 0;
    for (int i{}; i != n-1; ++i) {
        for (int j{ i + 1 }; j != n; ++j) {
            std::cout << "(" << i << "," << j << ") " << hres[p++] << '\n';
        }
    }
}

template <typename T>
std::vector<thrust::device_vector<T>>& get_chunk(csv::reader& r, std::size_t line_count) {
    static auto column_count = r.column_count();
    static std::vector<thrust::host_vector<T>> tmp(column_count);
    // many columns as tmp
    static auto ans = std::vector<thrust::device_vector<T>>(column_count);
    static bool first = true;
    if (first) {
        // execute once
        first = false;
        for (auto& v : tmp) {
            // allocate space for line count rows on CPU memory
            v.resize(line_count);
        }
        // allocate space for line count rows on GPU memory
        auto& ans_v = ans;
        for (auto& v : ans_v) {
            // allocate space for line count rows on CPU memory
            v.resize(line_count);
        }
    }
    auto inserted_rows = 0;
    while (r.can_read() && inserted_rows != line_count) {
        auto line = r.getline();
        for (size_t i = 0; i < column_count; ++i) {
            tmp[i][inserted_rows] = math::convertions::ston<T>(line.data()[i]);
        }
        ++inserted_rows;
    }
    if (inserted_rows == 0) {
        ans.clear();
        return ans;
    }
    // if found less rows than expeceted, shrink vector
    else if (inserted_rows != line_count) {
        for (auto& v : tmp) {
            v.resize(inserted_rows);
        }
    }
    // copy data to GPU
    {
        auto& ans_v = ans;
        for (decltype(column_count) i{}; i != column_count; ++i) {
            ans_v[i] = tmp[i];
        }
    }
    return ans;
}

template <typename T>
void print(const PCC_Partial<T>& pcc) {
    std::cout
        << "count:         " << pcc.count << '\n'
        << "sum_1:         " << pcc.sum_1 << '\n'
        << "sum_1_squared: " << pcc.sum_1_squared << '\n'
        << "sum_2:         " << pcc.sum_2 << '\n'
        << "sum_2_squared: " << pcc.sum_2_squared << '\n'
        << "sum_prod:      " << pcc.sum_prod << '\n';
}

template <typename T>
thrust::host_vector<T> cuda_evaluate(csv::reader& r, std::size_t chunk_size)
{
    auto ts_count = static_cast<int>(r.column_count());

    // since this allocation the map containing
    // the results will never change its size,
    // so it is safe to access distinct elements
    // from different thread because no ops will
    // be performed on its structure
    auto partial = allocate_partial_container<T>(ts_count);

    while (true) {
        auto& chunk = get_chunk<T>(r, chunk_size);
        if (chunk.empty()) {
            break;
        }
        thrust::device_vector<T*> c;
        c.reserve(chunk.size());
        for (auto& v : chunk) {
            c.push_back(thrust::raw_pointer_cast(&v[0]));
        }
        evaluate<<<16,1024>>>(thrust::raw_pointer_cast(&partial[0]), thrust::raw_pointer_cast(&c[0]), ts_count, chunk[0].size());
    }
    //print(partial[0]);
    auto res = allocate_result_container<T>(ts_count);
    compute_results<<<16,1024>>>(ts_count, thrust::raw_pointer_cast(&res[0]), thrust::raw_pointer_cast(&partial[0]));
	thrust::host_vector<T> hres(res);
    return hres;
}


std::vector<double> cuda_evaluate_double(csv::reader& r, std::size_t chunk_size) {
	thrust::host_vector<double> hv = cuda_evaluate<double>(r, chunk_size);
	return std::vector<double>(hv.begin(), hv.end());
}

std::vector<float> cuda_evaluate_float(csv::reader& r, std::size_t chunk_size) {
	thrust::host_vector<float> hv = cuda_evaluate<double>(r, chunk_size);
	return std::vector<float>(hv.begin(), hv.end());
}
