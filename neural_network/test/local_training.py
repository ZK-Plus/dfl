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
        while True:
            try:
                if process.is_running() and process.status() != psutil.STATUS_ZOMBIE:
                    cpu_percent = process.cpu_percent(interval=1)
                    memory_percent = process.memory_percent()
                    timestamp = time.time()
                    f.write(f"{timestamp},{cpu_percent},{memory_percent}\n")
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
