#!/usr/bin/env python3
"""
Simple Plot Generator for High-Dimensional Results
Takes data files and generates plots in the desired format.
"""

import os
import subprocess
import glob

# ============================================================================
# CONFIGURATION - EDIT THESE SETTINGS
# ============================================================================

# Plot style settings
PLOT_STYLE = "black_lines"  # Options: "black_lines", "colored_lines"

# Output directory for plots
PLOT_DIR = "plot"

# ============================================================================
# FUNCTIONS
# ============================================================================

def get_plot_title(param_name):
    """Get appropriate plot titles based on parameter"""
    titles = {
        "d_hat_1": "c₁",
        "d_hat_2": "c₂", 
        "d_prime": "d'",
        "K": "K",
        "num_q": "Number of Questions",
        "epsilon": "Epsilon",
        "target_rr": "Target Regret Ratio",
        "scalability_n": "Dataset Size (n)",
        "scalability_d": "Dimensions (d)"
    }
    return titles.get(param_name, param_name)

def create_internal_parameters_plot(data_file, param_name):
    """Create plot for internal parameters experiment with different d' values as lines"""
    param_title = get_plot_title(param_name)
    
    # Create plot directory
    plot_subdir = f"{PLOT_DIR}/{param_name}"
    os.makedirs(plot_subdir, exist_ok=True)
    
    # Determine y-axis based on parameter
    if param_name == "d_hat_1":
        y_axis = "questions"  # Number of questions for d_hat_1
        plot_title = f"Number of Questions vs {param_title}"
        ylabel = "Number of Questions"
    else:  # d_hat_2
        y_axis = "regret_ratio"  # Regret ratio for d_hat_2
        plot_title = f"Regret Ratio vs {param_title}"
        ylabel = "Regret Ratio"
    
    # Create gnuplot script with different d' values as different lines
    script = f"""
set terminal pngcairo enhanced font "Arial,12" size 800,600
set output "{plot_subdir}/internal_parameters_plot.png"

# Black lines only style with different line styles
unset grid
set key outside above horizontal center
set style line 1 lc rgb '#000000' lt 1 lw 2 pt 1 ps 1.5   # solid line
set style line 2 lc rgb '#000000' lt 2 lw 2 pt 6 ps 1.5   # dashed line
set style line 3 lc rgb '#000000' lt 3 lw 2 pt 8 ps 1.5   # dotted line
set style line 4 lc rgb '#000000' lt 4 lw 2 pt 9 ps 1.5   # dash-dot line

set title "{plot_title}" font "Arial,16"
set xlabel "{param_title}" font "Arial,14"
set ylabel "{ylabel}" font "Arial,14"

# Plot different d' values as different lines
"""
    
    # Add plot commands for each d' value
    d_prime_values = [2, 3, 4, 5]
    plot_commands = []
    
    for i, d_prime in enumerate(d_prime_values):
        line_style = i + 1
        if y_axis == "questions":
            # Plot number of questions (column 9)
            plot_commands.append(f'"{data_file}" using ($2=={d_prime} ? $1 : 1/0):9 with linespoints ls {line_style} title "d\'={d_prime}"')
        else:
            # Plot regret ratio (column 4 - OurAlg RR)
            plot_commands.append(f'"{data_file}" using ($2=={d_prime} ? $1 : 1/0):4 with linespoints ls {line_style} title "d\'={d_prime}"')
    
    # Join all plot commands
    script += "plot " + ", \\\n     ".join(plot_commands) + "\n"
    
    return script

