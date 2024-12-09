#include "cycle.h"

#include <iostream>
#include <memory>
#include <string>

#include "Utilities.h"
#include "cache.h"
#include "emulator.h"

#include <iostream>
using namespace std;

// keep executed address at each pipelines step
struct PipeStateAddr
{
    uint32_t ifInstr_addr;
    uint32_t idInstr_addr;
    uint32_t exInstr_addr;
    uint32_t memInstr_addr;
    uint32_t wbInstr_addr;
};

static Emulator *emulator = nullptr;
static Cache *iCache = nullptr;
static Cache *dCache = nullptr;
static std::string output;
static Emulator::InstructionInfo info;
static uint32_t icache_delay;
static uint32_t dcache_delay;
static uint32_t branch_delay;
static uint32_t lastBranchCycleCount;
static PipeState pipeState;
static PipeStateAddr pipeStateAddr;
static bool HALTING; // is the halting instruction flowing thorugh the pipeline?
static uint32_t currentCycle;
static uint32_t nopArithmeticExcept;
static uint32_t nopIllegalInstruct;
static bool squashID;
static bool squashEX;
static bool nrLoadUseStalls;

// enum for specific instructions (i.e. halting)
enum Instructions
{
    HALT_INSTRUCTION = 0xfeedfeed,
};

// Control signals for pipeline
enum ControlSignal
{
    HALT_INSTRUCT_IN_PIPELINE = 0,
    FETCH_NEW = 1,
    I_CACHE_MISS = 2,
    D_CACHE_MISS = 3,
    LOAD_USE_STALL = 4,
    BRANCH_STALL = 5,
    ARITHMETIC_OVERFLOW = 6,
    ILLEGAL_INSTRUCION = 7,
};

// extract specific bits [start, end] from a 32 bit instruction
uint extractBitsAddr(uint32_t instruction, int start, int end)
{
    int bitsToExtract = start - end + 1;
    uint32_t mask = (1 << bitsToExtract) - 1;
    uint32_t clipped = instruction >> end;
    return clipped & mask;
}

// Helper function to check if a given opcode is a load function
bool isLoad(uint32_t OPCODE)
{
    return (OPCODE == OP_LBU || OPCODE == OP_LHU || OPCODE == OP_LW);
}

// Helper function to check if a given opcode is a store function
bool isStore(uint32_t OPCODE)
{
    return (OPCODE == OP_SB || OPCODE == OP_SH || OPCODE == OP_SW);
}

// Helper function to check if a given opcode is a branch function
bool isBranch(uint32_t OPCODE)
{
    return (OPCODE == OP_BLEZ || OPCODE == OP_BGTZ || OPCODE == OP_BEQ || OPCODE == OP_BNE);
}

// advances every part of the pipeline
void moveAllForward(PipeState &pipeline, PipeStateAddr &pipelineAddr)
{
    pipeline.wbInstr = pipeline.memInstr;
    pipeline.memInstr = pipeline.exInstr;
    pipeline.exInstr = pipeline.idInstr;
    pipeline.idInstr = pipeline.ifInstr;
    pipeline.ifInstr = 0;

    pipelineAddr.wbInstr_addr = pipelineAddr.memInstr_addr;
    pipelineAddr.memInstr_addr = pipelineAddr.exInstr_addr;
    pipelineAddr.exInstr_addr = pipelineAddr.idInstr_addr;
    pipelineAddr.idInstr_addr = pipelineAddr.ifInstr_addr;
    pipelineAddr.ifInstr_addr = 0;
}

// stalls only IF stage. This happens for icache misses but no dcache misses for instruction that is in id stage.
void stall_IF_stage(PipeState &pipeline, PipeStateAddr &pipelineAddr)
{
    pipeline.wbInstr = pipeline.memInstr;
    pipeline.memInstr = pipeline.exInstr;
    pipeline.exInstr = pipeline.idInstr;
    pipeline.idInstr = 0;

    pipelineAddr.wbInstr_addr = pipelineAddr.memInstr_addr;
    pipelineAddr.memInstr_addr = pipelineAddr.exInstr_addr;
    pipelineAddr.exInstr_addr = pipelineAddr.idInstr_addr;
    pipelineAddr.idInstr_addr = 0;
}

// Basically doesn't do anything so pipeline does not advance EXCEPT the WB stage which is a nop due to the rest waiting
void stall_IF_ID_EX_MEM_stage(PipeState &pipeline, PipeStateAddr &pipelineAddr)
{
    pipeline.wbInstr = 0;
    pipelineAddr.wbInstr_addr = 0;
    return;
}

