import sys
import math
import matplotlib.pyplot as plt

def read_access_counts(file_path):
    access_counts = []
    with open(file_path, 'r') as file:
        for line in file:
            access_counts.append(int(line.strip()))
    return access_counts

def calculate_entropy(access_counts):
    total_count = sum(access_counts)
    probabilities = [count / total_count for count in access_counts]
    entropy = -sum(p * math.log2(p) for p in probabilities if p > 0)
    normalized_entropy = entropy / math.log2(len(access_counts))
    return normalized_entropy

def plot_access_patterns(naive_counts, gshare_counts):
    plt.figure(figsize=(10, 6))
    plt.plot(naive_counts, label='Naive')
    plt.plot(gshare_counts, label='Gshare')
    plt.xlabel('State Table Index')
    plt.ylabel('Access Count')
    plt.title('State Table Access Patterns')
    plt.legend()

    # Calculate entropy for naive and Gshare indexing
    naive_entropy = calculate_entropy(naive_counts)
    gshare_entropy = calculate_entropy(gshare_counts)

    # Add entropy values to the plot
    print(f"Naive Entropy: {naive_entropy:.2f}")
    print(f"Gshare Entropy: {gshare_entropy:.2f}")
    # plt.text(0.95, 0.95, f"Naive Entropy: {naive_entropy:.2f}", transform=plt.gca().transAxes, ha='le', va='top')
    # plt.text(0.95, 0.90, f"Gshare Entropy: {gshare_entropy:.2f}", transform=plt.gca().transAxes, ha='right', va='top')

    plt.savefig("here.png")

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python plot_access_patterns.py <naive_access_count_file> <gshare_access_count_file>")
        sys.exit(1)

    naive_file = sys.argv[1]
    gshare_file = sys.argv[2]

    naive_counts = read_access_counts(naive_file)
    gshare_counts = read_access_counts(gshare_file)

    plot_access_patterns(naive_counts, gshare_counts)