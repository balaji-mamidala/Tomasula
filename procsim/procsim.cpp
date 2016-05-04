#include "procsim.hpp"
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

using namespace std;

#define DEBUG 0
#define FU_nos 3
#define REG_FILE_SIZE 128

uint32_t inst_no;
uint32_t current_cycle;

uint16_t fetchQ_max_size;
uint16_t schedQ_max_size;
uint16_t FU_size[FU_nos];
uint16_t result_bus_size;


vector<fetch_disp_struct> fetchQ;
vector<fetch_disp_struct> dispQ;
vector<sched_struct> schedQ;

scoreboard_struct **scoreboard;

result_bus_struct *result_bus;

reg_file_struct *reg_file;


vector<inst_cycles_struct> inst_cycles;

uint64_t total_dispQ_size;

// Initialize global variable
void init_global_var(uint64_t r, uint64_t k0, uint64_t k1, uint64_t k2, uint64_t f)
{
  inst_no         = 0;
  current_cycle   = 0;

  fetchQ_max_size = f;

  FU_size[0]      = k0;
  FU_size[1]      = k1;
  FU_size[2]      = k2;

  schedQ_max_size = 2*(FU_size[0] + FU_size[1] + FU_size[2]);

  result_bus_size = r;
}

// Allocate and initialize memory for scoreboard
void alloc_init_scoreboard()
{
  scoreboard = (struct scoreboard_struct**) malloc(FU_nos * sizeof(scoreboard_struct*));

  for(uint16_t i=0; i<FU_nos; i++)
  {
    scoreboard[i] = (struct scoreboard_struct*) malloc(FU_size[i] * sizeof(scoreboard_struct));
  }

  for(uint16_t i=0; i<FU_nos; i++)
  {
    for(uint16_t j=0; j<FU_size[i]; j++)
    {
      scoreboard[i][j].busy        = false;
      scoreboard[i][j].reg_no      = -1;
      scoreboard[i][j].reg_tag     = -1;
      scoreboard[i][j].added_cycle = 0;
    }
  }
}

// Allocate and initialize memory for result bus
void alloc_init_result_bus()
{
  result_bus = (struct result_bus_struct*) malloc(result_bus_size * sizeof(result_bus_struct));

  for(uint16_t i=0; i<result_bus_size; i++)
  {
    result_bus[i].busy        = false;
    result_bus[i].reg_no      = -1;
    result_bus[i].reg_tag     = -1;
  }

}

// Allocate and initialize memory for reg file
void alloc_init_reg_file()
{
  reg_file = (struct reg_file_struct*) malloc(REG_FILE_SIZE * sizeof(reg_file_struct));

  for(uint16_t i=0; i<REG_FILE_SIZE; i++)
  {
    reg_file[i].ready   = true;
    reg_file[i].reg_tag = -1;
  }
}

void add_inst_cycles()
{
  inst_cycles_struct temp;

  temp.fetch_cycle = 0;
  temp.disp_cycle  = 0;
  temp.sched_cycle = 0;
  temp.ex_cycle    = 0;
  temp.su_cycle    = 0;

  inst_cycles.push_back(temp);
}

void inst_to_fetchQ(proc_inst_t inst)
{
  fetch_disp_struct temp;

  temp.inst_no        = inst_no;

  if(-1 == inst.op_code)
  {
    temp.FU_type      = 1;
  }
  else
  {
    temp.FU_type      = inst.op_code;
  }

  temp.dest_reg_no    = inst.dest_reg;
  temp.src_reg_no[0]  = inst.src_reg[0];
  temp.src_reg_no[1]  = inst.src_reg[1];

  fetchQ.push_back(temp);
}