void BRANCH_stall(PipeState &pipeline, PipeStateAddr &pipelineAddr)
{
    pipeline.wbInstr = pipeline.memInstr;
    pipeline.memInstr = pipeline.exInstr;
    pipeline.exInstr = 0;

    pipelineAddr.wbInstr_addr = pipelineAddr.memInstr_addr;
    pipelineAddr.memInstr_addr = pipelineAddr.exInstr_addr;
    pipelineAddr.exInstr_addr = 0;
}

void load_use_stall_stage(PipeState &pipeline, PipeStateAddr &pipelineAddr)
{
    pipeline.wbInstr = pipeline.memInstr;
    pipeline.memInstr = pipeline.exInstr;
    pipeline.exInstr = 0;

    pipelineAddr.wbInstr_addr = pipelineAddr.memInstr_addr;
    pipelineAddr.memInstr_addr = pipelineAddr.exInstr_addr;
    pipelineAddr.exInstr_addr = 0;
}

void insertNopException(PipeState &pipeline, PipeStateAddr &pipelineAddr)
{
    pipeline.wbInstr = pipeline.memInstr;
    pipeline.memInstr = pipeline.exInstr;
    pipeline.exInstr = pipeline.idInstr;
    pipeline.idInstr = pipeline.ifInstr;
    pipeline.ifInstr = 0;

    pipelineAddr.wbInstr_addr = pipelineAddr.memInstr_addr;
    pipelineAddr.memInstr_addr = pipelineAddr.exInstr_addr;
    pipelineAddr.exInstr_addr = pipelineAddr.idInstr_addr;
    pipelineAddr.idInstr_addr = pipelineAddr.ifInstr_addr;
    pipelineAddr.ifInstr_addr = 0;
}


// NOTE: The list of places in the source code that are marked ToDo might not be comprehensive.
// Please keep this in mind as you work on the project.

/**TODO:
 * Implement the pipeline emulation for the MIPS machine in this section.
 * A basic template is provided below but it doesn't account for all possible stalls and hazards as is
 */

// initialize the emulator
Status initSimulator(CacheConfig &iCacheConfig, CacheConfig &dCacheConfig, MemoryStore *mem,
                     const std::string &output_name)
{
    output = output_name;
    emulator = new Emulator();
    emulator->setMemory(mem);
    iCache = new Cache(iCacheConfig, I_CACHE);
    dCache = new Cache(dCacheConfig, D_CACHE);
    pipeState = {
        0,
    };
    pipeStateAddr = {
        0,
    };
    currentCycle = 0;
    HALTING = false;
    lastBranchCycleCount = 0;
    nopArithmeticExcept = 0;
    nopIllegalInstruct = 0;
    squashID = false;
    squashEX = false;
    return SUCCESS;
}

