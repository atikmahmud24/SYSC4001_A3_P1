import os
import glob
import csv
import re
from collections import defaultdict

#Need to change the DATA_FOLDER according to what we want to run, currently set to EP.
DATA_FOLDER = "output_files/EP"
OUTPUT_CSV = "results.csv"

ROW_PATTERN = re.compile(
    r"\|\s*(\d+)\s*\|\s*(\d+)\s*\|\s*([A-Z]+)\s*\|\s*([A-Z]+)\s*\|"
)


def parse_events(file_path):
    events = []
    with open(file_path, "r") as f:
        for line in f:
            line = line.strip()
            match = ROW_PATTERN.match(line)
            if match:
                time = int(match.group(1))
                pid = int(match.group(2))
                old_state = match.group(3)
                new_state = match.group(4)
                events.append((time, pid, old_state, new_state))

    events.sort(key=lambda e: e[0])
    return events


def compute_metrics(events):
    if not events:
        return None

    arrival = {}
    completion = {}
    first_run = {}
    ready_time = defaultdict(int)

    last_state = {}
    last_time = {}

    for time, pid, old_state, new_state in events:
        if pid in last_state:
            dt = time - last_time[pid]
            prev_state = last_state[pid]

            if prev_state == "READY":
                ready_time[pid] += dt

        if old_state == "NEW" and new_state == "READY" and pid not in arrival:
            arrival[pid] = time

        if new_state == "RUNNING" and pid not in first_run:
            first_run[pid] = time

        if new_state == "TERMINATED":
            completion[pid] = time

        last_state[pid] = new_state
        last_time[pid] = time

    pids = sorted(completion.keys())
    if not pids:
        return None

    for pid in pids:
        if pid not in arrival:
            first_event_time = min(t for t, p, _, _ in events if p == pid)
            arrival[pid] = first_event_time

    turnaround_list = []
    waiting_list = []
    response_list = []

    for pid in pids:
        tat = completion[pid] - arrival[pid]
        turnaround_list.append(tat)

        wt = ready_time.get(pid, 0)
        waiting_list.append(wt)

        fr = first_run.get(pid, arrival[pid])
        response_list.append(fr - arrival[pid])

    makespan = max(completion[pid] for pid in pids) - min(arrival[pid] for pid in pids)
    if makespan <= 0:
        throughput = 0.0
    else:
        throughput = len(pids) / makespan

    avg_wait = sum(waiting_list) / len(waiting_list)
    avg_tat = sum(turnaround_list) / len(turnaround_list)
    avg_resp = sum(response_list) / len(response_list)

    return throughput, avg_wait, avg_tat, avg_resp


def main():
    files = sorted(glob.glob(os.path.join(DATA_FOLDER, "execution*.txt")))

    with open(OUTPUT_CSV, "w", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(
            [
                "file",
                "throughput",
                "avg_waiting_time",
                "avg_turnaround_time",
                "avg_response_time",
            ]
        )

        for path in files:
            events = parse_events(path)
            metrics = compute_metrics(events)
            if metrics is None:
                continue
            throughput, avg_wait, avg_tat, avg_resp = metrics
            writer.writerow(
                [
                    os.path.basename(path),
                    f"{throughput:.6f}",
                    f"{avg_wait:.6f}",
                    f"{avg_tat:.6f}",
                    f"{avg_resp:.6f}",
                ]
            )


if __name__ == "__main__":
    main()