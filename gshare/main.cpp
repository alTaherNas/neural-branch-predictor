#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <cstdlib>
#include <cmath>
#include <bitset>
#include "pin.H"

using std::ofstream;
using std::cout;
using std::cerr;
using std::endl;
using std::string;

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,         "pintool",
                            "o", "branch_predictor.out", "specify output file name");

KNOB<BOOL>   KnobPid(KNOB_MODE_WRITEONCE,                "pintool",
                            "i", "0", "append pid to output");

KNOB<UINT64> KnobBranchLimit(KNOB_MODE_WRITEONCE,        "pintool",
                            "l", "0", "set limit of branches analyzed");

KNOB<BOOL>   KnobUseGHR(KNOB_MODE_WRITEONCE,              "pintool",
                            "x", "1", "use GHR for XOR indexing");

/* =====================================================================  

                Gshare Branch Predictor Parameters

=====================================================================  */

#define GHR_LENGTH 16
#define ST_SIZE 65536

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */
UINT64 CountSeen = 0;
UINT64 CountTaken = 0;
UINT64 CountCorrect = 0;
UINT64 ST_AccessCount[ST_SIZE] = {0}; // Array to store access counts for each state table entry

UINT64 GHR = 0; // Global History Register

/* =====================================================================  

                Modeling of the tables and state of automatons

=====================================================================  */

struct st_entry {
    UINT8 state = 0; // Initialize with 00, predicting not taken
} ST[ST_SIZE];

/* =====================================================================  

                Initialization of the states

=====================================================================  */

VOID ST_init() {
    for (int i = 0; i < ST_SIZE; i++) {
        ST[i].state = 0;
    }
}

/* =====================================================================  

                Modeling the output of state machines

=====================================================================  */

bool ST_predict(ADDRINT ins_ptr) {
    UINT64 st_index;
    if (KnobUseGHR.Value()) {
        st_index = (GHR ^ ins_ptr) & (ST_SIZE - 1); // Gshare indexing
    } else {
        st_index = ins_ptr & (ST_SIZE - 1); // Naive indexing without GHR
    }
    return ST[st_index].state >= 2; // Predict taken if state is 10 or 11
}

/* =====================================================================  

                Modeling the transition of state machines

=====================================================================  */

VOID ST_transition(ADDRINT ins_ptr, bool taken, UINT32 branch_info) {
    UINT64 st_index;
    if (KnobUseGHR.Value()) {
        st_index = (GHR ^ ins_ptr) & (ST_SIZE - 1); // Gshare indexing
    } else {
        st_index = ins_ptr & (ST_SIZE - 1); // Naive indexing without GHR
    }

    ST_AccessCount[st_index]++; // Increment the access count for the accessed state table entry

    // Update the state of the ST entry based on the outcome
    if (taken) {
        if (ST[st_index].state < 3) ST[st_index].state++;
    } else {
        if (ST[st_index].state > 0) ST[st_index].state--;
    }

    // Now, shift the GHR and update with the new outcome
    GHR <<= 1; // Shift left
    GHR |= taken; // Set the LSB to the outcome of the branch
    GHR &= (1 << GHR_LENGTH) - 1; // Keep only the last GHR_LENGTH bits
}

/* ===================================================================== */

static INT32 Usage() {
    cerr << "This pin tool collects a profile of jump/branch/call instructions for an application\n";
    cerr << KNOB_BASE::StringKnobSummary();
    cerr << endl;
    return -1;
}

/* ===================================================================== */

VOID write_results(bool limit_reached) {
    string output_file = KnobOutputFile.Value();
    if (KnobPid) output_file += "." + decstr(getpid());
    
    std::ofstream out(output_file.c_str());

    if (limit_reached)
        out << "Reason: limit reached\n";
    else
        out << "Reason: fini\n";

    // Calculate and output accuracy
    double accuracy = (double)CountCorrect / (double)CountSeen;
    out << "Accuracy: " << std::fixed << std::setprecision(2) << accuracy * 100 << "%" << endl;
    out << "Count Seen: " << CountSeen << endl;
    out << "Count Taken: " << CountTaken << endl;
    out << "Count Correct: " << CountCorrect << endl;
    out.close();

    // Write state table access counts to a file
    string access_count_file = KnobOutputFile.Value() + ".access_count";
    if (KnobPid) access_count_file += "." + decstr(getpid());

    std::ofstream access_out(access_count_file.c_str());
    for (int i = 0; i < ST_SIZE; i++) {
        access_out << ST_AccessCount[i] << endl;
    }
    access_out.close();
}

/* ===================================================================== */

VOID br_predict(ADDRINT ins_ptr, INT32 taken, UINT32 branch_info) {
    // Count the number of branches seen
    CountSeen++;
    // Count the taken branches
    if (taken) {
        CountTaken++;
    }
    
    // Count the correctly predicted branches
    if (ST_predict(ins_ptr) == taken) {
        CountCorrect++;
    }

    // Update branch prediction tables
    ST_transition(ins_ptr, taken, branch_info);

    if (CountSeen == KnobBranchLimit.Value()) {
        write_results(true);
        exit(0);
    }
}

/* ===================================================================== */

VOID Instruction(INS ins, void *v) {
    if (INS_IsRet(ins)) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)br_predict, 
            IARG_INST_PTR, IARG_BRANCH_TAKEN, IARG_UINT32, 0x08, IARG_END);
    }
    // else if (INS_IsSyscall(ins)) {
    //     INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)br_predict, 
    //         IARG_INST_PTR, IARG_BRANCH_TAKEN, IARG_UINT32, 0x00, IARG_END);
    // } 
    else if (INS_IsBranch(ins)) {
        UINT32 branch_info = 0;
        if (INS_IsDirectBranch(ins)) {
            branch_info |= 0x01; // direct
            INT32 is_forward = (INS_DirectControlFlowTargetAddress(ins) > INS_Address(ins)) ? 1 : 0;
            branch_info |= (is_forward << 4);
        } else {
            branch_info |= 0x02; // indirect
            branch_info |= (0 << 4); // Set forward/backward bit to 0 (backward) for indirect branches
        }
        if (INS_IsCall(ins)) {
            branch_info |= 0x04; // call
        }
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)br_predict, 
            IARG_INST_PTR, IARG_BRANCH_TAKEN, IARG_UINT32, branch_info, IARG_END);
}
}

/* ===================================================================== */

VOID Fini(int n, void *v) {
    write_results(false);
}

/* ===================================================================== */

int main(int argc, char *argv[]) {
    if (PIN_Init(argc, argv)) {
        return Usage();
    }

    ST_init();
        
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    PIN_StartProgram();
    
    return 0;
}