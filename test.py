import os
import sys
import json
import ast
import matplotlib.pyplot as plt
from itu_p1203 import P1203Standalone
from itu_p1203 import __main__ as main_script

# Sliding window parameters
N = 5  # Window size
P = 2  # Step size

def get_segment_files(folder="segment"):
    """Get all segment files sorted alphabetically."""
    files = [os.path.join(folder, f) for f in os.listdir(folder) if f.endswith(".ts")]
    return sorted(files)

def evaluate_segments(segment_files, mode, stalls=None):
    """Evaluate a list of segment files with a specific mode and return MOS result."""
    input_report = main_script.Extractor(input_files=segment_files, mode=mode).extract()
    if stalls:
        input_report["I23"]["stalling"] = stalls

    itu_p1203 = P1203Standalone(
        input_report,
        debug=False,
        quiet=True,
        amendment_1_audiovisual=False,
        amendment_1_stalling=False,
        amendment_1_app_2=False,
        fast_mode=False,
    )

    return itu_p1203.calculate_complete(input_report)

def plot_mos(mos_dict):
    """Plot MOS values for each mode on the same graph."""
    plt.figure(figsize=(10, 6))
    
    for mode, mos_list in mos_dict.items():
        x = list(range(1, len(mos_list) + 1))
        plt.plot(x, mos_list, marker='o', label=f'Mode {mode}')
    
    plt.title('MOS over Sliding Windows for Different Modes')
    plt.xlabel('Window Index')
    plt.ylabel('MOS (Mean Opinion Score)')
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.savefig("mos_modes_plot.png")
    plt.show()

def main():
    segment_files = get_segment_files("./segments")
    stalls = []

    if "-s" in sys.argv:
        try:
            s_index = sys.argv.index("-s")
            stalls = ast.literal_eval(sys.argv[s_index + 1])
        except (IndexError, SyntaxError, ValueError):
            print("Invalid stalls format. Use -s [[start, duration], [start, duration]]")
            sys.exit(1)

    total_segments = 27
    mos_results = {0: [], 1: [], 2: [], 3: []}

    for mode in range(4):
        print(f"\n--- Processing mode {mode} ---")
        i = 0
        while i + N <= total_segments:
            window_segments = segment_files[i:i + N]
            print(f"Evaluating segments {i + 1} to {i + N}")
            result = evaluate_segments(window_segments, mode, stalls)

            try:
                mos_value = result["O46"]
            except KeyError:
                print("MOS not found.")
                mos_value = None

            print(f"Mode {mode} - MOS: {mos_value}")
            mos_results[mode].append(mos_value)
            i += P

    plot_mos(mos_results)

if __name__ == "__main__":
    main()
