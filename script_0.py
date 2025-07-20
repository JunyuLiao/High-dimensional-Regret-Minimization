import subprocess
import re
from tqdm import tqdm
from collections import defaultdict
import matplotlib.pyplot as plt
import numpy as np

# Configuration
dataset_path = "datasets/e100-10k.txt"

# Parameter to vary - change this to experiment with different parameters
VARY_PARAM = "d_prime"  # Options: "d_hat_1", "d_hat_2", "d_prime", "K", "num_q"

# Default parameters (will be overridden by the varied parameter)
default_params = {
    "d_prime": 3,
    "d_hat_1": 8,
    "d_hat_2": 8,
    "K": 30,
    "num_q": 20
}

# Define parameter ranges for each parameter
param_ranges = {
    "d_prime": [2, 3, 4, 5],
    "d_hat_1": [5, 6, 7, 8, 9, 10],
    "d_hat_2": [5, 6, 7, 8, 9, 10],
    "K": [20, 30, 40, 50],
    "num_q": [15, 17, 19, 21, 23, 25, 27, 30]
}

# Number of runs per parameter value
NUM_RUNS = 10

def mean(lst):
    return sum(lst) / len(lst) if lst else float('nan')

def get_command(param_value):
    """Generate command with the specified parameter value"""
    params = default_params.copy()
    params[VARY_PARAM] = param_value
    
    return ["./run", dataset_path, 
            str(params["d_prime"]), 
            str(params["d_hat_1"]), 
            str(params["d_hat_2"]), 
            str(params["K"]), 
            str(params["num_q"])]

def get_plot_title(param_name):
    """Get appropriate plot titles based on parameter"""
    titles = {
        "d_hat_1": "d̂₁",
        "d_hat_2": "d̂₂", 
        "d_prime": "d'",
        "K": "K",
        "num_q": "Number of Questions"
    }
    return titles.get(param_name, param_name)

# Get values to test
param_values = param_ranges[VARY_PARAM]
print(f"Varying parameter: {VARY_PARAM}")
print(f"Testing values: {param_values}")
print(f"Default parameters: {default_params}")

# Store results for all parameter values
all_results = {}

