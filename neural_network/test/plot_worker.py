import pandas as pd
import matplotlib.pyplot as plt


def plot_metrics():
    # Load the log data
    metrics_df = pd.read_csv("./local_worker.txt")

    # Create a figure and axis
    fig, ax1 = plt.subplots()

    # Plot CPU usage
    (execution_line,) = ax1.plot(
        metrics_df["amount"],
        metrics_df["execution_time"],
        label="Execution Time (s)",
        color="#17BEBB",
    )
    ax1.tick_params(axis="y", labelcolor="#0F7F7D")

    # Customize the plot to remove the top, left, and right spines
    ax1.spines["top"].set_visible(False)
    ax1.spines["right"].set_visible(False)
    ax1.spines["left"].set_visible(False)

    # Customize the ticks to appear only at the bottom
    ax1.xaxis.set_ticks_position("bottom")
    ax1.yaxis.set_ticks_position("none")

    # Set x-axis ticks to show every number
    ax1.set_xticks(metrics_df["amount"])

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
    fig.tight_layout(pad=2)
    # add x axis label
    plt.xlabel("Amount of local models")
    # Add grid lines for the y-axis
    ax1.yaxis.grid(True, which="major", linestyle="--", linewidth=0.5, color="grey")
    # SECTIONax2.yaxis.grid(True, which="major", linestyle="--", linewidth=0.5, color="grey")
    # display legend
    fig.legend(handles=[execution_line], loc="upper left", bbox_to_anchor=(0.1, 1))
    plt.show()


if __name__ == "__main__":
    plot_metrics()
