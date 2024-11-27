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
static uint32_t ICACHE_DELAY;
static uint32_t DCACHE_DELAY;
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
};

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
    return SUCCESS;
}

// advances every part of the pipeline
void moveAllForward(PipeState &pipline)
{
    pipline.wbInstr = pipline.memInstr;
    pipline.memInstr = pipline.exInstr;
    pipline.exInstr = pipline.idInstr;
    pipline.idInstr = pipline.ifInstr;
    pipline.ifInstr = 0;
}

// stalls only IF stage. This happens for icache misses but no dcache misses for instruction that is in id stage.
void stall_IF_stage(PipeState &pipline)
{
    pipline.wbInstr = pipline.memInstr;
    pipline.memInstr = pipline.exInstr;
    pipline.exInstr = pipline.idInstr;
    pipline.idInstr = 0;
}

// Basically doesn't do anything so pipeline does not advance EXCEPT the WB stage which is a nop due to the rest waiting
void stall_IF_ID_EX_MEM_stage(PipeState &pipline)
{
    pipline.wbInstr = 0;
    return;
}

// Control Detection Unit: returns pipeline behavior
uint32_t Control(PipeState &pipline)
{
    uint32_t currentCycle = emulator->getCurCyle();
    pipline.cycle = currentCycle;
    ControlSignal state;
    // 8. 0xfeedfeed instruction completes its IF stage, instruction fetch should stop fetching instructions and insert NOPs. You should halt execution when the 0xfeedfeed has completed its WB stage
    if (info.isHalt)
    {
        state = HALT_INSTRUCT_IN_PIPELINE;
    }
    else if (DCACHE_DELAY == 0 && ICACHE_DELAY == 0 && !HALTING)
    {
        state = FETCH_NEW;
    }
    else if (DCACHE_DELAY == 0 && ICACHE_DELAY > 0 && !HALTING)
    {
        state = I_CACHE_MISS;
    }
    else if (DCACHE_DELAY > 0 && !HALTING)
    {
        state = D_CACHE_MISS;
    }
    else
    {
        throw std::invalid_argument("Control got to an unknow situation. Please fix!");
    }

    // reduce delay by 1 with floor of 0 (do every time irrespective of what happened)
    ICACHE_DELAY = (ICACHE_DELAY == 0) ? 0 : ICACHE_DELAY - 1;
    DCACHE_DELAY = (DCACHE_DELAY == 0) ? 0 : DCACHE_DELAY - 1;
    return state;
}

// helper function to get opcode and address (copied from emulator.cpp)
uint32_t extractBits(uint32_t instruction, int start, int end)
{
    int bitsToExtract = start - end + 1;
    uint32_t mask = (1 << bitsToExtract) - 1;
    uint32_t clipped = instruction >> end;
    return clipped & mask;
}

void execute_DCACHE_Check(PipeState &pipline)
{
    // handle dCache delays (in a multicycle style)
    // NOTE: We are only reading/writing to Data Memory. Hence this is done in the MEM stage. The writeback stage is free as we can do forwarding I think so we dont have to worry about it (not 100% but I think so.)
    // instruction in stage MEM is the one reading if lw
    uint32_t MEM_OPCODE = extractBits(pipline.memInstr, 31, 26);
    uint32_t MEM_ADDRESS = extractBits(pipline.memInstr, 25, 0);

    // TODO: check if this should be info.isValid or the mem stage on if.valid
    if (info.isValid && (MEM_OPCODE == OP_LBU || MEM_OPCODE == OP_LHU || MEM_OPCODE == OP_LW))
        DCACHE_DELAY += dCache->access(MEM_ADDRESS, CACHE_READ) ? 0 : dCache->config.missLatency;

    // instruction in stage MEM is the one writing if sw
    if (info.isValid && (MEM_OPCODE == OP_SB || MEM_OPCODE == OP_SH || MEM_OPCODE == OP_SW))
        DCACHE_DELAY += dCache->access(MEM_ADDRESS, CACHE_WRITE) ? 0 : dCache->config.missLatency;
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

            // Conduct ICACHECHECK here. No need for function as we will only do when we fetch a new instruction.
            // current PC is the one reading from Icache
            ICACHE_DELAY = iCache->access(info.pc, CACHE_READ) ? 0 : iCache->config.missLatency;

            // checks if current MEM stage misses or hits
            execute_DCACHE_Check(pipeState);
        }
        else if (state == I_CACHE_MISS)
        {
            stall_IF_stage(pipeState);

            // checks if current MEM stage misses or hits
            execute_DCACHE_Check(pipeState);
        }
        else if (state == D_CACHE_MISS)
        {
            stall_IF_ID_EX_MEM_stage(pipeState);
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
