import subprocess
import psutil
import time


def log_system_metrics(pid):
    """Logs system metrics (CPU and Memory usage) for the given process PID."""
    try:
        process = psutil.Process(pid)
    except psutil.NoSuchProcess:
        print(f"No process found with PID: {pid}")
        return None

    # Initialize metrics accumulation
    total_cpu_percent = 0.0
    total_memory_usage = 0.0  # In bytes
    sample_count = 0

    # Prime the cpu_percent calculation
    process.cpu_percent(interval=1)

    # Continuously log CPU and memory usage
    while True:
        try:
            if not process.is_running():
                print(f"Process {pid} is no longer running.")
                break

            if process.status() == psutil.STATUS_ZOMBIE:
                print(f"Process {pid} has become a zombie.")
                break

            cpu_percent = process.cpu_percent(interval=1)
            memory_usage = process.memory_info().rss / (
                1024 * 1024
            )  # Convert bytes to MB
            total_cpu_percent += cpu_percent
            total_memory_usage += memory_usage
            sample_count += 1

            # Debugging - print to console
            print(f"CPU: {cpu_percent}%, Memory: {memory_usage} MB")

        except psutil.NoSuchProcess:
            print(f"Process {pid} terminated.")
            break
        except psutil.ZombieProcess:
            print(f"Process {pid} has turned into a zombie, skipping metrics.")
            break

    # Calculate and return averages
    avg_cpu_percent = total_cpu_percent / sample_count if sample_count > 0 else 0
    avg_memory_usage = total_memory_usage / sample_count if sample_count > 0 else 0
    return avg_cpu_percent, avg_memory_usage


if __name__ == "__main__":
    executable_path = "./start.exe"
    with open("./aggr_metrics.txt", "w") as f:
        f.write("models,execution_time,avg_cpu_percent,avg_memory_usage_MB\n")
        model_amount = 1000
        for i in range(1, 10):
            print(f"Running aggregate {i}")
            start_time = time.time()

            # Start the subprocess
            process = subprocess.Popen(
                [executable_path, "aggregate", str(model_amount)], cwd="../"
            )

            # Introduce a small delay to let the process initialize
            time.sleep(0.1)

            # Log CPU and memory usage for this process
            avg_cpu_percent, avg_memory_usage = log_system_metrics(process.pid)

            # Wait for the process to complete
            process.wait()

            # Calculate execution time
            end_time = time.time()
            execution_time = end_time - start_time

            # Log metrics for this epoch
            f.write(
                f"{model_amount},{execution_time},{avg_cpu_percent},{avg_memory_usage}\n"
            )
            f.flush()  # Ensure the data is written to file

            # Print for debugging
            print(f"Required time: {execution_time} seconds")
            print(
                f"Average CPU: {avg_cpu_percent}%, Average Memory: {avg_memory_usage} MB"
            )

            # Update model amount
            model_amount += 3000
