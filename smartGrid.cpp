/*
 * Smart Power Grid Demand-Response Coordinator (CLI)
 *
 * Functionality:
 * - Report consumer demand
 * - Balance load across substations (auto-shed if capacity insufficient)
 * - Schedule and handle maintenance windows
 * - Show grid status
 */

#include <bits/stdc++.h>
using namespace std;

//----------- DemandRequest.h -----------
// Abstract base class representing a power demand request
class DemandRequest {
public:
    string consumerID;      // Unique ID of the consumer
    double megawatts;       // Power requested in MW
    time_t timestamp;       // Time when request was created
    enum State { CREATED, QUEUED, ALLOCATED, SHED, COMPLETED } state;

    DemandRequest(string cid, double mw)
      : consumerID(cid), megawatts(mw), timestamp(time(nullptr)), state(CREATED) {}

    // Derived classes must define priority: higher means more critical
    virtual int priority() const = 0;
    virtual ~DemandRequest() {}
};

// Residential requests have lowest priority
class ResidentialRequest : public DemandRequest {
public:
    ResidentialRequest(string c, double m): DemandRequest(c,m) {}
    int priority() const override { return 1; }
};
// Commercial requests have medium priority
class CommercialRequest : public DemandRequest {
public:
    CommercialRequest(string c, double m): DemandRequest(c,m) {}
    int priority() const override { return 2; }
};
// Industrial requests have highest priority
class IndustrialRequest : public DemandRequest {
public:
    IndustrialRequest(string c, double m): DemandRequest(c,m) {}
    int priority() const override { return 3; }
};

// Comparator for priority queue: compare by priority then by timestamp (earlier first)
struct CompareDemand {
    bool operator()(DemandRequest* a, DemandRequest* b) const {
        if (a->priority() != b->priority())
            return a->priority() < b->priority();     // higher priority first
        return a->timestamp > b->timestamp;           // older requests first
    }
};

//----------- Substation.h -----------
// Represents a power substation with capacity
class Substation {
public:
    string id;            // Substation identifier
    double capacityMW;    // Maximum capacity in MW
    double usedMW = 0;    // Currently allocated MW
    bool online = true;   // Whether the substation is operational

    Substation(string i, double cap): id(i), capacityMW(cap) {}

    // Returns available capacity if online, else zero
    double available() const { return online ? capacityMW - usedMW : 0; }

    // Try allocate 'mw'; return true if successful
    bool allocate(double mw) {
        if (available() >= mw) {
            usedMW += mw;
            return true;
        }
        return false;
    }

    // Deallocate 'mw' from used capacity (not used in current logic but provided)
    void deallocate(double mw) { usedMW = max(0.0, usedMW - mw); }
};

//----------- MaintenanceJob.h -----------
// Represents a scheduled maintenance for a substation
class MaintenanceJob {
public:
    string substationID;
    time_t startTime;
    time_t endTime;
    enum State { SCHEDULED, IN_PROGRESS, DONE } state;

    MaintenanceJob(string sid, time_t st, time_t et)
      : substationID(sid), startTime(st), endTime(et), state(SCHEDULED) {}

    // Advance job state based on current time
    void advanceState(time_t now) {
        if (state == SCHEDULED && now >= startTime)
            state = IN_PROGRESS;
        if (state == IN_PROGRESS && now >= endTime)
            state = DONE;
    }
};

//----------- GridController.h -----------
// Manages demands, substations, and maintenance
class GridController {
    // Priority queue for pending demand requests
    priority_queue<DemandRequest*, vector<DemandRequest*>, CompareDemand> demandQ;
    vector<Substation> substations;          // All substations in grid
    list<MaintenanceJob> maintenanceList;    // Scheduled maintenance jobs

public:
    GridController() {}

    // Add a substation to the grid
    void addSubstation(const string &id, double cap) {
        substations.emplace_back(id, cap);
    }

    // Enqueue a new demand request
    void receiveDemand(DemandRequest* req) {
        req->state = DemandRequest::QUEUED;
        demandQ.push(req);
    }

    // Schedule a maintenance window [start, end)
    void scheduleMaintenance(const string &sid, time_t start, time_t end) {
        maintenanceList.emplace_back(sid, start, end);
    }

    // Core scheduler: update maintenance, allocate demands or shed
    void runScheduler() {
        time_t now = time(nullptr);

        // 1) Update maintenance jobs and offline substations
        for (auto &job : maintenanceList) {
            job.advanceState(now);
            for (auto &sub : substations) {
                if (sub.id == job.substationID) {
                    // Substation offline during maintenance in-progress
                    sub.online = (job.state != MaintenanceJob::IN_PROGRESS);
                }
            }
        }

        // 2) Process all pending demand requests
        queue<DemandRequest*> temp;
        while (!demandQ.empty()) {
            auto *req = demandQ.top();
            demandQ.pop();
            bool allocated = false;

            // Try allocating request to any available substation
            for (auto &sub : substations) {
                if (sub.allocate(req->megawatts)) {
                    req->state = DemandRequest::ALLOCATED;
                    allocated = true;
                    break;
                }
            }
            // If not allocated, mark as shed
            if (!allocated) {
                req->state = DemandRequest::SHED;
            }
            temp.push(req);
        }

        // 3) Re-enqueue only those still in QUEUED state
        while (!temp.empty()) {
            auto *r = temp.front();
            temp.pop();
            if (r->state == DemandRequest::QUEUED)
                demandQ.push(r);
        }
    }

