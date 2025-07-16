import subprocess
import re
from tqdm import tqdm
from collections import defaultdict
import matplotlib.pyplot as plt
import numpy as np
import os

# Default parameters
dataset_path = "datasets/e100-10k.txt"
d_prime_values = [2, 3, 4, 5]  # Values to sweep for d_prime
d_hat_1_values = [5, 6, 7, 8, 9, 10]  # Values to sweep
d_hat_2 = 7
K = 30
num_q = 150

def mean(lst):
    return sum(lst) / len(lst) if lst else float('nan')

# Store results for all d_prime and d_hat_1 combinations
all_results = {}

# Iterate over d_prime values first, then d_hat_1 values
for d_prime in d_prime_values:
    all_results[d_prime] = {}
    
    for d_hat_1 in d_hat_1_values:
        command = ["./run", dataset_path, str(d_prime), str(d_hat_1), str(d_hat_2), str(K), str(num_q)]

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
        for _ in tqdm(range(10), desc=f"d_prime = {d_prime}, d_hat_1 = {d_hat_1}"):
            result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

            if result.returncode != 0:
                print(f"Command failed with return code {result.returncode}")
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

        # Store results for this d_prime and d_hat_1 combination
        all_results[d_prime][d_hat_1] = stats

        # Report for this combination
        print(f"\n===== Results for d_prime = {d_prime}, d_hat_1 = {d_hat_1} =====")

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

# Colors and markers for different d_prime values
colors = ['blue', 'red', 'green', 'orange']
markers = ['o', 's', '^', 'D']

# Create directory for individual plots
individual_plots_dir = 'plot/individual_figures'
os.makedirs(individual_plots_dir, exist_ok=True)

# Create individual figures for each plot type
plot_types = [
    ('attr_avg_questions', 'Attribute Subset: Avg Questions', 'Average Questions'),
    ('attr_avg_rounds', 'Attribute Subset: Avg Rounds', 'Average Rounds'),
    ('regret_ratio', 'Regret Ratio Comparison', 'Regret Ratio'),
    ('attr_win_rate', 'Attsub Win Rate', 'Win Rate (%)'),
    ('interactive_questions', 'Interactive: Avg Questions', 'Average Questions'),
    ('comparison', 'Questions: Attr Subset vs Interactive', 'Average Questions')
]

