import pandas as pd
import matplotlib.pyplot as plt


def plot_metrics():
    # Load the log data
    metrics_df = pd.read_csv("./local_training_log.txt")

    # Convert the 'time' column to datetime
    metrics_df["time"] = pd.to_datetime(metrics_df["time"], unit="s")

    # Calculate relative time in seconds (starting from zero)
    start_time = metrics_df["time"].iloc[0]
    metrics_df["relative_time"] = (metrics_df["time"] - start_time).dt.total_seconds()

    # Create a figure and axis
    fig, ax1 = plt.subplots()

    # Plot CPU usage
    (cpu_line,) = ax1.plot(
        metrics_df["relative_time"],
        metrics_df["cpu_percent"],
        label="CPU Usage (%)",
        color="#17BEBB",
    )
    ax1.tick_params(axis="y", labelcolor="#2E282A")

    # Create a second y-axis to plot memory usage
    ax2 = ax1.twinx()
    (memory_line,) = ax2.plot(
        metrics_df["relative_time"],
        metrics_df["memory_percent"],
        label="Memory Usage (%)",
        color="#EF3E36",
    )
    ax2.tick_params(axis="y", labelcolor="tab:red")

    # Customize the plot to remove the top, left, and right spines
    ax1.spines["top"].set_visible(False)
    ax1.spines["right"].set_visible(False)
    ax1.spines["left"].set_visible(False)
    ax2.spines["top"].set_visible(False)
    ax2.spines["right"].set_visible(False)
    ax2.spines["left"].set_visible(False)

    # Customize the ticks to appear only at the bottom
    ax1.xaxis.set_ticks_position("bottom")
    ax1.yaxis.set_ticks_position("none")
    ax2.yaxis.set_ticks_position("none")

    # Add text labels to the end of the lines
    # ax1.text(
    #     metrics_df["relative_time"].iloc[-1] * 1.2,
    #     metrics_df["cpu_percent"].iloc[-1],
    #     "CPU Usage (%)",
    #     color="#17BEBB",
    #     fontweight="bold",
    #     horizontalalignment="left",
    #     verticalalignment="center",
    # )
    # ax2.text(
    #     metrics_df["relative_time"].iloc[-1] * 1.2,
    #     metrics_df["memory_percent"].iloc[-1],
    #     "Memory Usage (%)",
    #     color="#EF3E36",
    #     fontweight="bold",
    #     horizontalalignment="left",
    #     verticalalignment="center",
    # )

    # Add a title and show the plot
    fig.tight_layout()
    # display legend
    fig.legend(
        handles=[cpu_line, memory_line], loc="upper left", bbox_to_anchor=(0.1, 0.98)
    )
    plt.show()


if __name__ == "__main__":
    plot_metrics()
