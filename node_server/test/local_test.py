import subprocess
import psutil
import time


def log_system_metrics(pid):
    process = psutil.Process(pid)
    with open("./metrics_log.txt", "w") as f:
        f.write("time,cpu_percent,memory_percent\n")
        while process.is_running():
            cpu_percent = process.cpu_percent(interval=1)
            memory_info = process.memory_info()
            memory_percent = process.memory_percent()
            timestamp = time.time()
            f.write(f"{timestamp},{cpu_percent},{memory_percent}\n")
            time.sleep(1)


if __name__ == "__main__":
    executable_path = "../start.exe"
    process = subprocess.Popen([executable_path, "train"])
    log_system_metrics(process.pid)
    process.wait()
