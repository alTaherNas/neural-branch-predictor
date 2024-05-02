import pandas as pd
import matplotlib.pyplot as plt
import os
import argparse

# Fixed parameter values
FIXED_NUM_PERCEPTRONS = 1024
FIXED_GHR_LENGTH = 32
FIXED_LHR_LENGTH = 16
FIXED_LHT_SIZE = 4096
FIXED_HASHING_SCHEME = 3

def plot_perceptrons(csv_file):
    # Load data from CSV file
    data = pd.read_csv(csv_file)

    # Get the benchmark name from the CSV file name
    benchmark_name = os.path.splitext(os.path.basename(csv_file))[0].split("_")[0]

    # Plot accuracy vs. NUM_PERCEPTRONS
    subset = data[(data['GHR_LENGTH'] == FIXED_GHR_LENGTH) &
                  (data['LHR_LENGTH'] == FIXED_LHR_LENGTH) &
                  (data['LHT_SIZE'] == FIXED_LHT_SIZE) &
                  (data['HASHING_SCHEME'] == FIXED_HASHING_SCHEME)]
    subset = subset.sort_values('NUM_PERCEPTRONS')  # Sort by NUM_PERCEPTRONS
    fig, ax = plt.subplots(figsize=(8, 6))
    ax.plot(subset['NUM_PERCEPTRONS'], subset['Accuracy'], marker='o')
    ax.set_xscale('log', base=2)  # Set logarithmic x-axis with base 2
    ax.set_title(f"{benchmark_name}: Accuracy vs. NUM_PERCEPTRONS (GHR_LENGTH={FIXED_GHR_LENGTH}, LHR_LENGTH={FIXED_LHR_LENGTH}, LHT_SIZE={FIXED_LHT_SIZE}, HASHING_SCHEME={FIXED_HASHING_SCHEME})")
    ax.set_xlabel("NUM_PERCEPTRONS")
    ax.set_ylabel("Accuracy (%)")
    plt.savefig(f"{benchmark_name}_accuracy_vs_num_perceptrons_ghr{FIXED_GHR_LENGTH}_lhr{FIXED_LHR_LENGTH}_lht{FIXED_LHT_SIZE}_hash{FIXED_HASHING_SCHEME}.png", bbox_inches="tight")
    plt.close()

    # Plot accuracy vs. GHR_LENGTH
    subset = data[(data['NUM_PERCEPTRONS'] == FIXED_NUM_PERCEPTRONS) &
                  (data['LHR_LENGTH'] == FIXED_LHR_LENGTH) &
                  (data['LHT_SIZE'] == FIXED_LHT_SIZE) &
                  (data['HASHING_SCHEME'] == FIXED_HASHING_SCHEME)]
    subset = subset.sort_values('GHR_LENGTH')  # Sort by GHR_LENGTH
    fig, ax = plt.subplots(figsize=(8, 6))
    ax.plot(subset['GHR_LENGTH'], subset['Accuracy'], marker='o')
    ax.set_title(f"{benchmark_name}: Accuracy vs. GHR_LENGTH (NUM_PERCEPTRONS={FIXED_NUM_PERCEPTRONS}, LHR_LENGTH={FIXED_LHR_LENGTH}, LHT_SIZE={FIXED_LHT_SIZE}, HASHING_SCHEME={FIXED_HASHING_SCHEME})")
    ax.set_xlabel("GHR_LENGTH")
    ax.set_ylabel("Accuracy (%)")
    plt.savefig(f"{benchmark_name}_accuracy_vs_ghr_length_perceptrons{FIXED_NUM_PERCEPTRONS}_lhr{FIXED_LHR_LENGTH}_lht{FIXED_LHT_SIZE}_hash{FIXED_HASHING_SCHEME}.png", bbox_inches="tight")
    plt.close()

    # Plot accuracy vs. LHR_LENGTH
    subset = data[(data['NUM_PERCEPTRONS'] == FIXED_NUM_PERCEPTRONS) &
                  (data['GHR_LENGTH'] == FIXED_GHR_LENGTH) &
                  (data['LHT_SIZE'] == FIXED_LHT_SIZE) &
                  (data['HASHING_SCHEME'] == FIXED_HASHING_SCHEME)]
    subset = subset.sort_values('LHR_LENGTH')  # Sort by LHR_LENGTH
    fig, ax = plt.subplots(figsize=(8, 6))
    ax.plot(subset['LHR_LENGTH'], subset['Accuracy'], marker='o')
    ax.set_title(f"{benchmark_name}: Accuracy vs. LHR_LENGTH (NUM_PERCEPTRONS={FIXED_NUM_PERCEPTRONS}, GHR_LENGTH={FIXED_GHR_LENGTH}, LHT_SIZE={FIXED_LHT_SIZE}, HASHING_SCHEME={FIXED_HASHING_SCHEME})")
    ax.set_xlabel("LHR_LENGTH")
    ax.set_ylabel("Accuracy (%)")
    plt.savefig(f"{benchmark_name}_accuracy_vs_lhr_length_perceptrons{FIXED_NUM_PERCEPTRONS}_ghr{FIXED_GHR_LENGTH}_lht{FIXED_LHT_SIZE}_hash{FIXED_HASHING_SCHEME}.png", bbox_inches="tight")
    plt.close()

    # Plot accuracy vs. LHT_SIZE
    subset = data[(data['NUM_PERCEPTRONS'] == FIXED_NUM_PERCEPTRONS) &
                  (data['GHR_LENGTH'] == FIXED_GHR_LENGTH) &
                  (data['LHR_LENGTH'] == FIXED_LHR_LENGTH) &
                  (data['HASHING_SCHEME'] == FIXED_HASHING_SCHEME)]
    subset = subset.sort_values('LHT_SIZE')  # Sort by LHT_SIZE
    fig, ax = plt.subplots(figsize=(8, 6))
    ax.plot(subset['LHT_SIZE'], subset['Accuracy'], marker='o')
    ax.set_xscale('log', base=2)  # Set logarithmic x-axis with base 2
    ax.set_title(f"{benchmark_name}: Accuracy vs. LHT_SIZE (NUM_PERCEPTRONS={FIXED_NUM_PERCEPTRONS}, GHR_LENGTH={FIXED_GHR_LENGTH}, LHR_LENGTH={FIXED_LHR_LENGTH}, HASHING_SCHEME={FIXED_HASHING_SCHEME})")
    ax.set_xlabel("LHT_SIZE")
    ax.set_ylabel("Accuracy (%)")
    plt.savefig(f"{benchmark_name}_accuracy_vs_lht_size_perceptrons{FIXED_NUM_PERCEPTRONS}_ghr{FIXED_GHR_LENGTH}_lhr{FIXED_LHR_LENGTH}_hash{FIXED_HASHING_SCHEME}.png", bbox_inches="tight")
    plt.close()

    # Plot accuracy vs. HASHING_SCHEME
    subset = data[(data['NUM_PERCEPTRONS'] == FIXED_NUM_PERCEPTRONS) &
                  (data['GHR_LENGTH'] == FIXED_GHR_LENGTH) &
                  (data['LHR_LENGTH'] == FIXED_LHR_LENGTH) &
                  (data['LHT_SIZE'] == FIXED_LHT_SIZE)]
    fig, ax = plt.subplots(figsize=(8, 6))
    ax.bar(subset['HASHING_SCHEME'], subset['Accuracy'])
    ax.set_title(f"{benchmark_name}: Accuracy vs. HASHING_SCHEME (NUM_PERCEPTRONS={FIXED_NUM_PERCEPTRONS}, GHR_LENGTH={FIXED_GHR_LENGTH}, LHR_LENGTH={FIXED_LHR_LENGTH}, LHT_SIZE={FIXED_LHT_SIZE})")
    ax.set_xlabel("HASHING_SCHEME")
    ax.set_ylabel("Accuracy (%)")
    plt.savefig(f"{benchmark_name}_accuracy_vs_hashing_scheme_perceptrons{FIXED_NUM_PERCEPTRONS}_ghr{FIXED_GHR_LENGTH}_lhr{FIXED_LHR_LENGTH}_lht{FIXED_LHT_SIZE}_hash{FIXED_HASHING_SCHEME}.png", bbox_inches="tight")
    plt.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Plot perceptron predictor accuracy")
    parser.add_argument("csv_file", help="Path to the CSV file containing perceptron results")
    args = parser.parse_args()

    plot_perceptrons(args.csv_file)