void dispQ_to_schedQ()
{
  sched_struct temp;

  if(-1 != dispQ.front().src_reg_no[0])
  {
    if(false == reg_file[dispQ.front().src_reg_no[0]].ready)
    {
      temp.src_reg_ready[0] = false;
      temp.src_reg_tag[0]   = reg_file[dispQ.front().src_reg_no[0]].reg_tag;
    }
    else
    {
      temp.src_reg_ready[0] = true;
      temp.src_reg_tag[0]   = -1;
    }
  }
  else
  {
    temp.src_reg_ready[0] = true;
    temp.src_reg_tag[0]   = -1;
  }

  if(-1 != dispQ.front().src_reg_no[1])
  {
    if(false == reg_file[dispQ.front().src_reg_no[1]].ready)
    {
      temp.src_reg_ready[1] = false;
      temp.src_reg_tag[1]   = reg_file[dispQ.front().src_reg_no[1]].reg_tag;
    }
    else
    {
      temp.src_reg_ready[1] = true;
      temp.src_reg_tag[1]   = -1;
    }
  }
  else
  {
    temp.src_reg_ready[1] = true;
    temp.src_reg_tag[1]   = -1;
  }


  temp.FU_type          = dispQ.front().FU_type;
  temp.dest_reg_no      = dispQ.front().dest_reg_no;

  temp.dest_reg_tag     = dispQ.front().inst_no;

  temp.inst_no          = dispQ.front().inst_no;

  temp.fired            = false;
  temp.complete         = false;
  temp.recently_updated = false;

  schedQ.push_back(temp);


	if(-1 != dispQ.front().dest_reg_no)
	{
    reg_file[dispQ.front().dest_reg_no].ready    = false;
    reg_file[dispQ.front().dest_reg_no].reg_tag = dispQ.front().inst_no;
	}
}

bool Is_FU_free(uint16_t FU_type, uint16_t& FU_free_index)
{
  for(uint16_t i=0; i<FU_size[FU_type]; i++)
  {
    if(false ==  scoreboard[FU_type][i].busy)
    {
      FU_free_index = i;
      return true;
    }
  }

  return false;
}

bool is_any_FU_busy(uint16_t& FU_type, uint16_t& FU_index)
{
  for(uint16_t i=0; i<FU_nos; i++)
  {
    for(uint16_t j=0; j<FU_size[i]; j++)
    {
      if(true == scoreboard[i][j].busy)
      {
        FU_type  = i;
        FU_index = j;
        return true;
      }
    }
  }

  return false;
}

void get_least_FU_busy(uint16_t& FU_type, uint16_t &FU_index)
{
  uint16_t i = FU_type;
  uint16_t j = FU_index;

  while(i<FU_nos)
  {
    while(j<FU_size[i])
    {
      if(true == scoreboard[i][j].busy)
      {
        if(scoreboard[i][j].added_cycle < scoreboard[FU_type][FU_index].added_cycle)
        {
          FU_type  = i;
          FU_index = j;
        }
        else if(scoreboard[i][j].added_cycle == scoreboard[FU_type][FU_index].added_cycle)
        {
          if(scoreboard[i][j].reg_tag < scoreboard[FU_type][FU_index].reg_tag)
          {
            FU_type  = i;
            FU_index = j;
          }
        }
      }

      j++;
    }

    j = 0;
    i++;
  }
}

bool is_any_result_bus_free(uint16_t& free_result_bus_index)
{
  for(uint16_t i=0; i<result_bus_size; i++)
  {
    if(false == result_bus[i].busy)
    {
      free_result_bus_index = i;
      return true;
    }
  }

  return false;
}

void result_bus_to_reg_file()
{
  for(uint16_t i=0; i<result_bus_size; i++)
  {
    if(true == result_bus[i].busy)
    {
      if(-1 != result_bus[i].reg_no)
      {
        if(result_bus[i].reg_tag == reg_file[result_bus[i].reg_no].reg_tag)
        {
          reg_file[result_bus[i].reg_no].ready   = true;
          reg_file[result_bus[i].reg_no].reg_tag = -1;
        }
      }
    }
  }
}


