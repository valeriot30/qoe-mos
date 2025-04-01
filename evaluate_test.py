from itu_p1203 import P1203Standalone
from itu_p1203 import P1203Pq
from itu_p1203 import P1203Pa
from itu_p1203 import P1203Pv

from itu_p1203 import P1203Standalone
from itu_p1203 import P1203Pq
from itu_p1203 import P1203Pa
from itu_p1203 import P1203Pv

from itu_p1203 import __main__ as main_script

import sys
import json
import time
import ast


def main():

    start_time = time.time()
    if len(sys.argv) < 1:
        print("Usage: python evaluate.py segment-x.ts")
        sys.exit(1)

    segment_files = sys.argv[1:]
    stalls = []
    args = sys.argv[1:]

    if "-s" in args:
        s_index = args.index("-s")
        segment_files = args[:s_index]  # Everything before -s is a segment file
        try:
            stalls = ast.literal_eval(args[s_index + 1])  # Convert next argument to list
        except (IndexError, SyntaxError, ValueError):
            print("Invalid stalls format. Use -s [[start, duration], [start, duration]]")
            sys.exit(1)
    else:
        segment_files = args  # No -s, assume only segment files

    #print("Segments:", segment_files)

    input_report = main_script.Extractor(input_files=segment_files, mode=3).extract()
    print(input_report)
    input_report["I23"]["stalling"] = stalls

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

    #print(results)


    end_time = time.time()  # Capture the end time

    execution_time = end_time - start_time  # Calculate the execution time
    print(f"Execution time: {execution_time:.2f} seconds")



if __name__ == "__main__":
    main()