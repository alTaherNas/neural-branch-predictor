# Perceptron-Based Branch Predictor

This project explores the design space of perceptron-based branch predictors using the Pin tool and benchmarks from the SPEC suite. The goal is to investigate various parameters and configurations to optimize the performance of perceptron-based branch predictors.

## Table of Contents
- [Project Structure](#project-structure)
- [Prerequisites](#prerequisites)
- [Usage](#usage)
  - [Adding Benchmarks](#adding-benchmarks)
  - [Running Perceptron Experiments](#running-perceptron-experiments)
  - [Plotting Perceptron Results](#plotting-perceptron-results)
- [Hashing Schemes](#hashing-schemes)
- [Benchmark Results](#benchmark-results)
  - [Number of Perceptrons](#number-of-perceptrons)
  - [Global History Register Length](#global-history-register-length)
  - [Hashing Scheme Comparison](#hashing-scheme-comparison)
  - [Local History Table Size](#local-history-table-size)
- [Confidence Estimation](#confidence-estimation)
- [Access Patterns and Hashing](#access-patterns-and-hashing)
- [Acknowledgments](#acknowledgments)

## Project Structure

```
.
├── README.md
├── benchmarks
│   └── ...
├── experiments
│   └── run_perceptrons.py
├── perceptron
│   ├── main.cpp
│   ├── makefile
│   └── makefile.rules
└── results
    ├── plot_perceptrons.py
    ├── plot_confidence.py
    └── runs
```

## Prerequisites

To run the experiments and simulations, you need to have the following installed:
- Pin tool
- Python 3.x
- C++ compiler

## Usage

### Adding Benchmarks

To use your own benchmarks, place the benchmark executables in the `benchmarks` directory. Make sure to have the necessary permissions to execute the benchmarks.

### Running Perceptron Experiments

To run the perceptron experiments, navigate to the `experiments` directory and run the following command:

```bash
python run_perceptrons.py <benchmark> [<benchmark_args>]
```

Replace `<benchmark>` with the path to the benchmark executable and `<benchmark_args>` with any additional arguments required by the benchmark.

For example:
```bash
python run_perceptrons.py ../benchmarks/your_benchmark 
```

The script will run the perceptron predictor with various parameter configurations and save the results in the `results` directory.

### Plotting Perceptron Results

To plot the results of the perceptron experiments, navigate to the `results` directory and run the following command:

```bash
python plot_perceptrons.py <results_file>
```

Replace `<results_file>` with the path to the CSV file containing the experiment results.

For example:
```bash
python plot_perceptrons.py your_benchmark_perceptron_results.csv
```

The script will generate plots visualizing the impact of different parameters on the prediction accuracy.

## Hashing Schemes

The project explores various hashing schemes for perceptron table indexing. The following hashing schemes are implemented:

| Scheme | Hashing Formula                                                 |
| ------ | --------------------------------------------------------------- |
| 1      | perceptron_index = pc % num_perceptrons                         |
| 2      | perceptron_index = (pc ^ GHR) % num_perceptrons                 |
| 3      | perceptron_index = local_history % num_perceptrons              |
| 4      | perceptron_index = pc % num_perceptrons                         |
| 5      | perceptron_index = GHR % num_perceptrons                        |
| 6      | perceptron_index = (pc ^ local_history) % num_perceptrons       |
| 7      | perceptron_index = (pc ^ GHR) % num_perceptrons                 |
| 8      | perceptron_index = (local_history ^ GHR) % num_perceptrons      |
| 9      | perceptron_index = (pc ^ local_history ^ GHR) % num_perceptrons |

## Benchmark Results

The following sections present some of the key findings from the experiments conducted on various benchmarks.

### Number of Perceptrons

Increasing the number of perceptrons consistently improves the prediction accuracy across different benchmarks. This suggests that a larger number of perceptrons enhances the predictor's ability to capture complex branch patterns.

![Number of Perceptrons](dealII_accuracy_vs_num_perceptrons_ghr32_lhr16_lht4096_hash3.png)

### Global History Register Length

Longer global history register lengths initially improve accuracy but may lead to a decrease in accuracy beyond a certain threshold for both benchmarks. This behavior indicates that capturing too much history can introduce noise and hinder the predictor's ability to adapt to changing branch patterns. Finding the right balance between history length and accuracy is crucial for optimal predictor performance across different workloads.

![Global History Register Length](libquantum_accuracy_vs_ghr_length_perceptrons1024_lhr16_lht4096_hash3.png)

### Hashing Scheme Comparison

The choice of hashing scheme significantly affects the prediction accuracy. Among the explored hashing schemes, hashing scheme 3 achieves the best accuracy for both the dealII and libquantum benchmarks. This highlights the importance of selecting an appropriate hashing function that minimizes aliasing and maximizes table utilization, and the effectiveness of this hashing scheme across different benchmarks.

![Hashing Scheme Comparison](libquantum_accuracy_vs_hashing_scheme_perceptrons1024_ghr32_lhr16_lht4096_hash3.png)

### Local History Table Size

The following image shows the impact of local history table size on the prediction accuracy for the libquantum benchmark:

![Local History Table Size](libquantum_accuracy_vs_lht_size_perceptrons1024_ghr32_lhr16_hash3.png)

The plot demonstrates that increasing the local history table size improves the prediction accuracy up to a certain point, beyond which the accuracy stabilizes or slightly decreases.

## Confidence Estimation

Confidence estimation is an important aspect of perceptron-based branch predictors. It provides a measure of the predictor's confidence in its predictions. The project includes a script `plot_confidence.py` to visualize the confidence estimation using perceptron output magnitude and saturation counters.

To plot the confidence estimation results, navigate to the `results` directory and run the following command:

```bash
python plot_confidence.py <confidence_data_file>
```

Replace `<confidence_data_file>` with the path to the CSV file containing the confidence estimation data.

The script will generate a plot illustrating how the predictor's confidence evolves over time as it learns and adapts to the branch behavior.

## Access Patterns and Hashing

Hashing can be used to mitigate the impact of aliasing by increasing the entropy and improving the utilization of the tables. The following image compares the access patterns of a naive predictor and a hashed predictor:

![Access Patterns](access_pattern.png)

The hashed predictor exhibits a more spread-out access pattern, indicating better table utilization and reduced aliasing.

## Acknowledgments

This project was done as part of a project for Harvard University's CS 246 Advanced Computer Architecture course.