void result_bus_to_schedQ(proc_stats_t* p_stats)
{
  for(uint16_t i=0; i<result_bus_size; i++)
  {
    if(true == result_bus[i].busy)
    {
      for(uint16_t j=0; j<schedQ.size(); j++)
      {
        if(result_bus[i].reg_tag == schedQ[j].src_reg_tag[0])
        {
          schedQ[j].src_reg_ready[0] = true;
          schedQ[j].src_reg_tag[0]   = -1;
          schedQ[j].recently_updated = true;
        }
        if(result_bus[i].reg_tag == schedQ[j].src_reg_tag[1])
        {
          schedQ[j].src_reg_ready[1] = true;
          schedQ[j].src_reg_tag[1]   = -1;
          schedQ[j].recently_updated = true;
        }
        if(result_bus[i].reg_tag == schedQ[j].dest_reg_tag)
        {
          schedQ[j].complete = true;
          schedQ[j].recently_updated = true;

          inst_cycles[schedQ[j].inst_no -1].su_cycle = current_cycle;

          p_stats->retired_instruction++;
        }
      }
    }
  }
}


/**
 * Subroutine for initializing the processor. You many add and initialize any global or heap
 * variables as needed.
 * XXX: You're responsible for completing this routine
 *
 * @r ROB size
 * @k0 Number of k0 FUs
 * @k1 Number of k1 FUs
 * @k2 Number of k2 FUs
 * @f Number of instructions to fetch
 */
void setup_proc(uint64_t r, uint64_t k0, uint64_t k1, uint64_t k2, uint64_t f)
{
  init_global_var(r, k0, k1, k2, f);

  //No need to allocate and intialize fetchQ or schedQ

  //Reserve memory for dispatch queue and instructions
  dispQ.reserve(56000);
  inst_cycles.reserve(100000);

  alloc_init_scoreboard();

  alloc_init_result_bus();

  alloc_init_reg_file();
}