// Control Detection Unit: returns pipeline behavior
uint32_t Control(PipeState &pipeline)
{
    pipeline.cycle = currentCycle;
    ControlSignal state;

    // check if current state of pipeline requires branch stall (i.e. branch reg is in ex stage)
    uint32_t ID_OPCODE = extractBitsAddr(pipeline.idInstr, 31, 26);
    uint32_t EX_OPCODE = extractBitsAddr(pipeline.exInstr, 31, 26);

    uint32_t ID_RS = extractBitsAddr(pipeline.idInstr, 25, 21);
    uint32_t ID_RT = extractBitsAddr(pipeline.idInstr, 20, 16);
    // uint32_t EX_RS = extractBitsAddr(pipeline.exInstr, 25, 21);
    uint32_t EX_RT = extractBitsAddr(pipeline.exInstr, 20, 16);

    
    if (
        isBranch(ID_OPCODE) && 
        (ID_RS == EX_RT || ID_RT == EX_RT) &&
        EX_RT != 0 &&
        pipeline.exInstr != 0 &&
        lastBranchCycleCount != info.instructionID
    )
    {
        if (
            (ID_OPCODE == OP_BLEZ || ID_OPCODE == OP_BGTZ) &&
            ID_RS == EX_RT)
        {
            if (isLoad(EX_OPCODE))
            {
                nrLoadUseStalls += 1;
                branch_delay = 2;
            }
            else
            {
                branch_delay = 1;
            }
            // NOTE: I think this is necessary to make sure we don't keep getting into the if statement when we hit a branch but simultaneously a dcache miss. To not get stuck like doing 8 dcache latencymisses + 2 for branch instead of including the 2 in the 8.
            lastBranchCycleCount = info.instructionID;
        }
        
        // NOTE: For BEG and BNE we need to check rs and rt.
        if (
            (ID_OPCODE == OP_BEQ || ID_OPCODE == OP_BNE) &&
            (ID_RS == EX_RT || ID_RT == EX_RT))
        {
            if (isLoad(EX_OPCODE))
            {
                nrLoadUseStalls += 1;
                branch_delay = 2;
            }
            else
            {
                branch_delay = 1;
            }
            // see above
            lastBranchCycleCount = info.instructionID;
        }

        state = BRANCH_STALL;
    }

    // Load Use Stall:
    else if (
        isLoad(EX_OPCODE) && 
        (EX_RT == ID_RS || EX_RT == ID_RT) &&
        EX_RT != 0
        )
    {
        nrLoadUseStalls += 1;
        state = LOAD_USE_STALL;
    }
    // assign state to variable to decide which way to go in main loop
    // CHECK: what if it's a load brach stall but I_CACHE is >0? What should be done in that case? - Answer: I think these two should be independent
    else if (info.isOverflow || nopArithmeticExcept > 0)
    {
        state = ARITHMETIC_OVERFLOW;
        if (nopArithmeticExcept == 0)
        {
            nopArithmeticExcept = 2;
        }
    }
    else if (!info.isValid || nopIllegalInstruct > 0)
    {
        state = ILLEGAL_INSTRUCION;
        if (nopIllegalInstruct == 0)
        {
            nopIllegalInstruct = 1;
        }
    }
    else if (dcache_delay == 0 && branch_delay > 0 && !HALTING)
    {
        state = BRANCH_STALL;
    }
    else if ((dcache_delay == 0 && icache_delay == 0 && branch_delay == 0 && !HALTING) || squashID)
    {
        if (info.isHalt)
        {
            state = HALT_INSTRUCT_IN_PIPELINE;
        }
        else
        {
            state = FETCH_NEW;
        }
    }
    else if (dcache_delay == 0 && icache_delay > 0 && !HALTING)
    {
        state = I_CACHE_MISS;
    }
    else if (dcache_delay > 0 && !HALTING)
    {
        state = D_CACHE_MISS;
        if (branch_delay > 0) {
            branch_delay += 1;
        }
    }
    else
    {
        throw std::invalid_argument("Control got to an unknow situation. Please fix!");
    }

    // reduce delay by 1 with floor of 0 (do every time irrespective of what happened)
    icache_delay = (icache_delay == 0) ? 0 : icache_delay - 1;
    dcache_delay = (dcache_delay == 0) ? 0 : dcache_delay - 1;
    branch_delay = (branch_delay == 0) ? 0 : branch_delay - 1;
    return state;
}

void execute_DCACHE_write_Check(PipeState &pipeline, PipeStateAddr &pipelineAddr)
{
    // handle dCache delays (in a multicycle style)
    // NOTE: We are only reading/writing to Data Memory. Hence this is done in the MEM stage. The writeback stage is free as we can do forwarding I think so we dont have to worry about it (not 100% but I think so.)
    // instruction in stage MEM is the one reading if lw
    uint32_t MEM_OPCODE = extractBitsAddr(pipeline.memInstr, 31, 26);
    uint32_t MEM_ADDRESS = pipelineAddr.memInstr_addr;

    // uint32_t MEM_RT = extractBitsAddr(pipeline.exInstr, 20, 16);

    // instruction in stage MEM is the one writing if sw
    if (info.isValid && isStore(MEM_OPCODE))
    {
        dcache_delay += dCache->access(MEM_ADDRESS, CACHE_WRITE) ? 0 : dCache->config.missLatency;
    }

    if (info.isValid && isLoad(MEM_OPCODE)) // && MEM_RT != 0)
    {
        dcache_delay += dCache->access(MEM_ADDRESS, CACHE_READ) ? 0 : dCache->config.missLatency;
    }
}