for plot_idx, (plot_name, plot_title, ylabel) in enumerate(plot_types):
    plt.figure(figsize=(10, 6))
    
    for d_prime_idx, d_prime in enumerate(d_prime_values):
        color = colors[d_prime_idx % len(colors)]
        marker = markers[d_prime_idx % len(markers)]
        
        # Prepare data for this d_prime
        d_hat_1_list = []
        attr_avg_questions = []
        attr_avg_rounds = []
        attr_sphere_rr = []
        attr_attsub_rr = []
        attr_win_rates = []
        interactive_avg_questions = []

        for d_hat_1 in d_hat_1_values:
            if d_hat_1 in all_results[d_prime]:
                stats = all_results[d_prime][d_hat_1]
                d_hat_1_list.append(d_hat_1)
                
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

        # Plot based on type
        if plot_name == 'attr_avg_questions':
            plt.plot(d_hat_1_list, attr_avg_questions, color=color, marker=marker,
                    linewidth=2, markersize=8, label=f'd_prime={d_prime}')
        elif plot_name == 'attr_avg_rounds':
            plt.plot(d_hat_1_list, attr_avg_rounds, color=color, marker=marker,
                    linewidth=2, markersize=8, label=f'd_prime={d_prime}')
        elif plot_name == 'regret_ratio':
            plt.plot(d_hat_1_list, attr_sphere_rr, color=color, marker=marker,
                    linewidth=2, markersize=8, linestyle='--', alpha=0.7, 
                    label=f'Sphere (d_prime={d_prime})')
            plt.plot(d_hat_1_list, attr_attsub_rr, color=color, marker=marker,
                    linewidth=2, markersize=8, label=f'Attsub (d_prime={d_prime})')
        elif plot_name == 'attr_win_rate':
            plt.plot(d_hat_1_list, [wr * 100 for wr in attr_win_rates], color=color, marker=marker,
                    linewidth=2, markersize=8, label=f'd_prime={d_prime}')
        elif plot_name == 'interactive_questions':
            plt.plot(d_hat_1_list, interactive_avg_questions, color=color, marker=marker,
                    linewidth=2, markersize=8, label=f'd_prime={d_prime}')
        elif plot_name == 'comparison':
            plt.plot(d_hat_1_list, attr_avg_questions, color=color, marker=marker,
                    linewidth=2, markersize=8, linestyle='-', 
                    label=f'Attr Subset (d_prime={d_prime})')
            plt.plot(d_hat_1_list, interactive_avg_questions, color=color, marker=marker,
                    linewidth=2, markersize=8, linestyle=':', 
                    label=f'Interactive (d_prime={d_prime})')

    plt.title(plot_title, fontsize=14)
    plt.xlabel('d_hat_1')
    plt.ylabel(ylabel)
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    
    # Save individual figure
    filename = f'{plot_idx+1:02d}_{plot_name}.png'
    filepath = os.path.join(individual_plots_dir, filename)
    plt.savefig(filepath, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Saved: {filepath}")

# Also create the combined figure
fig, axes = plt.subplots(2, 3, figsize=(18, 12))
fig.suptitle('Results vs d_hat_1 Values for Different d_prime', fontsize=16)

for d_prime_idx, d_prime in enumerate(d_prime_values):
    color = colors[d_prime_idx % len(colors)]
    marker = markers[d_prime_idx % len(markers)]
    
    # Prepare data for this d_prime
    d_hat_1_list = []
    attr_avg_questions = []
    attr_avg_rounds = []
    attr_sphere_rr = []
    attr_attsub_rr = []
    attr_win_rates = []
    interactive_avg_questions = []

    for d_hat_1 in d_hat_1_values:
        if d_hat_1 in all_results[d_prime]:
            stats = all_results[d_prime][d_hat_1]
            d_hat_1_list.append(d_hat_1)
            
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

    # Plot on each subplot
    axes[0, 0].plot(d_hat_1_list, attr_avg_questions, color=color, marker=marker,
                    linewidth=2, markersize=8, label=f'd_prime={d_prime}')
    axes[0, 1].plot(d_hat_1_list, attr_avg_rounds, color=color, marker=marker,
                    linewidth=2, markersize=8, label=f'd_prime={d_prime}')
    axes[0, 2].plot(d_hat_1_list, attr_sphere_rr, color=color, marker=marker,
                    linewidth=2, markersize=8, linestyle='--', alpha=0.7,
                    label=f'Sphere (d_prime={d_prime})')
    axes[0, 2].plot(d_hat_1_list, attr_attsub_rr, color=color, marker=marker,
                    linewidth=2, markersize=8, label=f'Attsub (d_prime={d_prime})')
    axes[1, 0].plot(d_hat_1_list, [wr * 100 for wr in attr_win_rates], color=color, marker=marker,
                    linewidth=2, markersize=8, label=f'd_prime={d_prime}')
    axes[1, 1].plot(d_hat_1_list, interactive_avg_questions, color=color, marker=marker,
                    linewidth=2, markersize=8, label=f'd_prime={d_prime}')
    axes[1, 2].plot(d_hat_1_list, attr_avg_questions, color=color, marker=marker,
                    linewidth=2, markersize=8, linestyle='-',
                    label=f'Attr Subset (d_prime={d_prime})')
    axes[1, 2].plot(d_hat_1_list, interactive_avg_questions, color=color, marker=marker,
                    linewidth=2, markersize=8, linestyle=':',
                    label=f'Interactive (d_prime={d_prime})')

# Configure subplots
axes[0, 0].set_title('Attribute Subset: Avg Questions')
axes[0, 0].set_xlabel('d_hat_1')
axes[0, 0].set_ylabel('Average Questions')
axes[0, 0].legend()
axes[0, 0].grid(True, alpha=0.3)

axes[0, 1].set_title('Attribute Subset: Avg Rounds')
axes[0, 1].set_xlabel('d_hat_1')
axes[0, 1].set_ylabel('Average Rounds')
axes[0, 1].legend()
axes[0, 1].grid(True, alpha=0.3)

axes[0, 2].set_title('Regret Ratio Comparison')
axes[0, 2].set_xlabel('d_hat_1')
axes[0, 2].set_ylabel('Regret Ratio')
axes[0, 2].legend()
axes[0, 2].grid(True, alpha=0.3)

axes[1, 0].set_title('Attsub Win Rate')
axes[1, 0].set_xlabel('d_hat_1')
axes[1, 0].set_ylabel('Win Rate (%)')
axes[1, 0].legend()
axes[1, 0].grid(True, alpha=0.3)

axes[1, 1].set_title('Interactive: Avg Questions')
axes[1, 1].set_xlabel('d_hat_1')
axes[1, 1].set_ylabel('Average Questions')
axes[1, 1].legend()
axes[1, 1].grid(True, alpha=0.3)

axes[1, 2].set_title('Questions: Attr Subset vs Interactive')
axes[1, 2].set_xlabel('d_hat_1')
axes[1, 2].set_ylabel('Average Questions')
axes[1, 2].legend()
axes[1, 2].grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig('plot/d_prime_d_hat_1_results.png', dpi=300, bbox_inches='tight')
plt.show()

print("Combined plot saved as 'plot/d_prime_d_hat_1_results.png'")
print(f"Individual plots saved in '{individual_plots_dir}/'")

# Print summary tables for each d_prime
for d_prime in d_prime_values:
    print(f"\n===== Summary Table for d_prime = {d_prime} =====")
    print("d_hat_1 | Attr Questions | Attr Rounds | Sphere RR  | Attsub RR  | Win Rate | Interactive Q")
    print("--------|----------------|-------------|------------|------------|----------|-------------")
    
    for d_hat_1 in d_hat_1_values:
        if d_hat_1 in all_results[d_prime]:
            stats = all_results[d_prime][d_hat_1]
            
            if stats["attr_subset_count"] > 0:
                attr_q = mean(stats["attr_subset"]["questions"])
                attr_r = mean(stats["attr_subset"]["rounds"])
                sphere_rr = mean(stats["attr_subset"]["sphere_rr"])
                attsub_rr = mean(stats["attr_subset"]["attsub_rr"])
                win_rate = stats["attr_subset"]["attsub_wins"] / stats["attr_subset_count"]
            else:
                attr_q = attr_r = sphere_rr = attsub_rr = win_rate = float('nan')
            
            if stats["interactive_count"] > 0:
                inter_q = mean(stats["interactive"]["questions"])
            else:
                inter_q = float('nan')
                
            print(f"{d_hat_1:7} | {attr_q:13.2f} | {attr_r:10.2f} | {sphere_rr:9.6f} | {attsub_rr:9.6f} | {win_rate*100:7.1f}% | {inter_q:11.2f}")