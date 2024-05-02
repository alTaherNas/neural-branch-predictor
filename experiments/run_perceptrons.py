import os
import csv
import multiprocessing
import subprocess
import argparse
import time

# Fixed parameter values
FIXED_NUM_PERCEPTRONS = 1024
FIXED_GHR_LENGTH = 32
FIXED_LHR_LENGTH = 16
FIXED_LHT_SIZE = 4096
FIXED_HASHING_SCHEME = 3

num_perceptrons_values = [32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768]
ghr_lengths = [8, 16, 32, 64]
lhr_lengths = [8, 16, 32, 64]
lht_sizes = [256, 512, 1024, 2048, 4096, 8192, 16384, 32768]
hashing_schemes = [1, 2, 3, 4, 5, 6, 7, 8, 9]

def run_predictor(num_perceptrons, ghr_length, lhr_length, lht_size, hashing_scheme, benchmark, benchmark_args, results_file):
    process_name = multiprocessing.current_process().name
    process_id = multiprocessing.current_process().pid
    print(f"Process {process_name} (PID: {process_id}) - Running experiment with parameters:")
    print(f"  NUM_PERCEPTRONS: {num_perceptrons}")
    print(f"  GHR_LENGTH: {ghr_length}")
    print(f"  LHR_LENGTH: {lhr_length}")
    print(f"  LHT_SIZE: {lht_size}")
    print(f"  HASHING_SCHEME: {hashing_scheme}")
    print(f"  Benchmark: {benchmark}")
    
    pin_root = os.environ['PIN_ROOT']
    pin_tool = os.path.join(pin_root, 'pin')
    predictor_so = '../perceptron/obj-intel64/main.so'
    output_file = f'../results/runs/perceptron_results_{num_perceptrons}_{ghr_length}_{lhr_length}_{lht_size}_{hashing_scheme}_{os.path.basename(benchmark)}.out'
    
    cmd = [pin_tool, '-t', predictor_so,
           '-o', output_file,
           '-p', str(num_perceptrons),
           '-g', str(ghr_length),
           '-l', str(lhr_length),
           '-s', str(lht_size),
           '-x', str(hashing_scheme),
           '--', benchmark] + benchmark_args
    
    # Run the subprocess with stdout and stderr redirected to DEVNULL
    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    
    # Wait for the output file to be created
    max_attempts = 10
    attempt = 0
    while not os.path.exists(output_file) and attempt < max_attempts:
        time.sleep(1)
        attempt += 1

    if attempt == max_attempts:
        raise ValueError(f"Output file not created after {max_attempts} attempts: {output_file}")

    # Wait for the output file to be fully written
    max_attempts = 10
    attempt = 0
    file_size = -1
    while attempt < max_attempts:
        new_file_size = os.path.getsize(output_file)
        if new_file_size == file_size:
            break
        file_size = new_file_size
        time.sleep(1)
        attempt += 1

    if attempt == max_attempts:
        raise ValueError(f"Output file size did not stabilize after {max_attempts} attempts: {output_file}")

    # Read the output file and extract the accuracy
    with open(output_file, 'r') as file:
        lines = file.readlines()
        for line in lines:
            if line.startswith("Accuracy:"):
                accuracy = float(line.split(":")[1].strip().rstrip('%'))
                break
        else:
            raise ValueError(f"Accuracy not found in the output file: {output_file}")

    # Write the result to the CSV file
    with open(results_file, 'a', newline='') as file:
        writer = csv.writer(file)
        writer.writerow([os.path.basename(benchmark), num_perceptrons, ghr_length, lhr_length, lht_size, hashing_scheme, accuracy])

    return num_perceptrons, ghr_length, lhr_length, lht_size, hashing_scheme, accuracy

