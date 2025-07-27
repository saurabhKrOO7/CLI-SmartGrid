# CLI-SmartGrid
PowerGrid‑CLI: Priority‑Based Load Balancing &amp; Maintenance Simulator

Here’s a comprehensive **project overview** for your CLI Smart‑Grid Demand‑Response Coordinator:

---

## 1. Project Title

**CLI‑SmartGrid: Priority‑Based Demand‑Response & Maintenance Scheduler**

---

## 2. Functionality

* **Report Demand**: Consumers submit power requests (Residential, Commercial, Industrial).
* **Balance Load**: Allocate requests across substations in priority order; auto‑shed excess when capacity is insufficient.
* **Schedule Maintenance**: Temporarily take substations offline for maintenance windows.
* **Show Status**: View live substation loads, pending demands, and maintenance jobs.

---

## 3. Features

* **Priority Queueing**: Industrial > Commercial > Residential ordering.
* **Dynamic Allocation**: Greedy substation allocation, tracks used vs. available MW.
* **Auto‑Shedding**: Unserved requests are flagged as “shed” if grid capacity is exceeded.
* **Maintenance Simulation**: Schedule 1‑hour maintenance after a user‑defined delay.
* **Interactive CLI**: REPL style with `help`, usage prompts, and feedback messages.

---

## 4. How It Works (Logic & Flow)

1. **Initialization**

   * Load a hard‑coded list of substations (IDs & capacities).
2. **REPL Loop**

   * Prompt `> `, parse user command.
3. **Report Command**

   * `report C101 res 25.5` → instantiates a `ResidentialRequest`, timestamps it, and enqueues.
4. **Maintenance Command**

   * `maintenance S02 300` → schedules a `MaintenanceJob` to start 300 s later, lasting 1 h.
5. **Balance Command**

   * At `balance`:

     * **Advance Maintenance**: Mark substations offline/online based on current time.
     * **Process Queue**: Pop each `DemandRequest` in priority order; try to allocate to any **online** substation.
     * **Mark States**: `ALLOCATED` if assigned, otherwise `SHED`.
     * **Requeue** only those still `QUEUED` (e.g., partial allocations could be added).
6. **Status Command**

   * Prints:

     * **Substations**: `usedMW/capacityMW (ONLINE/OFFLINE)`
     * **Pending Demands**: List of queued requests with priority levels.
     * **Maintenance Jobs**: Substation ID and current job state.

---

## 5. Technologies & Concepts Used

| Area                | Details                                                          |
| ------------------- | ---------------------------------------------------------------- |
| **Language**        | C++17                                                            |
| **Build Tool**      | g++ (GNU C++ Compiler)                                           |
| **OOP Principles**  | Inheritance, Polymorphism, Encapsulation                         |
| **Data Structures** | Priority Queue (`std::priority_queue`), Queue, Vector, List, Map |
| **OS Concepts**     | Job Scheduling (priority queue), Process States (QUEUED → …)     |
| **Time Management** | `time_t` timestamps, simulated delays for maintenance            |
| **CLI Parsing**     | `std::getline`, `std::istringstream` for simple tokenization     |

---

