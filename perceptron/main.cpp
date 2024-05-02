#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <cmath>
#include "pin.H"

using namespace std;

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,    "pintool",
    "o", "perceptron_predictor.out", "specify output file name");

KNOB<BOOL>   KnobPid(KNOB_MODE_WRITEONCE,                "pintool",
    "i", "0", "append pid to output");

KNOB<UINT64> KnobNumPerceptrons(KNOB_MODE_WRITEONCE,         "pintool",
    "p", "1024", "specify number of perceptrons");

KNOB<UINT64> KnobGHRLength(KNOB_MODE_WRITEONCE,         "pintool",
    "g", "24", "specify global history register length");

KNOB<UINT64> KnobLHRLength(KNOB_MODE_WRITEONCE,         "pintool",
    "l", "8", "specify local history register length");

KNOB<UINT64> KnobLHTSize(KNOB_MODE_WRITEONCE,         "pintool",
    "s", "1024", "specify size of local history table");

KNOB<UINT64> KnobHashingScheme(KNOB_MODE_WRITEONCE,         "pintool",
    "x", "3", "specify hashing scheme (1-9)");

KNOB<UINT64> KnobConfidenceEstimation(KNOB_MODE_WRITEONCE,         "pintool",
    "q", "0", "enable confidence estimation (0: disable, 1: enable)");

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */

UINT64 branch_count = 0;
UINT64 mis_predict = 0;

INT8 *perceptron_table;
UINT64 *lht;

UINT64 num_perceptrons;
UINT64 ghr_length;
UINT64 lhr_length;
UINT64 lht_size;
INT64 threshold;
UINT64 hashing_scheme;

UINT64 GHR; // Global History Register

UINT64 total_predictions = 0;
UINT64 correct_predictions = 0;
INT64 perceptron_magnitude = 0;
UINT64 saturation_counter = 0;

std::ofstream confidence_file;

/* ===================================================================== */
/* Helper Functions */
/* ===================================================================== */

// Compute perceptron output
INT64 perceptron_predict(ADDRINT pc, UINT64 local_history) {
    INT64 yout = 0;
    UINT64 perceptron_index;

    switch (hashing_scheme) {
        case 1:
            perceptron_index = pc % num_perceptrons;
            break;
        case 2:
            perceptron_index = (pc ^ GHR) % num_perceptrons;
            break;
        case 3:
            perceptron_index = local_history % num_perceptrons;
            break;
        case 4:
            perceptron_index = pc % num_perceptrons;
            break;
        case 5:
            perceptron_index = GHR % num_perceptrons;
            break;
        case 6:
            perceptron_index = (pc ^ local_history) % num_perceptrons;
            break;
        case 7:
            perceptron_index = (pc ^ GHR) % num_perceptrons;
            break;
        case 8:
            perceptron_index = (local_history ^ GHR) % num_perceptrons;
            break;
        case 9:
            perceptron_index = (pc ^ local_history ^ GHR) % num_perceptrons;
            break;
        default:
            perceptron_index = (pc ^ GHR) % num_perceptrons;
            break;
    }

    INT8* perceptron = &perceptron_table[perceptron_index * (ghr_length + lhr_length + 1)];

    for (UINT64 i=0; i<ghr_length; i++) {
        yout += perceptron[i] * ((GHR >> i) & 1 ? 1 : -1);
    }
    for (UINT64 i=0; i<lhr_length; i++) {
        yout += perceptron[ghr_length + i] * ((local_history >> i) & 1 ? 1 : -1);
    }
    yout += perceptron[ghr_length + lhr_length] * 1; // bias

    return yout;
}

