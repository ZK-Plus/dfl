import subprocess
import psutil
import time


if __name__ == "__main__":
    executable_path = "./start.exe"
    with open("./local_worker.txt", "w") as f:
        f.write("amount,execution_time\n")
        for i in range(1, 21):
            print(f"Running aggregate {i}")
            start_time = time.time()
            process = subprocess.Popen(
                [executable_path, "aggregate", str(i)], cwd="../"
            )
            process.wait()
            end_time = time.time()
            f.write(f"{i},{end_time - start_time}\n")