    // Display current grid status: substations, demands, maintenance
    void showStatus() const {
        cout << "--- Grid Status ---\n";

        cout << "Substations:" << "\n";
        for (auto &s : substations) {
            cout << "  " << s.id << ": " << s.usedMW << "/" << s.capacityMW
                 << (s.online ? " MW (ONLINE)" : " MW (OFFLINE)") << "\n";
        }

        cout << "Pending Demands:" << "\n";
        auto copyQ = demandQ;  // Copy to iterate without modifying
        while (!copyQ.empty()) {
            auto *r = copyQ.top();
            copyQ.pop();
            cout << "  " << r->consumerID << " (" << r->megawatts
                 << "MW, pr=" << r->priority() << ")\n";
        }

        cout << "Maintenance Jobs:" << "\n";
        for (auto &m : maintenanceList) {
            cout << "  " << m.substationID << " [" << m.state << "]\n";
        }
    }
};

//----------- main.cpp -----------
int main() {
    GridController grid;
    // Initialize some example substations
    grid.addSubstation("S01", 50.0);
    grid.addSubstation("S02", 40.0);
    grid.addSubstation("S03", 60.0);

    // Welcome banner and usage instructions
    cout << "Smart Grid CLI — Demand-Response Coordinator\n";
    cout << "Enter commands to manage grid.\n";
    cout << "Type 'help' for detailed syntax and examples.\n\n";

    string line;
    while (true) {
        cout << "> ";
        if (!getline(cin, line)) break;  // Exit on EOF
        istringstream iss(line);
        string cmd;
        iss >> cmd;

        if (cmd == "exit") {
            cout << "Exiting Smart Grid CLI. Goodbye!\n";
            break;
        }
        else if (cmd == "help") {
            // Expanded help with formats and examples
            cout << "Available commands:\n";
            cout << "  report <consumerID> <res|com|ind> <MW>   "
                 << "-- Submit a demand request.\n";
            cout << "       e.g.: report C101 res 25.5\n";
            cout << "  balance                               "
                 << "-- Run scheduling: allocate or shed load.\n";
            cout << "       e.g.: balance\n";
            cout << "  maintenance <subID> <delaySec>        "
                 << "-- Schedule 1h maintenance after delay.\n";
            cout << "       e.g.: maintenance S02 300   "
                 << "(start in 5 min, lasts 1h)\n";
            cout << "  status                                "
                 << "-- Show grid, demands, maintenance.\n";
            cout << "       e.g.: status\n";
            cout << "  help                                  "
                 << "-- Show this help message.\n";
            cout << "  exit                                  "
                 << "-- Quit the program.\n";
        }
        else if (cmd == "report") {
            // Prompt format if insufficient args
            string cid, type;
            double mw;
            if (!(iss >> cid >> type >> mw)) {
                cout << "Usage: report <consumerID> <res|com|ind> <MW>\n";
                continue;
            }
            DemandRequest *d = nullptr;
            if (type == "res") d = new ResidentialRequest(cid, mw);
            else if (type == "com") d = new CommercialRequest(cid, mw);
            else if (type == "ind") d = new IndustrialRequest(cid, mw);
            if (d) {
                grid.receiveDemand(d);
                cout << "Demand recorded for " << cid << ".\n";
            } else {
                cout << "Invalid type. Use 'res', 'com', or 'ind'.\n";
            }
        }
        else if (cmd == "balance") {
            // Run the scheduler logic
            grid.runScheduler();
            cout << "Load balancing complete.\n";
        }
        else if (cmd == "maintenance") {
            string sid;
            int delaySec;
            if (!(iss >> sid >> delaySec)) {
                cout << "Usage: maintenance <subID> <delaySec>\n";
                continue;
            }
            time_t now = time(nullptr);
            grid.scheduleMaintenance(sid, now + delaySec, now + delaySec + 3600);
            cout << "Maintenance scheduled for " << sid
                 << " starting in " << delaySec << " seconds.\n";
        }
        else if (cmd == "status") {
            // Display current state of the grid
            grid.showStatus();
        }
        else if (cmd.empty()) {
            continue;  // ignore empty lines
        }
        else {
            cout << "Unknown command. Type 'help' for list of commands.\n";
        }
    }
    return 0;
}
