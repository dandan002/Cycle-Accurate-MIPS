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
    BRANCH_STALL = 4,
};

// helper function to get opcode and address (copied from emulator.cpp)
uint32_t extractBits(uint32_t instruction, int start, int end)
{
    int bitsToExtract = start - end + 1;
    uint32_t mask = (1 << bitsToExtract) - 1;
    uint32_t clipped = instruction >> end;
    return clipped & mask;
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
    return SUCCESS;
}

// advances every part of the pipeline
void moveAllForward(PipeState &pipeline)
{
    pipeline.wbInstr = pipeline.memInstr;
    pipeline.memInstr = pipeline.exInstr;
    pipeline.exInstr = pipeline.idInstr;
    pipeline.idInstr = pipeline.ifInstr;
    pipeline.ifInstr = 0;

    pipeline.wbInstr_addr = pipeline.memInstr_addr;
    pipeline.memInstr_addr = pipeline.exInstr_addr;
    pipeline.exInstr_addr = pipeline.idInstr_addr;
    pipeline.idInstr_addr = pipeline.ifInstr_addr;
    pipeline.ifInstr_addr = 0;
}

// stalls only IF stage. This happens for icache misses but no dcache misses for instruction that is in id stage.
void stall_IF_stage(PipeState &pipeline)
{
    pipeline.wbInstr = pipeline.memInstr;
    pipeline.memInstr = pipeline.exInstr;
    pipeline.exInstr = pipeline.idInstr;
    pipeline.idInstr = 0;

    pipeline.wbInstr_addr = pipeline.memInstr_addr;
    pipeline.memInstr_addr = pipeline.exInstr_addr;
    pipeline.exInstr_addr = pipeline.idInstr_addr;
    pipeline.idInstr_addr = 0;
}

// Basically doesn't do anything so pipeline does not advance EXCEPT the WB stage which is a nop due to the rest waiting
void stall_IF_ID_EX_MEM_stage(PipeState &pipeline)
{
    pipeline.wbInstr = 0;
    pipeline.wbInstr_addr = 0;
    return;
}

void stall_ID_BRACH_stage(PipeState &pipeline)
{
    pipeline.wbInstr = pipeline.memInstr;
    pipeline.memInstr = pipeline.exInstr;
    pipeline.exInstr = 0;

    pipeline.wbInstr_addr = pipeline.memInstr_addr;
    pipeline.memInstr_addr = pipeline.exInstr_addr;
    pipeline.exInstr_addr = 0;
}

// Control Detection Unit: returns pipeline behavior
uint32_t Control(PipeState &pipeline)
{
    uint32_t currentCycle = emulator->getCurCyle();
    pipeline.cycle = currentCycle;
    ControlSignal state;
    bool BRANCH_NOP = false;

    // check if current state of pipeline requires branch stall (i.e. branch reg is in ex stage)
    uint32_t ID_OPCODE = extractBits(pipeline.idInstr, 31, 26);
    uint32_t ID_RS = extractBits(pipeline.idInstr, 25, 21);
    uint32_t EX_RS = extractBits(pipeline.exInstr, 25, 21);

    // check if nop after branch is needed (if Ex state write to same place as )
    if (info.isValid && (ID_OPCODE == OP_BLEZ || ID_OPCODE == OP_BGTZ) && ID_RS == EX_RS)
    {
        BRANCH_NOP = true;
    }

    // assign state to variable to decide which way to go in main loop
    if (info.isHalt)
    {
        state = HALT_INSTRUCT_IN_PIPELINE;
    }
    else if (DCACHE_DELAY == 0 && ICACHE_DELAY == 0 && !BRANCH_NOP && !HALTING)
    {
        state = FETCH_NEW;
    }
    else if (DCACHE_DELAY == 0 && ICACHE_DELAY == 0 && BRANCH_NOP && !HALTING)
    {
        state = BRANCH_STALL;
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

void execute_DCACHE_write_Check(PipeState &pipeline)
{
    // handle dCache delays (in a multicycle style)
    // NOTE: We are only reading/writing to Data Memory. Hence this is done in the MEM stage. The writeback stage is free as we can do forwarding I think so we dont have to worry about it (not 100% but I think so.)
    // instruction in stage MEM is the one reading if lw
    uint32_t MEM_OPCODE = extractBits(pipeline.memInstr, 31, 26);
    uint32_t MEM_ADDRESS = pipeline.memInstr_addr;

    // instruction in stage MEM is the one writing if sw
    if (info.isValid && (MEM_OPCODE == OP_SB || MEM_OPCODE == OP_SH || MEM_OPCODE == OP_SW))
    {
        DCACHE_DELAY += dCache->access(MEM_ADDRESS, CACHE_WRITE) ? 0 : dCache->config.missLatency;
    }

    // TODO: check if this should be info.isValid or the mem stage on if.valid
    if (info.isValid && (MEM_OPCODE == OP_LBU || MEM_OPCODE == OP_LHU || MEM_OPCODE == OP_LW))
    {
        DCACHE_DELAY += dCache->access(MEM_ADDRESS, CACHE_READ) ? 0 : dCache->config.missLatency;
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

            uint32_t IF_OPCODE = extractBits(pipeState.ifInstr, 31, 26);
            if (IF_OPCODE == OP_SB || IF_OPCODE == OP_SH || IF_OPCODE == OP_SW)
            {
                pipeState.ifInstr_addr = info.storeAddress;
            }

            if ((IF_OPCODE == OP_LBU || IF_OPCODE == OP_LHU || IF_OPCODE == OP_LW))
            {
                pipeState.ifInstr_addr = info.loadAddress;
            }

            // Conduct ICACHECHECK here. No need for function as we will only do when we fetch a new instruction.
            // current PC is the one reading from Icache
            ICACHE_DELAY = iCache->access(info.pc, CACHE_READ) ? 0 : iCache->config.missLatency;

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
            stall_ID_BRACH_stage(pipeState);
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