# Iterate over parameter values
for param_value in param_values:
    command = get_command(param_value)
    print(f"\nTesting {VARY_PARAM} = {param_value}")
    print(f"Command: {' '.join(command)}")

    stats = {
        "attr_subset_count": 0,
        "interactive_count": 0,
        "attr_subset": {
            "rounds": [],
            "questions": [],
            "sphere_rr": [],
            "sphere_size": [],
            "attsub_rr": [],
            "attsub_size": [],
            "attsub_wins": 0
        },
        "interactive": {
            "questions": [],
            "methods": defaultdict(list)  # method → list of (questions, point ID)
        }
    }

    # Execution loop with progress bar
    for run_idx in tqdm(range(NUM_RUNS), desc=f"{VARY_PARAM} = {param_value}"):
        try:
            result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=60)
        except subprocess.TimeoutExpired:
            print(f"Run {run_idx + 1} timed out, skipping...")
            continue

        if result.returncode != 0:
            print(f"Run {run_idx + 1} failed with return code {result.returncode}")
            if result.stderr:
                print(f"Error: {result.stderr}")
            continue

        output = result.stdout.lower()

        # Case 1: Attribute Subset Mode
        if "attribute subset" in output:
            stats["attr_subset_count"] += 1

            rounds = re.search(r"number of rounds in attribute subset method:\s*(\d+)", output)
            questions = re.search(r"number of questions asked:\s*(\d+)", output)
            sphere = re.search(r"output size of sphere:\s*(\d+)\s*\|\s*rr of sphere:\s*([0-9.]+)", output)
            attsub = re.search(r"output size of attsub:\s*(\d+)\s*\|\s*rr of attsub:\s*([0-9.]+)", output)

            if rounds and questions and sphere and attsub:
                r = int(rounds.group(1))
                q = int(questions.group(1))
                s_size = int(sphere.group(1))
                s_rr = float(sphere.group(2))
                a_size = int(attsub.group(1))
                a_rr = float(attsub.group(2))

                stats["attr_subset"]["rounds"].append(r)
                stats["attr_subset"]["questions"].append(q)
                stats["attr_subset"]["sphere_size"].append(s_size)
                stats["attr_subset"]["sphere_rr"].append(s_rr)
                stats["attr_subset"]["attsub_size"].append(a_size)
                stats["attr_subset"]["attsub_rr"].append(a_rr)

                if a_rr <= s_rr:
                    stats["attr_subset"]["attsub_wins"] += 1

        # Case 2: Interactive Algorithm Mode
        elif "interactive algorithm" in output:
            stats["interactive_count"] += 1

            questions = re.search(r"number of questions asked:\s*(\d+)", output)
            if questions:
                stats["interactive"]["questions"].append(int(questions.group(1)))

            # Parse method result table entries
            method_entries = re.findall(r"\|\s*([A-Za-z\- ]+)\s*\|\s*(\d+|[-])\s*\|\s*(\d+)", output)
            for method, q_str, pid_str in method_entries:
                try:
                    q = int(q_str) if q_str != "-" else -1
                    pid = int(pid_str)
                    stats["interactive"]["methods"][method.strip()].append((q, pid))
                except ValueError:
                    continue

    # Store results for this parameter value
    all_results[param_value] = stats

    # Report for this parameter value
    print(f"\n===== Results for {VARY_PARAM} = {param_value} =====")

    # Attribute Subset Summary
    asc = stats["attr_subset_count"]
    print(f"Total AttributeSubset runs: {asc}")
    if asc:
        print(f"  Avg rounds: {mean(stats['attr_subset']['rounds']):.2f}")
        print(f"  Avg questions: {mean(stats['attr_subset']['questions']):.2f}")
        print(f"  Sphere:  avg size = {mean(stats['attr_subset']['sphere_size']):.2f}, avg RR = {mean(stats['attr_subset']['sphere_rr']):.6f}")
        print(f"  Attsub: avg size = {mean(stats['attr_subset']['attsub_size']):.2f}, avg RR = {mean(stats['attr_subset']['attsub_rr']):.6f}")
        win_rate = stats["attr_subset"]["attsub_wins"] / asc
        print(f"  Attsub Winning Rate: {win_rate:.2%} ({stats['attr_subset']['attsub_wins']} / {asc})")

    # Interactive Summary
    ic = stats["interactive_count"]
    print(f"\nTotal Interactive runs: {ic}")
    if ic:
        print(f"  Avg questions asked: {mean(stats['interactive']['questions']):.2f}")
        for method, entries in stats["interactive"]["methods"].items():
            if method != "ground truth":
                q_vals = [q for q, _ in entries if q >= 0]
                print(f"  {method}: avg questions = {mean(q_vals):.2f}")

# Create plots
print("\n===== Creating Plots =====")

# Prepare data for plotting
param_list = []
attr_avg_questions = []
attr_avg_rounds = []
attr_sphere_rr = []
attr_attsub_rr = []
attr_win_rates = []
interactive_avg_questions = []

for param_value in param_values:
    if param_value in all_results:
        stats = all_results[param_value]
        param_list.append(param_value)
        
        # Attribute subset metrics
        if stats["attr_subset_count"] > 0:
            attr_avg_questions.append(mean(stats["attr_subset"]["questions"]))
            attr_avg_rounds.append(mean(stats["attr_subset"]["rounds"]))
            attr_sphere_rr.append(mean(stats["attr_subset"]["sphere_rr"]))
            attr_attsub_rr.append(mean(stats["attr_subset"]["attsub_rr"]))
            attr_win_rates.append(stats["attr_subset"]["attsub_wins"] / stats["attr_subset_count"])
        else:
            attr_avg_questions.append(np.nan)
            attr_avg_rounds.append(np.nan)
            attr_sphere_rr.append(np.nan)
            attr_attsub_rr.append(np.nan)
            attr_win_rates.append(np.nan)
        
        # Interactive metrics
        if stats["interactive_count"] > 0:
            interactive_avg_questions.append(mean(stats["interactive"]["questions"]))
        else:
            interactive_avg_questions.append(np.nan)