def run_experiments(benchmark, benchmark_args, num_processes, results_file):
    
    parameters = []
    
    # Read existing results from the CSV file
    existing_results = set()
    if os.path.exists(results_file):
        with open(results_file, 'r') as file:
            reader = csv.reader(file)
            next(reader)  # Skip the header row
            for row in reader:
                existing_results.add(tuple(map(int, row[1:6])))  # Convert parameters to integers and add to set
    
    # Sweep over NUM_PERCEPTRONS
    for num_perceptrons in num_perceptrons_values:
        if (num_perceptrons, FIXED_GHR_LENGTH, FIXED_LHR_LENGTH, FIXED_LHT_SIZE, FIXED_HASHING_SCHEME) not in existing_results:
            parameters.append((num_perceptrons, FIXED_GHR_LENGTH, FIXED_LHR_LENGTH, FIXED_LHT_SIZE, FIXED_HASHING_SCHEME, benchmark, benchmark_args, results_file))
    
    # Sweep over GHR_LENGTH
    for ghr_length in ghr_lengths:
        if (FIXED_NUM_PERCEPTRONS, ghr_length, FIXED_LHR_LENGTH, FIXED_LHT_SIZE, FIXED_HASHING_SCHEME) not in existing_results:
            parameters.append((FIXED_NUM_PERCEPTRONS, ghr_length, FIXED_LHR_LENGTH, FIXED_LHT_SIZE, FIXED_HASHING_SCHEME, benchmark, benchmark_args, results_file))
    
    # Sweep over LHR_LENGTH
    for lhr_length in lhr_lengths:
        if (FIXED_NUM_PERCEPTRONS, FIXED_GHR_LENGTH, lhr_length, FIXED_LHT_SIZE, FIXED_HASHING_SCHEME) not in existing_results:
            parameters.append((FIXED_NUM_PERCEPTRONS, FIXED_GHR_LENGTH, lhr_length, FIXED_LHT_SIZE, FIXED_HASHING_SCHEME, benchmark, benchmark_args, results_file))
    
    # Sweep over LHT_SIZE
    for lht_size in lht_sizes:
        if (FIXED_NUM_PERCEPTRONS, FIXED_GHR_LENGTH, FIXED_LHR_LENGTH, lht_size, FIXED_HASHING_SCHEME) not in existing_results:
            parameters.append((FIXED_NUM_PERCEPTRONS, FIXED_GHR_LENGTH, FIXED_LHR_LENGTH, lht_size, FIXED_HASHING_SCHEME, benchmark, benchmark_args, results_file))
    
    # Sweep over HASHING_SCHEME
    for hashing_scheme in hashing_schemes:
        if (FIXED_NUM_PERCEPTRONS, FIXED_GHR_LENGTH, FIXED_LHR_LENGTH, FIXED_LHT_SIZE, hashing_scheme) not in existing_results:
            parameters.append((FIXED_NUM_PERCEPTRONS, FIXED_GHR_LENGTH, FIXED_LHR_LENGTH, FIXED_LHT_SIZE, hashing_scheme, benchmark, benchmark_args, results_file))
    
    with multiprocessing.Pool(num_processes) as pool:
        results = pool.starmap(run_predictor, parameters)
    
    return results

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Run perceptron predictor experiments')
    parser.add_argument('benchmark', type=str, help='Path to the benchmark executable')
    parser.add_argument('benchmark_args', nargs='*', help='Arguments for the benchmark')
    parser.add_argument('--num_processes', type=int, default=multiprocessing.cpu_count(), help='Number of processes to use (default: number of CPU cores)')
    args = parser.parse_args()
    
    benchmark = args.benchmark
    benchmark_args = args.benchmark_args
    num_processes = args.num_processes
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    results_dir = os.path.join(script_dir, '..', 'results')
    os.makedirs(results_dir, exist_ok=True)
    
    benchmark_name = os.path.splitext(os.path.basename(benchmark))[0]
    output_path = os.path.join(results_dir, f'{benchmark_name}_perceptron_results.csv')
    
    # Write the header row if the file doesn't exist
    if not os.path.exists(output_path):
        with open(output_path, 'w', newline='') as file:
            writer = csv.writer(file)
            writer.writerow(['Benchmark', 'NUM_PERCEPTRONS', 'GHR_LENGTH', 'LHR_LENGTH', 'LHT_SIZE', 'HASHING_SCHEME', 'Accuracy'])
    
    start_time = time.time()
    results = run_experiments(benchmark, benchmark_args, num_processes, output_path)
    end_time = time.time()
    
    execution_time = end_time - start_time
    print(f"\nExperiments completed in {execution_time:.2f} seconds")
    print(f"\nResults saved to: {output_path}")