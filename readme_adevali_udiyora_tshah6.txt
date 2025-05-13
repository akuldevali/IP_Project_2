# README for ns-3 TCP Comparison Simulation

This document provides instructions on how to compile and run the provided ns-3 simulation script designed to compare the performance of TcpBic and TcpDctcp congestion control algorithms under different scenarios.

## How to Compile

1.  **Prerequisites:** Ensure you have a working ns-3 environment installed (ns-3.44 preferred). The standard ns-3 build process should have been completed successfully. See the main ns-3 documentation (<https://www.nsnam.org/documentation/latest/>) for installation instructions if needed.
2.  **Placement:** Copy the simulation C++ code file (`tcp_adevali_udiyora_tshah6.cc`) into the `scratch` subdirectory located within your main ns-3 installation directory (e.g., `ns-allinone-3.44/ns-3.44/scratch/`).
3.  **Navigate:** Open a terminal or command prompt and navigate to the top-level directory of your ns-3 installation (e.g., `cd /path/to/your/ns-allinone-3.44/ns-3.44/`).

## How to Execute

1.  **Prerequisites:** Ensure proper installation of ns3 and placement of script inside scrath folder.
2.  **Navigate:** Make sure your terminal is still in the top-level directory of your ns-3 installation.
3.  **Execution Command:** Run the simulation script using the `./ns3 run scratch/tcp_adevali_udiyora_tshah6.cc`

    ```bash
    ./ns3 run scratch/tcp_adevali_udiyora_tshah6.cc
    ```

4.  **Output:**
    - The simulation will print status messages to the console indicating which experiment and iteration is currently running, along with FlowMonitor statistics for each flow upon completion of each run.
    - After all experiments and iterations are finished, the script will generate a CSV file named `tcp_adevali_udiyora_tshah6.csv` in the top-level ns-3 directory.
    ```bash
    /ns-allonone-3.44/ns-3.44/tcp_adevali_udiyora_tshah6.csv
    ```
    This file contains the aggregated results, including the raw throughput (Mbps) and average flow completion time (Seconds) for each run, as well as the calculated average and standard deviation for each metric across the three runs per experiment.

## Extra Information Regarding Implementation

- **Network Topology:** The simulation employs a classic dumbbell topology:
  - 2 Source Nodes (connected to Router 1)
  - 2 Router Nodes (Router 1 and Router 2, connected by the bottleneck link)
  - 2 Destination Nodes (connected to Router 2)
- **Link Characteristics:**
  - **Edge Links** (Source-Router, Router-Destination):
    - Data Rate: `1Gbps`
    - Delay: `0ms`
    - MTU: `1500` bytes
  - **Bottleneck Link** (Router1-Router2):
    - Data Rate: `1Gbps`
    - Delay: `0ms`
    - MTU: `1500` bytes
    - **Queue:** `ns3::DropTailQueue` with `MaxSize` = `100p` (100 packets). This relatively small queue on a high-speed link is intended to induce packet drops and congestion.
    - **ECN for Dctcp:** When experiments involve `TcpDctcp`, the queue discipline on the bottleneck link is replaced with `ns3::RedQueueDisc` configured with `UseEcn` = `true` to enable Explicit Congestion Notification marking, which Dctcp requires.
- **TCP Configuration:**
  - TCP Variants Tested: `TcpBic`, `TcpDctcp`
  - Maximum Bytes per Flow (`maxBytes`): `50 * 1024 * 1024` bytes (50 MiB)
  - Simulation Duration (`flow_time`): Flows run for a maximum of `30.0` seconds. The simulation stops at `32.0` seconds.
  - TCP Segment Size: `1448` bytes
  - Initial Congestion Window (cwnd): `10` segments
  - Send/Receive Buffer Size: `2 * 1024 * 1024` bytes (2 MiB)
- **Randomness:** The simulation uses `RngSeedManager::SetRun` based on the experiment number and iteration number to ensure variability between runs while maintaining reproducibility if the base seed (`12345`) is kept constant.
- **Flow Monitoring:** `ns3::FlowMonitor` is used to collect detailed statistics (throughput, completion time, packet counts) for each TCP flow.

## Required Packages and Dependencies

For successful compilation and execution of this ns-3 script, the following are necessary:

1.  **Build Tools:**

    - A C++ compiler supporting C++11 or later (e.g., `g++` or `clang++`).
    - `CMake` (version 3.10 or later recommended).
    - `Python3` (required by the `./ns3` command-line tool).
    - Standard Unix build utilities (e.g., `make`).
    - `pkg-config` (often required by ns-3 during configuration).

2.  \*\*ns-3 Simulator Core:

    - A working installation of ns-3 (ns-3.44 recommended, as Dctcp and Bic are standard).

3.  **Specific ns-3 Modules:** The script explicitly includes headers from and relies on the functionality of the following ns-3 modules. These must be available and enabled in your ns-3 build:

    - `core-module`
    - `network-module`
    - `internet-module` (provides TCP implementations like TcpBic, TcpDctcp, IP stack, etc.)
    - `point-to-point-module` (for network links)
    - `applications-module` (provides BulkSendApplication, PacketSink)
    - `ipv4-global-routing-helper` (for populating routing tables)
    - `flow-monitor-module` (for collecting statistics)
    - `traffic-control-module` (required for `RedQueueDisc` used with ECN/Dctcp)

    _Note: A standard ns-3 configuration like `./ns3 configure --enable-examples --enable-tests` usually builds all these necessary modules._

4.  **Standard C++ Libraries:** The code uses standard C++ libraries (`<fstream>`, `<string>`, `<cmath>`, `<vector>`, `<iostream>`, `<algorithm>`, `<random>`), which are included with your C++ compiler. No separate installation is needed for these.
