#define BOOST_TEST_MODULE MatrixLayoutBenchmark
#include <boost/test/unit_test.hpp>

#include <chrono>
#include <Eigen/Dense>

BOOST_AUTO_TEST_SUITE(MatrixLayout_Tests)

BOOST_AUTO_TEST_CASE(SameLoop_RowMajor_vs_ColMajor) {
    const int rows = 4096;
    const int cols = 4096;
    const int reps = 50;

    Eigen::MatrixXd colMajor = Eigen::MatrixXd::Random(rows, cols);
    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> rowMajor(colMajor);

    volatile double sum = 0.0;

    sum = 0.0;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int r = 0; r < reps; ++r) {
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                sum += colMajor(i, j);
            }
        }
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double colMajorMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    sum = 0.0;
    t0 = std::chrono::high_resolution_clock::now();
    for (int r = 0; r < reps; ++r) {
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                sum += rowMajor(i, j);
            }
        }
    }
    t1 = std::chrono::high_resolution_clock::now();
    double rowMajorMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    BOOST_TEST_MESSAGE("\n--- Mismo bucle (for i for j), distinto layout ---");
    BOOST_TEST_MESSAGE("ColMajor: " << colMajorMs << " ms");
    BOOST_TEST_MESSAGE("RowMajor: " << rowMajorMs << " ms");
    BOOST_TEST_MESSAGE("Speedup RowMajor / ColMajor: " << (colMajorMs / rowMajorMs) << "x\n");

    BOOST_CHECK_LT(rowMajorMs, colMajorMs);
}

BOOST_AUTO_TEST_SUITE_END()
