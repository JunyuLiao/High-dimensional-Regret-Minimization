import subprocess
import re
import sys
from tqdm import tqdm
from collections import defaultdict

# if len(sys.argv) != 7:
#     print("Usage: python script.py <dataset_path> <d_prime> <d_hat_1> <d_hat_2> <K> <num_q>")
#     sys.exit(1)

dataset_path = "datasets/e100-10k.txt"
d_prime = 3
d_hat_1 = 8
d_hat_2 = 7
K = 30
num_q = 150

# Parameters and command
command = ["./run", dataset_path, str(d_prime), str(d_hat_1), str(d_hat_2), str(K), str(num_q)]

# Metrics containers
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
for _ in tqdm(range(10), desc="Running experiments"):
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    
    if result.returncode != 0:
        continue

    output = result.stdout.lower()
    print(output)  # For debugging

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

# Helper
def mean(lst):
    return sum(lst) / len(lst) if lst else float('nan')

# Report
print("\n===== Final Results =====")

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