// Train perceptron
void train(ADDRINT pc, bool taken, INT64 yout, UINT64 local_history) {
    INT64 t = taken ? 1 : -1;
    UINT64 perceptron_index;

    switch (hashing_scheme) {
        case 1:
            perceptron_index = pc % num_perceptrons;
            break;
        case 2:
            perceptron_index = (pc ^ GHR) % num_perceptrons;
            break;
        case 3:
            perceptron_index = local_history % num_perceptrons;
            break;
        case 4:
            perceptron_index = pc % num_perceptrons;
            break;
        case 5:
            perceptron_index = GHR % num_perceptrons;
            break;
        case 6:
            perceptron_index = (pc ^ local_history) % num_perceptrons;
            break;
        case 7:
            perceptron_index = (pc ^ GHR) % num_perceptrons;
            break;
        case 8:
            perceptron_index = (local_history ^ GHR) % num_perceptrons;
            break;
        case 9:
            perceptron_index = (pc ^ local_history ^ GHR) % num_perceptrons;
            break;
        default:
            perceptron_index = (pc ^ GHR) % num_perceptrons;
            break;
    }

    INT8* perceptron = &perceptron_table[perceptron_index * (ghr_length + lhr_length + 1)];

    if (abs(yout) <= threshold) {
        for (UINT64 i=0; i<ghr_length; i++) {
            perceptron[i] += t * ((GHR >> i) & 1 ? 1 : -1);
        }        
        for (UINT64 i=0; i<lhr_length; i++) {
            perceptron[ghr_length + i] += t * ((local_history >> i) & 1 ? 1 : -1);
        }
        perceptron[ghr_length + lhr_length] += t*1; // update bias
    }
}

void update_confidence(bool correct_prediction, INT64 yout) {
    total_predictions++;
    if (correct_prediction) {
        correct_predictions++;
    }
    perceptron_magnitude += abs(yout);
    if (correct_prediction) {
        saturation_counter = (saturation_counter < 3) ? saturation_counter + 1 : 3;
    } else {
        saturation_counter = (saturation_counter > 0) ? saturation_counter - 1 : 0;
    }

    if (KnobConfidenceEstimation.Value() == 1) {
        double accuracy = (double)correct_predictions / total_predictions;
        double avg_magnitude = (double)perceptron_magnitude / total_predictions;
        confidence_file << total_predictions << "," << accuracy << "," << avg_magnitude << "," << saturation_counter << std::endl;
    }
}

void br_predict(ADDRINT pc, bool taken, UINT32 br_info)
{
    branch_count++;
    
    UINT64 lht_index;
    switch (hashing_scheme) {
        case 1:
        case 2:
        case 4:
        case 6:
        case 7:
        case 9:
            lht_index = pc % lht_size;
            break;
        case 3:
        case 5:
        case 8:
            lht_index = (pc ^ GHR) % lht_size;
            break;
        default:
            lht_index = (pc ^ GHR) % lht_size;
            break;
    }

    UINT64 local_history = lht[lht_index];

    INT64 yout = perceptron_predict(pc, local_history);
    bool prediction = (yout >= 0);

    if (prediction != taken) mis_predict++;
    
    train(pc, taken, yout, local_history);
    
    bool correct_prediction = (prediction == taken);
    if (KnobConfidenceEstimation.Value() == 1) {
        update_confidence(correct_prediction, yout);
    }
    
    // Update global and local history registers
    GHR = (GHR << 1) | (taken ? 1 : 0);
    GHR &= ((1ULL << ghr_length) - 1);
    lht[lht_index] = ((local_history << 1) | (taken ? 1 : 0)) & ((1ULL << lhr_length) - 1);
}

/* ===================================================================== */

