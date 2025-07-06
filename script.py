import subprocess
import re
import sys
from tqdm import tqdm  # Install with: pip install tqdm

if len(sys.argv) != 6:
    print("Usage: python script.py <dataset_path> <d_prime> <d_hat_1> <d_hat_2> <rounds>")
    sys.exit(1)

# Command-line arguments
executable = "./run"
args = sys.argv[1:]
command = [executable] + args

# Metrics
sphere_sizes = []
sphere_rrs = []
attsub_sizes = []
attsub_rrs = []
attsub_better_count = 0  # For winning rate

# Run with tqdm progress bar
for _ in tqdm(range(100), desc="Running experiments"):
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    if result.returncode != 0:
        continue

    output = result.stdout

    # Extract metrics
    sphere_match = re.search(r"output size of Sphere:\s*(\d+)\s*\|\s*RR of Sphere:\s*([0-9.]+)", output)
    attsub_match = re.search(r"output size of Attsub:\s*(\d+)\s*\|\s*RR of Attsub:\s*([0-9.]+)", output)

    if sphere_match and attsub_match:
        sphere_size = int(sphere_match.group(1))
        sphere_rr = float(sphere_match.group(2))
        attsub_size = int(attsub_match.group(1))
        attsub_rr = float(attsub_match.group(2))

        sphere_sizes.append(sphere_size)
        sphere_rrs.append(sphere_rr)
        attsub_sizes.append(attsub_size)
        attsub_rrs.append(attsub_rr)

        if attsub_rr <= sphere_rr:
            attsub_better_count += 1

# Compute stats
def mean(lst):
    return sum(lst) / len(lst) if lst else float('nan')

n = len(sphere_rrs)
winning_rate = attsub_better_count / n if n else float('nan')

# Summary
print("\n===== Final Results =====")
print(f"Sphere:  avg size = {mean(sphere_sizes):.2f}, avg RR = {mean(sphere_rrs):.6f}")
print(f"Attsub: avg size = {mean(attsub_sizes):.2f}, avg RR = {mean(attsub_rrs):.6f}")
print(f"Attsub Winning Rate: {winning_rate:.2%} ({attsub_better_count} / {n})")
