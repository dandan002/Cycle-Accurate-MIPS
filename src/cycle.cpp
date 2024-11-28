#include "cycle.h"

#include <iostream>
#include <memory>
#include <string>

#include "Utilities.h"
#include "cache.h"
#include "emulator.h"

#include <iostream>
using namespace std;

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
static bool HALTING; // is the halting instruction flowing thorugh the pipeline?

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
};

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
    HALTING = false;
    lastBranchCycleCount = 0;
    return SUCCESS;
}

// Control Detection Unit: returns pipeline behavior
uint32_t Control(PipeState &pipeline)
{
    uint32_t currentCycle = emulator->getCurCyle();
    pipeline.cycle = currentCycle;
    ControlSignal state;

    // check if current state of pipeline requires branch stall (i.e. branch reg is in ex stage)
    uint32_t ID_OPCODE = emulator->extractBits(pipeline.idInstr, 31, 26);
    uint32_t EX_OPCODE = emulator->extractBits(pipeline.exInstr, 31, 26);

    uint32_t ID_RS = emulator->extractBits(pipeline.idInstr, 25, 21);
    uint32_t EX_RS = emulator->extractBits(pipeline.exInstr, 25, 21);

    // check if nop after branch is needed (if Ex state write to same place as )
    // CHECK: Do we need to include BNE, BEQ here as well?
    // CHECK: Other dependencies here? Can we just assume if not load -> arithmetic?
    if ((ID_OPCODE == OP_BLEZ || ID_OPCODE == OP_BGTZ) && ID_RS == EX_RS && lastBranchCycleCount != info.instructionID)
    {
        if (isLoad(EX_OPCODE))
        {
            branch_delay = 2;
        }
        else
        {
            branch_delay = 1;
        }
        // NOTE: I think this is necessary to make sure we don't keep getting into the if statement when we hit a branch but simultaneously a dcache miss. To not get stuck like doing 8 dcache latencymisses + 2 for branch instead of including the 2 in the 8.
        lastBranchCycleCount = info.instructionID;
    }

    // assign state to variable to decide which way to go in main loop
    // CHECK: what if it's a load brach stall but I_CACHE is >0? What should be done in that case? - Answer: I think these two should be independent
    if (info.isHalt)
    {
        state = HALT_INSTRUCT_IN_PIPELINE;
    }
    else if (dcache_delay == 0 && branch_delay > 0 && !HALTING)
    {
        state = BRANCH_STALL;
    }
    else if (dcache_delay == 0 && icache_delay == 0 && branch_delay == 0 && !HALTING)
    {
        state = FETCH_NEW;
    }
    else if (dcache_delay == 0 && icache_delay > 0 && !HALTING)
    {
        state = I_CACHE_MISS;
    }
    else if (dcache_delay > 0 && !HALTING)
    {
        state = D_CACHE_MISS;
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

void execute_DCACHE_write_Check(PipeState &pipeline)
{
    // handle dCache delays (in a multicycle style)
    // NOTE: We are only reading/writing to Data Memory. Hence this is done in the MEM stage. The writeback stage is free as we can do forwarding I think so we dont have to worry about it (not 100% but I think so.)
    // instruction in stage MEM is the one reading if lw
    uint32_t MEM_OPCODE = emulator->extractBits(pipeline.memInstr, 31, 26);
    uint32_t MEM_ADDRESS = pipeline.memInstr_addr;

    // instruction in stage MEM is the one writing if sw
    if (info.isValid && isStore(MEM_OPCODE))
    {
        dcache_delay += dCache->access(MEM_ADDRESS, CACHE_WRITE) ? 0 : dCache->config.missLatency;
    }

    if (info.isValid && isLoad(MEM_OPCODE))
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
            moveAllForward(pipeState);

            info = emulator->executeInstruction();
            pipeState.ifInstr = info.instruction;

            uint32_t IF_OPCODE = emulator->extractBits(pipeState.ifInstr, 31, 26);
            if (isStore(IF_OPCODE))
            {
                pipeState.ifInstr_addr = info.storeAddress;
            }

            if (isLoad(IF_OPCODE))
            {
                pipeState.ifInstr_addr = info.loadAddress;
            }

            // Conduct ICACHECHECK here. No need for function as we will only do when we fetch a new instruction.
            // current PC is the one reading from Icache
            icache_delay = iCache->access(info.pc, CACHE_READ) ? 0 : iCache->config.missLatency;

            // checks if current MEM stage misses or hits
            execute_DCACHE_write_Check(pipeState);
        }
        else if (state == I_CACHE_MISS)
        {
            stall_IF_stage(pipeState);

            // checks if current MEM stage misses or hits
            execute_DCACHE_write_Check(pipeState);
        }
        else if (state == D_CACHE_MISS)
        {
            stall_IF_ID_EX_MEM_stage(pipeState);
        }
        else if (state == BRANCH_STALL)
        {
            BRANCH_stall(pipeState);
        }
        else if (state == HALT_INSTRUCT_IN_PIPELINE)
        {
            moveAllForward(pipeState);
        }

        count += 1;
        emulator->ranCyle();

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
    SimulationStats stats{emulator->getDin(), emulator->getCurCyle(), iCache->getHits(), iCache->getMisses(), dCache->getHits(), dCache->getMisses()}; // TODO: Incomplete Implementation
    dumpSimStats(stats, output);
    return SUCCESS;
}
