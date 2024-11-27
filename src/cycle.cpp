#include "cycle.h"

#include <iostream>
#include <memory>
#include <string>

#include "Utilities.h"
#include "cache.h"
#include "emulator.h"

static Emulator *emulator = nullptr;
static Cache *iCache = nullptr;
static Cache *dCache = nullptr;
static std::string output;
static Emulator::InstructionInfo info;
static uint32_t IF_DELAY;
static PipeState pipeState;
static bool HALTING; // is the halting instruction flowing thorugh the pipeline?

// enum for specific instructions (i.e. halting)
enum Instructions
{
    HALT_INSTRUCTION = 0xfeedfeed,
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

void moveAllForward(PipeState &pipline)
{
    pipline.wbInstr = pipline.memInstr;
    pipline.memInstr = pipline.exInstr;
    pipline.exInstr = pipline.idInstr;
    pipline.idInstr = pipline.ifInstr;
    pipline.ifInstr = 0;
}

void stall_IF_stage(PipeState &pipline)
{
    pipline.wbInstr = pipline.memInstr;
    pipline.memInstr = pipline.exInstr;
    pipline.exInstr = pipline.idInstr;
    pipline.idInstr = 0;
}

enum ControlSignal
{
    HALT_INSTRUCT_IN_PIPELINE = 0,
    FETCH_NEW = 100,
    IF_CACHE_MISS = 200,
};

// Control Detection Unit: returns pipeline behavior
uint32_t Control(PipeState &pipline)
{
    uint32_t currentCycle = emulator->getCurCyle();
    pipline.cycle = currentCycle;
    if (info.isHalt) {
        return HALT_INSTRUCT_IN_PIPELINE;
    } else if (IF_DELAY == 0 && !HALTING)
    {
        return FETCH_NEW;
    } else if (IF_DELAY > 0 && !HALTING) {
        IF_DELAY -= 1;
        return IF_CACHE_MISS;
    }

    if (false)
    {
        // handle dCache delays (in a multicycle style)
        uint32_t cacheDelay;
        if (info.isValid && (info.opcode == OP_LBU || info.opcode == OP_LHU || info.opcode == OP_LW))
            cacheDelay += dCache->access(info.address, CACHE_READ) ? 0 : dCache->config.missLatency;

        if (info.isValid && (info.opcode == OP_SB || info.opcode == OP_SH || info.opcode == OP_SW))
            cacheDelay += dCache->access(info.address, CACHE_WRITE) ? 0 : dCache->config.missLatency;
    }

    return 0;
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

            // handle iCache delay
            IF_DELAY = iCache->access(info.pc, CACHE_READ) ? 0 : iCache->config.missLatency;
        }
        else if (state == IF_CACHE_MISS) {
            stall_IF_stage(pipeState);
        } else if (state == HALT_INSTRUCT_IN_PIPELINE) {
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
    while (true)
    {
        status = static_cast<Status>(runCycles(1));
        if (status == HALT)
            break;
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
