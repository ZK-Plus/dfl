import subprocess
import psutil
import time


def log_system_metrics(pid):
    try:
        process = psutil.Process(pid)
    except psutil.NoSuchProcess:
        print(f"No process found with PID: {pid}")
        return

    with open("./local_training_log.txt", "w") as f:
        f.write("time,cpu_percent,memory_percent\n")

        # Initial call to prime the cpu_percent calculation
        process.cpu_percent(interval=1)

        while True:
            try:
                if process.is_running() and process.status() != psutil.STATUS_ZOMBIE:
                    timestamp = time.time()
                    cpu_percent = process.cpu_percent(interval=1)
                    memory_percent = process.memory_percent()
                    f.write(f"{timestamp},{cpu_percent},{memory_percent}\n")
                    f.flush()  # Ensure data is written to the file
                    # For debugging: print the metrics to the console
                    print(
                        f"Time: {timestamp}, CPU: {cpu_percent}%, Memory: {memory_percent}%"
                    )
                else:
                    break
            except psutil.NoSuchProcess:
                print(f"Process {pid} no longer exists.")
                break
            except psutil.ZombieProcess:
                print(f"Process {pid} has become a zombie.")
                break
            time.sleep(1)


if __name__ == "__main__":
    executable_path = "./start.exe"
    start_time = time.time()
    process = subprocess.Popen([executable_path, "train"], cwd="../")
    log_system_metrics(process.pid)
    process.wait()
    end_time = time.time()
    print(f"Training process finished in {end_time - start_time} seconds.")