void write_results(bool error)
{
    char filename[100];
    snprintf(filename, 100, "%s", KnobOutputFile.Value().c_str());
    if (KnobPid)
        snprintf(filename, 100, "%s.%d", filename, PIN_GetPid());
    
    std::ofstream out(filename);

    if (error) {
        out << "ERROR: Limit Reached!" << endl; 
    }

    double accuracy = 100 - (100.0 * (double)mis_predict / branch_count);

    out << "Branches:         " << branch_count << endl;
    out << "Mispredictions:      " << mis_predict << endl;
    out << "Misprediction Rate:  " << setprecision(2) << fixed << 
           100.0 * (double)mis_predict / branch_count << "%" << endl;
    out << "Accuracy:        " << setprecision(2) << fixed << accuracy << "%" << endl;

    // Output the parameters
    out << "NUM_PERCEPTRONS: " << num_perceptrons << endl;
    out << "GHR_LENGTH: " << ghr_length << endl;
    out << "LHR_LENGTH: " << lhr_length << endl;
    out << "LHT_SIZE: " << lht_size << endl;
    out << "HASHING_SCHEME: " << hashing_scheme << endl;

    out.close();
}

/* ===================================================================== */
/* Instrumentation Routines */
/* ===================================================================== */

// PIN calls this function every time a new instruction is encountered
VOID Instruction(INS ins, void* v) 
{
    if (INS_IsRet(ins)) 
    {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)br_predict,
                       IARG_INST_PTR, IARG_BRANCH_TAKEN, IARG_UINT32, 0x08, IARG_END);
    }
    else if (INS_IsBranch(ins)) 
    {
        UINT32 br_info = 0;
        if (INS_IsDirectBranch(ins)) {
            br_info |= 0x01; // direct
            INT32 is_forward = (INS_DirectControlFlowTargetAddress(ins) > INS_Address(ins)) ? 1 : 0;
            br_info |= (is_forward << 4);
        } 
        else {
            br_info |= 0x02; // indirect
            br_info |= (0 << 4); // Set forward/backward bit to 0 (backward) for indirect branches
        }
        if (INS_IsCall(ins)) {
            br_info |= 0x04; // call
        }
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)br_predict,
                       IARG_INST_PTR, IARG_BRANCH_TAKEN, IARG_UINT32, br_info, IARG_END);
    }
}

/* ===================================================================== */

VOID Fini(int n, void* v)
{
    write_results(false);
    delete[] perceptron_table;
    delete[] lht;

    if (KnobConfidenceEstimation.Value() == 1) {
        confidence_file.close();
    }
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    PIN_InitSymbols();
    if (PIN_Init(argc, argv)) return 1;

    num_perceptrons = KnobNumPerceptrons.Value();
    ghr_length = KnobGHRLength.Value();
    lhr_length = KnobLHRLength.Value();
    lht_size = KnobLHTSize.Value();
    hashing_scheme = KnobHashingScheme.Value();

    // Check if LHR_LENGTH is less than or equal to 64
    if (lhr_length > 64) {
        cerr << "Error: LHR_LENGTH cannot be greater than 64." << endl;
        return 1;
    }

    // Check if HASHING_SCHEME is within the valid range
    if (hashing_scheme < 1 || hashing_scheme > 9) {
        cerr << "Error: HASHING_SCHEME must be between 1 and 9 (inclusive)." << endl;
        return 1;
    }

    // Calculate the threshold based on the number of features
    UINT64 num_features = ghr_length + lhr_length;
    threshold = static_cast<INT64>(1.93 * num_features + 14);

    perceptron_table = new INT8[num_perceptrons * (ghr_length + lhr_length + 1)];
    lht = new UINT64[lht_size];

    // Initialize perceptron table to small random values
    for (UINT64 i=0; i<num_perceptrons; i++) {
        for (UINT64 j=0; j<ghr_length + lhr_length + 1; j++) {
            perceptron_table[i * (ghr_length + lhr_length + 1) + j] = (rand() % 3) - 1;  // Random value in {-1, 0, 1}
        }
    }

    // Initialize local history table to all zeros
    for (UINT64 i=0; i<lht_size; i++) {
        lht[i] = 0;
    }

    if (KnobConfidenceEstimation.Value() == 1) {
        confidence_file.open("confidence_data.csv");
        confidence_file << "Prediction,Accuracy,AvgMagnitude,SaturationCounter" << std::endl;
    }

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}
