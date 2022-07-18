#include "util.h"
#include "CorrelationCalculator.h"

[[noreturn]] void help(const std::string &cmd, int exit_status = EXIT_FAILURE) {
  std::cerr << "Usage:\n";
  std::cerr << '\t' << cmd << "mode input.csv [chunk_size]\n";
  std::cerr << "\t\t" << "input.csv: file containing time series\n";
  std::cerr << "\t\t" << "chunk_size: how many lines per chunk\n";
  exit(exit_status);
}

int main(int argc, char const *argv[]) {
  try {

    std::size_t chunk_size = 500;
    if (argc < 2 || (argc >= 2 && argv[1] == "--help"s)) {
      help(argv[0], !(argc >= 2 && argv[1] == "--help"s));
    }

    if (argc >= 4) {
      long tmp = std::stol(argv[3]);
      if (tmp <= 0) {
        throw std::runtime_error{
            "chunk_size must be a positive integer, found '"s + argv[2] + "'"s};
      }
      chunk_size = tmp;

    }

    std::string input{argv[2]};
    std::ifstream fin{input};
    if (!fin.good()) {
      throw std::runtime_error{"Failed to open file '"s + input + "'"s};
    }

    auto r = std::make_unique<csv::reader>(fin);
    std::string mode = argv[1];
    auto fp_mode = CorrelationCalculator::kUseDouble;
    auto cc = CorrelationCalculator::create_or_throw(mode, chunk_size, std::move(r));
    auto rv = cc->evaluate_all();
    print_results(rv);

  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    help(argv[0], EXIT_FAILURE);
  }
  return 0;
}