/**
 * Subroutine that simulates the processor.
 *   The processor should fetch instructions as appropriate, until all instructions have executed
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 */
void run_proc(proc_stats_t* p_stats)
{
  p_stats->avg_inst_retired = 0;
	p_stats->avg_inst_fired = 0;
	p_stats->avg_disp_size = 0;
	p_stats->max_disp_size = 0;
	p_stats->retired_instruction = 0;
	p_stats->cycle_count = 0;

  bool inst_file_read_complete = false;

  cout << "INST	FETCH	DISP	SCHED	EXEC	STATE"<< endl;

  do
  {
    current_cycle++;

    //Copy from FU to result bus
    uint16_t free_result_bus_index;
    uint16_t FU_type  = 0;
    uint16_t FU_index = 0;

    while( (true == is_any_result_bus_free(free_result_bus_index)) && (true == is_any_FU_busy(FU_type, FU_index)) )
    {
      get_least_FU_busy(FU_type, FU_index);

      result_bus[free_result_bus_index].busy    = true;
      result_bus[free_result_bus_index].reg_no  = scoreboard[FU_type][FU_index].reg_no;
      result_bus[free_result_bus_index].reg_tag = scoreboard[FU_type][FU_index].reg_tag;

      scoreboard[FU_type][FU_index].busy        = false;
      scoreboard[FU_type][FU_index].reg_no      = -1;
      scoreboard[FU_type][FU_index].reg_tag     = -1;
      scoreboard[FU_type][FU_index].added_cycle = 0;
    }

    //Copy from result bus to Reg file and SchedQ
    result_bus_to_reg_file();
    result_bus_to_schedQ(p_stats);

    //Fire from SchedQ to FU
    for(uint16_t i=0; i<schedQ.size(); i++)
    {
      uint16_t FU_free_index;

      if( (true == schedQ[i].src_reg_ready[0]) && (true == schedQ[i].src_reg_ready[1]) && (true != schedQ[i].recently_updated) &&
          (true != schedQ[i].fired) && (true == Is_FU_free(schedQ[i].FU_type, FU_free_index)) )
      {
        scoreboard[schedQ[i].FU_type][FU_free_index].busy        = true;
        scoreboard[schedQ[i].FU_type][FU_free_index].reg_no      = schedQ[i].dest_reg_no;
        scoreboard[schedQ[i].FU_type][FU_free_index].reg_tag     = schedQ[i].dest_reg_tag;
        scoreboard[schedQ[i].FU_type][FU_free_index].added_cycle = current_cycle;

        schedQ[i].fired = true;

        inst_cycles[schedQ[i].inst_no -1].ex_cycle = current_cycle;
      }

    }

    //Copy from dispQ to schedQ while SchedQ is not full and DispQ is not empty
    while( (schedQ.size() < schedQ_max_size) && (0 != dispQ.size()) )
    {
      dispQ_to_schedQ();
      dispQ.erase(dispQ.begin());

      inst_cycles[schedQ.back().inst_no -1].sched_cycle = current_cycle;
    }

    //Copy from fetchQ to DispQ
    while(0 != fetchQ.size())
    {
      dispQ.push_back(fetchQ.front());
      fetchQ.erase(fetchQ.begin());

      inst_cycles[dispQ.back().inst_no -1].disp_cycle = current_cycle;
    }

    total_dispQ_size += dispQ.size();

    if(p_stats->max_disp_size < dispQ.size())
    {
      p_stats->max_disp_size = dispQ.size();
    }

    //Read upto fetchQ_size no. of instructions and put into FetchQ
    while( (fetchQ.size() < fetchQ_max_size) && (true != inst_file_read_complete) )
    {
      proc_inst_t inst;

      if(true == read_instruction(&inst))
      {
        inst_no++;

        inst_to_fetchQ(inst);

        add_inst_cycles();
        inst_cycles.back().fetch_cycle = current_cycle;
      }
      else // File completely read
      {
        inst_file_read_complete = true;
        break;
      }
    }

    for(vector<sched_struct>::iterator it=schedQ.begin(); it!=schedQ.end(); it++)
    {
      if( (true != it->recently_updated) && (true == it->complete) )
      {
        it = schedQ.erase(it);
        it--;
      }
    }

    for(uint16_t i=0; i<schedQ.size(); i++)
    {
      schedQ[i].recently_updated = false;
    }

    for(uint16_t i=0; i<result_bus_size; i++)
    {
      result_bus[i].busy        = false;
      result_bus[i].reg_no      = -1;
      result_bus[i].reg_tag     = -1;
    }

  }while( (true != inst_file_read_complete) || (0!=schedQ.size()) || (0!=dispQ.size()) || (fetchQ.size()));


  p_stats->cycle_count = --current_cycle;


  for(uint32_t i=0; i<inst_cycles.size(); i++)
  {
    cout << i+1 << "\t" << inst_cycles[i].fetch_cycle << "\t" << inst_cycles[i].disp_cycle << "\t" << inst_cycles[i].sched_cycle <<
            "\t" << inst_cycles[i].ex_cycle << "\t" << inst_cycles[i].su_cycle << endl ;
  }
  cout << endl;


//  cout << (double)p_stats->retired_instruction / (double)p_stats->cycle_count << endl;
}

/**
 * Subroutine for cleaning up any outstanding instructions and calculating overall statistics
 * such as average IPC, average fire rate etc.
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 */
void complete_proc(proc_stats_t *p_stats)
{
  for(uint16_t i=0; i<FU_nos; i++)
  {
    free(scoreboard[i]);
  }

  free(scoreboard);

  free(result_bus);
  free(reg_file);


  p_stats->avg_inst_fired = (double)p_stats->retired_instruction / (double)p_stats->cycle_count;

	p_stats->avg_inst_retired = (double)p_stats->retired_instruction / (double)p_stats->cycle_count;

	p_stats->avg_disp_size = (double)total_dispQ_size / (double)p_stats->cycle_count;
}