def create_regular_experiment_plots(data_file, param_name):
    """Create plots for regular experiments (multiple metrics)"""
    param_title = get_plot_title(param_name)
    
    # Create plot directory
    plot_subdir = f"{PLOT_DIR}/{param_name}"
    os.makedirs(plot_subdir, exist_ok=True)
    
    # Plot 1: Regret Ratio Comparison
    script1 = f"""
set terminal pngcairo enhanced font "Arial,12" size 800,600
set output "{plot_subdir}/01_regret_ratio.png"
unset grid
set key outside above horizontal center
set style line 1 lc rgb '#000000' lt 1 lw 2 pt 1 ps 1.5
set style line 2 lc rgb '#000000' lt 1 lw 2 pt 6 ps 1.5
set style line 3 lc rgb '#000000' lt 1 lw 2 pt 8 ps 1.5
set title "Regret Ratio Comparison vs {param_title}" font "Arial,16"
set xlabel "{param_title}" font "Arial,14"
set ylabel "Regret Ratio" font "Arial,14"
plot "{data_file}" using 1:2 with linespoints ls 1 title "Sphere RR", \\
     "{data_file}" using 1:3 with linespoints ls 2 title "OurAlg RR", \\
     "{data_file}" using 1:10 with linespoints ls 3 title "UtilityApprox RR"
"""
    
    # Plot 2: Time Comparison
    script2 = f"""
set terminal pngcairo enhanced font "Arial,12" size 800,600
set output "{plot_subdir}/02_time_comparison.png"
unset grid
set key outside above horizontal center
set style line 4 lc rgb '#000000' lt 1 lw 2 pt 9 ps 1.5
set style line 5 lc rgb '#000000' lt 1 lw 2 pt 4 ps 1.5
set style line 6 lc rgb '#000000' lt 1 lw 2 pt 5 ps 1.5
set title "Time Comparison vs {param_title}" font "Arial,16"
set xlabel "{param_title}" font "Arial,14"
set ylabel "Time (seconds)" font "Arial,14"
set logscale y
plot "{data_file}" using 1:4 with linespoints ls 4 title "Sphere Time", \\
     "{data_file}" using 1:5 with linespoints ls 5 title "OurAlg Time", \\
     "{data_file}" using 1:11 with linespoints ls 6 title "UtilityApprox Time"
"""
    
    # Plot 3: Questions
    script3 = f"""
set terminal pngcairo enhanced font "Arial,12" size 800,600
set output "{plot_subdir}/03_questions.png"
unset grid
set key outside above horizontal center
set style line 7 lc rgb '#000000' lt 1 lw 2 pt 12 ps 1.5
set title "Number of Questions vs {param_title}" font "Arial,16"
set xlabel "{param_title}" font "Arial,14"
set ylabel "Number of Questions" font "Arial,14"
plot "{data_file}" using 1:8 with linespoints ls 7 title "Questions"
"""
    
    # Plot 4: Outperformance Rate
    script4 = f"""
set terminal pngcairo enhanced font "Arial,12" size 800,600
set output "{plot_subdir}/04_outperformance_rate.png"
unset grid
set key outside above horizontal center
set style line 8 lc rgb '#000000' lt 1 lw 2 pt 2 ps 1.5
set title "Outperformance Rate vs {param_title}" font "Arial,16"
set xlabel "{param_title}" font "Arial,14"
set ylabel "Outperformance Rate (%)" font "Arial,14"
plot "{data_file}" using 1:($9*100) with linespoints ls 8 title "Outperformance Rate"
"""
    
    # Plot 5: Combined (multiplot)
    script5 = f"""
set terminal pngcairo enhanced font "Arial,12" size 1200,800
set output "{plot_subdir}/05_combined_metrics.png"

set multiplot layout 2,2 title "Metrics vs {param_title}" font "Arial,16"

unset grid
set key outside above horizontal center
set style line 1 lc rgb '#000000' lt 1 lw 2 pt 1 ps 1.5
set style line 2 lc rgb '#000000' lt 1 lw 2 pt 6 ps 1.5
set style line 3 lc rgb '#000000' lt 1 lw 2 pt 8 ps 1.5
set style line 4 lc rgb '#000000' lt 1 lw 2 pt 9 ps 1.5
set style line 5 lc rgb '#000000' lt 1 lw 2 pt 4 ps 1.5
set style line 6 lc rgb '#000000' lt 1 lw 2 pt 5 ps 1.5

set title "Regret Ratio" font "Arial,14"
set xlabel "{param_title}" font "Arial,12"
set ylabel "Regret Ratio" font "Arial,12"
set offsets 0, 0, 0, 0
plot "{data_file}" using 1:2 with linespoints ls 1 title "Sphere", \\
     "{data_file}" using 1:3 with linespoints ls 2 title "OurAlg", \\
     "{data_file}" using 1:10 with linespoints ls 3 title "UtilityApprox"

set title "Time" font "Arial,14"
set xlabel "{param_title}" font "Arial,12"
set ylabel "Time (seconds)" font "Arial,12"
set logscale y
plot "{data_file}" using 1:4 with linespoints ls 4 title "Sphere", \\
     "{data_file}" using 1:5 with linespoints ls 5 title "OurAlg", \\
     "{data_file}" using 1:11 with linespoints ls 6 title "UtilityApprox"

set title "Questions" font "Arial,14"
set xlabel "{param_title}" font "Arial,12"
set ylabel "Questions" font "Arial,12"
plot "{data_file}" using 1:8 with linespoints ls 1 title "Questions"

set title "Outperformance Rate" font "Arial,14"
set xlabel "{param_title}" font "Arial,12"
set ylabel "Rate (%)" font "Arial,12"
plot "{data_file}" using 1:($9*100) with linespoints ls 2 title "Outperformance"

unset multiplot
"""
    
    return [script1, script2, script3, script4, script5]