# Create subplots
param_title = get_plot_title(VARY_PARAM)
fig, axes = plt.subplots(2, 3, figsize=(15, 10))
fig.suptitle(f'Results vs {param_title} Values', fontsize=16)

# Plot 1: Average Questions (Attribute Subset)
axes[0, 0].plot(param_list, attr_avg_questions, 'bo-', linewidth=2, markersize=8)
axes[0, 0].set_title('Attribute Subset: Avg Questions')
axes[0, 0].set_xlabel(param_title)
axes[0, 0].set_ylabel('Average Questions')
axes[0, 0].grid(True, alpha=0.3)

# Plot 2: Average Rounds (Attribute Subset)
axes[0, 1].plot(param_list, attr_avg_rounds, 'go-', linewidth=2, markersize=8)
axes[0, 1].set_title('Attribute Subset: Avg Rounds')
axes[0, 1].set_xlabel(param_title)
axes[0, 1].set_ylabel('Average Rounds')
axes[0, 1].grid(True, alpha=0.3)

# Plot 3: Regret Ratio Comparison
axes[0, 2].plot(param_list, attr_sphere_rr, 'ro-', linewidth=2, markersize=8, label='Sphere RR')
axes[0, 2].plot(param_list, attr_attsub_rr, 'bo-', linewidth=2, markersize=8, label='Attsub RR')
axes[0, 2].set_title('Regret Ratio Comparison')
axes[0, 2].set_xlabel(param_title)
axes[0, 2].set_ylabel('Regret Ratio')
axes[0, 2].legend()
axes[0, 2].grid(True, alpha=0.3)

# Plot 4: Attsub Win Rate
axes[1, 0].plot(param_list, [wr * 100 for wr in attr_win_rates], 'mo-', linewidth=2, markersize=8)
axes[1, 0].set_title('Attsub Win Rate')
axes[1, 0].set_xlabel(param_title)
axes[1, 0].set_ylabel('Win Rate (%)')
axes[1, 0].grid(True, alpha=0.3)

# Plot 5: Interactive Average Questions
axes[1, 1].plot(param_list, interactive_avg_questions, 'co-', linewidth=2, markersize=8)
axes[1, 1].set_title('Interactive: Avg Questions')
axes[1, 1].set_xlabel(param_title)
axes[1, 1].set_ylabel('Average Questions')
axes[1, 1].grid(True, alpha=0.3)

# Plot 6: Summary comparison
axes[1, 2].plot(param_list, attr_avg_questions, 'bo-', linewidth=2, markersize=8, label='Attr Subset')
axes[1, 2].plot(param_list, interactive_avg_questions, 'ro-', linewidth=2, markersize=8, label='Interactive')
axes[1, 2].set_title('Questions: Attr Subset vs Interactive')
axes[1, 2].set_xlabel(param_title)
axes[1, 2].set_ylabel('Average Questions')
axes[1, 2].legend()
axes[1, 2].grid(True, alpha=0.3)

plt.tight_layout()
plot_filename = f'plot/{VARY_PARAM}_results.png'
plt.savefig(plot_filename, dpi=300, bbox_inches='tight')
plt.show()

print(f"Plots saved as '{plot_filename}'")

# Print summary table
print(f"\n===== Summary Table =====")
print(f"{param_title:>7} | Attr Questions | Attr Rounds | Sphere RR  | Attsub RR  | Win Rate | Interactive Q")
print("--------|----------------|-------------|------------|------------|----------|-------------")
for i, param_value in enumerate(param_list):
    print(f"{param_value:7} | {attr_avg_questions[i]:13.2f} | {attr_avg_rounds[i]:10.2f} | {attr_sphere_rr[i]:9.6f} | {attr_attsub_rr[i]:9.6f} | {attr_win_rates[i]*100:7.1f}% | {interactive_avg_questions[i]:11.2f}")