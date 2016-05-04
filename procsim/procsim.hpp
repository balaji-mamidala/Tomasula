#ifndef PROCSIM_HPP
#define PROCSIM_HPP

#include <cstdint>
#include <cstdio>

#define DEFAULT_K0 1
#define DEFAULT_K1 2
#define DEFAULT_K2 3
#define DEFAULT_R 8
#define DEFAULT_F 4

typedef struct _proc_inst_t
{
    uint32_t instruction_address;
    int32_t op_code;
    int32_t src_reg[2];
    int32_t dest_reg;

    // You may introduce other fields as needed

} proc_inst_t;

typedef struct _proc_stats_t
{
    float avg_inst_retired;
    float avg_inst_fired;
    float avg_disp_size;
    unsigned long max_disp_size;
    unsigned long retired_instruction;
    unsigned long cycle_count;
} proc_stats_t;

// Custom data structures start
struct fetch_disp_struct
{
  uint32_t inst_no;
  int32_t  FU_type;
  int32_t  dest_reg_no;
  int32_t  src_reg_no[2];
};

struct sched_struct
{
  uint32_t inst_no;
  int32_t FU_type;
  int32_t dest_reg_no;
  int32_t dest_reg_tag;
  int32_t src_reg_tag[2];
  int32_t src_reg_ready[2];

  bool fired;
  bool complete;
  bool recently_updated;
};

struct result_bus_struct
{
  bool busy;
  int32_t reg_no;
  int32_t reg_tag;
};

struct scoreboard_struct
{
  bool busy;
  int32_t reg_no;
  int32_t reg_tag;
  uint32_t added_cycle;
};

struct reg_file_struct
{
  bool ready;
  int32_t reg_tag;
};

struct inst_cycles_struct
{
  uint32_t fetch_cycle;
  uint32_t disp_cycle;
  uint32_t sched_cycle;
  uint32_t ex_cycle;
  uint32_t su_cycle;
};

// Custom data structures end


bool read_instruction(proc_inst_t* p_inst);

void setup_proc(uint64_t r, uint64_t k0, uint64_t k1, uint64_t k2, uint64_t f);
void run_proc(proc_stats_t* p_stats);
void complete_proc(proc_stats_t* p_stats);

#endif /* PROCSIM_HPP */