// Run the emulator for a certain number of cycles
// return HALT if the simulator halts on 0xfeedfeed
// return SUCCESS if we have executed the desired number of cycles
Status runCycles(uint32_t cycles)
{
    uint32_t count = 0;
    auto status = SUCCESS;

    while (cycles == 0 || count < cycles)
    {
        uint32_t state = Control(pipeState);
        if (state == FETCH_NEW)
        {
            // here move all instructions one forward
            moveAllForward(pipeState, pipeStateAddr);

            if (squashID)
            {
                pipeState.exInstr = 0;
                pipeStateAddr.exInstr_addr = 0;
                squashID = false;
            }
            else if (squashEX)
            {
                pipeState.memInstr = 0;
                pipeStateAddr.memInstr_addr = 0;
                squashEX = false;
            }

            info = emulator->executeInstruction();
            pipeState.ifInstr = info.instruction;

            uint32_t IF_OPCODE = extractBitsAddr(pipeState.ifInstr, 31, 26);
            if (isStore(IF_OPCODE))
            {
                pipeStateAddr.ifInstr_addr = info.storeAddress;
            }

            if (isLoad(IF_OPCODE))
            {
                pipeStateAddr.ifInstr_addr = info.loadAddress;
            }

            // Conduct ICACHECHECK here. No need for function as we will only do when we fetch a new instruction.
            // current PC is the one reading from Icache
            icache_delay = iCache->access(info.pc, CACHE_READ) ? 0 : iCache->config.missLatency;

            // checks if current MEM stage misses or hits
            execute_DCACHE_write_Check(pipeState, pipeStateAddr);
        }
        else if (state == I_CACHE_MISS)
        {
            stall_IF_stage(pipeState, pipeStateAddr);

            // checks if current MEM stage misses or hits
            execute_DCACHE_write_Check(pipeState, pipeStateAddr);
        }
        else if (state == D_CACHE_MISS)
        {
            stall_IF_ID_EX_MEM_stage(pipeState, pipeStateAddr);
        }
        else if (state == LOAD_USE_STALL)
        {
            load_use_stall_stage(pipeState, pipeStateAddr);

            // checks if current MEM stage misses or hits
            execute_DCACHE_write_Check(pipeState, pipeStateAddr);
        }
        else if (state == BRANCH_STALL)
        {
            BRANCH_stall(pipeState, pipeStateAddr);
            execute_DCACHE_write_Check(pipeState, pipeStateAddr);
        }
        else if (state == HALT_INSTRUCT_IN_PIPELINE)
        {
            moveAllForward(pipeState, pipeStateAddr);

            // checks if current MEM stage misses or hits
            execute_DCACHE_write_Check(pipeState, pipeStateAddr);
        }
        else if (state == ARITHMETIC_OVERFLOW)
        {
            if (icache_delay > 0) {
                stall_IF_stage(pipeState, pipeStateAddr);
            } else {
                insertNopException(pipeState, pipeStateAddr);
                nopArithmeticExcept -= 1;
                if (nopArithmeticExcept == 0)
                {
                    squashEX = true;
                }
                info.isOverflow = false;
            }
            

            // checks if current MEM stage misses or hits
            execute_DCACHE_write_Check(pipeState, pipeStateAddr);
        }
        else if (state == ILLEGAL_INSTRUCION)
        {
            if (icache_delay > 0) {
                stall_IF_stage(pipeState, pipeStateAddr);
            } else {
                insertNopException(pipeState, pipeStateAddr);
                nopIllegalInstruct -= 1;
                if (nopIllegalInstruct == 0)
                {
                    squashID = true;
                }
                info.isValid = true;
            }
            // checks if current MEM stage misses or hits
            execute_DCACHE_write_Check(pipeState, pipeStateAddr);
        }

        count += 1;
        currentCycle += 1;

        if (pipeState.wbInstr == HALT_INSTRUCTION)
        {
            status = HALT;
            break;
        }
    }

    dumpPipeState(pipeState, output);
    return status;
}

// run till halt (call runCycles() with cycles == 1 each time) until
// status tells you to HALT or ERROR out
Status runTillHalt()
{
    Status status;
    uint32_t t = 0;
    while (true)
    {
        status = static_cast<Status>(runCycles(1));
        if (status == HALT)
            break;
        t += 1;
    }
    return status;
}

// dump the state of the emulator
Status finalizeSimulator()
{
    emulator->dumpRegMem(output);
    SimulationStats stats{emulator->getDin(), currentCycle, iCache->getHits(), iCache->getMisses(), dCache->getHits(), dCache->getMisses(), nrLoadUseStalls};
    // TODO: Incomplete Implementation
    // [x]: Load use Stalls
    dumpSimStats(stats, output);
    return SUCCESS;
}
