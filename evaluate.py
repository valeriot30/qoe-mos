from itu_p1203 import P1203Standalone
from itu_p1203 import __main__ as main_script

import sys
import json
import ast


def main():
    if len(sys.argv) < 2:
        print("Usage: python evaluate.py segment-x.ts [-s [[start, duration], ...]] [-m mode]")
        sys.exit(1)

    args = sys.argv[1:]
    segment_files = []
    stalls = []
    mode = 1  # Default mode

    # Parse arguments
    i = 0
    while i < len(args):
        if args[i] == "-s":
            try:
                stalls = ast.literal_eval(args[i + 1])
                i += 2
            except (IndexError, SyntaxError, ValueError):
                print("Invalid stalls format. Use -s [[start, duration], [start, duration]]")
                sys.exit(1)
        elif args[i] == "-m":
            try:
                mode = int(args[i + 1])
                i += 2
            except (IndexError, ValueError):
                print("Invalid mode format. Use -m <integer>")
                sys.exit(1)
        else:
            segment_files.append(args[i])
            i += 1

    if not segment_files:
        print("No segment files specified.")
        sys.exit(1)

    # Extract input report
    input_report = main_script.Extractor(input_files=segment_files, mode=mode).extract()
    input_report["I23"]["stalling"] = stalls

    # Run P1203 evaluation
    itu_p1203 = P1203Standalone(
        input_report,
        debug=False,
        quiet=False,
        amendment_1_audiovisual=False,
        amendment_1_stalling=False,
        amendment_1_app_2=False,
        fast_mode=False,
    )

    results = json.dumps(itu_p1203.calculate_complete(input_report))
    print(results)


if __name__ == "__main__":
    main()