def generate_plots(data_file, param_name=None):
    """Generate plots for a data file"""
    if param_name is None:
        # Extract parameter name from filename
        basename = os.path.basename(data_file)
        param_name = basename.replace("_results.dat", "")
    
    print(f"Generating plots for {param_name}...")
    
    # Determine if this is an internal parameters experiment
    is_internal_param = param_name in ["d_hat_1", "d_hat_2"]
    
    if is_internal_param:
        # Internal parameters experiment - single plot with different d' values as lines
        print(f"  Internal parameters experiment - creating single plot with different d' values")
        script = create_internal_parameters_plot(data_file, param_name)
        scripts = [script]
        plot_names = ["internal_parameters_plot"]
    else:
        # Regular experiment - multiple plots
        print(f"  Regular experiment - creating multiple plots")
        scripts = create_regular_experiment_plots(data_file, param_name)
        plot_names = ["01_regret_ratio", "02_time_comparison", "03_questions", "04_outperformance_rate", "05_combined_metrics"]
    
    # Generate plots
    for i, (script, name) in enumerate(zip(scripts, plot_names)):
        # Create temporary script file
        script_file = f"{PLOT_DIR}/{param_name}_{name}.gp"
        with open(script_file, 'w') as f:
            f.write(script)
        
        try:
            subprocess.run(["gnuplot", script_file], check=True)
            print(f"  ✓ Generated {name}.png")
        except subprocess.CalledProcessError as e:
            print(f"  ✗ Error generating {name}.png: {e}")
        except FileNotFoundError:
            print(f"  ✗ Gnuplot not found. Please install gnuplot.")
            return False
        
        # Clean up temporary script file
        os.remove(script_file)
    
    return True

# ============================================================================
# MAIN EXECUTION
# ============================================================================

def main():
    """Main function to generate plots"""
    print("High-Dimensional Results Plot Generator")
    print("=" * 50)
    
    # Ensure plot directory exists
    os.makedirs(PLOT_DIR, exist_ok=True)
    
    # Find all .dat files in result directory
    dat_files = glob.glob("result/*_results.dat")
    
    if not dat_files:
        print("No .dat files found in result/ directory!")
        print("Please run data_collector.py first to generate data files.")
        return
    
    print(f"Found {len(dat_files)} data file(s):")
    for f in dat_files:
        print(f"  - {f}")
    
    # Generate plots for each data file
    success_count = 0
    for data_file in dat_files:
        if generate_plots(data_file):
            success_count += 1
    
    print(f"\nPlot generation complete!")
    print(f"Successfully generated plots for {success_count}/{len(dat_files)} data files.")
    print(f"Plots saved in {PLOT_DIR}/ directory.")

if __name__ == "__main__":
    main() 