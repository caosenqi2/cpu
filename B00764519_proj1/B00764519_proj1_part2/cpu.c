/*
 *  cpu.c
 *  Contains APEX cpu pipeline implementation
 *
 *  Author :
 *  Senqi Cao (scao16@binghamton.edu)
 *  State University of New York, Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"

/* Set this flag to 1 to enable debug messages */
int ENABLE_DEBUG_MESSAGES = 1;

static int halted=0;
static int halting=0;
int zero;
int a = 0;
int forwarded=0;


/*
 * This function creates and initializes APEX cpu.
 *
 * Note : You are free to edit this function according to your
 * 				implementation
 */
APEX_CPU*
APEX_cpu_init(const char* filename)
{
    if (!filename) {
        return NULL;
    }

    APEX_CPU* cpu = malloc(sizeof(*cpu));

    if (!cpu) {
        return NULL;
    }

    /* Initialize PC, Registers and all pipeline stages */
    cpu->pc = 4000;
    memset(cpu->regs, 0, sizeof(int) * 16);
    memset(cpu->regs_valid, 1, sizeof(int) * 16);
    memset(cpu->stage, 0, sizeof(CPU_Stage) * NUM_STAGES);
    memset(cpu->data_memory, 0, sizeof(int) * 4000);

    /* Parse input file and create code memory */
    cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);

    if (!cpu->code_memory) {
        free(cpu);
        return NULL;
    }

    if (ENABLE_DEBUG_MESSAGES) {
        fprintf(stderr,
                "APEX_CPU : Initialized APEX CPU, loaded %d instructions\n",
                cpu->code_memory_size);
        fprintf(stderr, "APEX_CPU : Printing Code Memory\n");
        printf("%-9s %-9s %-9s %-9s %-9s\n", "opcode", "rd", "rs1", "rs2", "imm");

        for (int i = 0; i < cpu->code_memory_size; ++i) {
            printf("%-9s %-9d %-9d %-9d %-9d\n",
                cpu->code_memory[i].opcode,
                cpu->code_memory[i].rd,
                cpu->code_memory[i].rs1,
                cpu->code_memory[i].rs2,
                cpu->code_memory[i].imm);
        }
    }


    /* Make all stages busy except Fetch stage, initally to start the pipeline */
      for (int i = 1; i < NUM_STAGES; ++i) {
        cpu->stage[i].busy = 0;
      }

    return cpu;
}

/*
 * This function de-allocates APEX cpu.
 *
 * Note : You are free to edit this function according to your
 * 				implementation
 */
void
APEX_cpu_stop(APEX_CPU* cpu)
{
    free(cpu->code_memory);
    free(cpu);
}

/* Converts the PC(4000 series) into
 * array index for code memory
 *
 * Note : You are not supposed to edit this function
 *
 */
int
get_code_index(int pc)
{
    return (pc - 4000) / 4;
}

static void
print_instruction(CPU_Stage* stage)
{

    if (strcmp(stage->opcode, "STORE") == 0) {
        printf("%s,R%d,R%d,#%d ", stage->opcode, stage->rs1, stage->rs2, stage->imm);
    }

    if (strcmp(stage->opcode, "STR") == 0) {
        printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rs1, stage->rs2, stage->rs3);
    }

    if (strcmp(stage->opcode, "MOVC") == 0) {
        printf("%s,R%d,#%d ", stage->opcode, stage->rd, stage->imm);
    }

    if (strcmp(stage->opcode, "LOAD") == 0) {
        printf("%s,R%d,R%d,#%d ", stage->opcode, stage->rd,stage->rs1, stage->imm);
    }

    if (strcmp(stage->opcode, "LDR") == 0) {
        printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rd,stage->rs1, stage->rs2);
    }

    if (strcmp(stage->opcode, "ADD") == 0) {
        printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1,stage->rs2);
    }

    if (strcmp(stage->opcode, "MUL") == 0) {
        printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1,stage->rs2);
    }

    if (strcmp(stage->opcode, "SUB") == 0) {
        printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1,stage->rs2);
    }


    if (strcmp(stage->opcode, "AND") == 0) {
        printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1,stage->rs2);
    }

    if (strcmp(stage->opcode, "OR") == 0) {
        printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1,stage->rs2);
    }

    if (strcmp(stage->opcode, "EX-OR") == 0) {
        printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1,stage->rs2);
    }


    if (strcmp(stage->opcode, "BZ") == 0) {
        printf("%s,#%d ", stage->opcode, stage->imm);
    }

    if (strcmp(stage->opcode, "BNZ") == 0) {
        printf("%s,#%d ", stage->opcode, stage->imm);
    }

    if (strcmp(stage->opcode, "JUMP") == 0) {
        printf("%s,R%d,#%d ", stage->opcode, stage->rs1, stage->imm);
    }

    if (strcmp(stage->opcode, "HALT") == 0) {
        printf("%s", stage->opcode);
    }
}

/* Debug function which dumps the cpu stage
 * content
 *
 * Note : You are not supposed to edit this function
 *
 */
static void
print_stage_content(char* name, CPU_Stage* stage)
{
    printf("Instruction at %-10s stage -->      (I%d:%d) ", name, get_code_index(stage->pc), stage->pc);
    print_instruction(stage);
    printf("\n");
}

static void
print_empty(char* name)
{
    printf("Instruction at %-10s stage -->      ", name);
    printf("EMPTY");
    printf("\n");
}

/*
 *  Fetch Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
fetch(APEX_CPU* cpu)
{
    CPU_Stage* stage = &cpu->stage[F];
    if ( !stage->stalled && !halted) {
    /* Store current PC in fetch latch */

        stage->pc = cpu->pc;

        /* Index into code memory using this pc and copy all instruction fields into
        * fetch latch
        */
        APEX_Instruction* current_ins = &cpu->code_memory[get_code_index(cpu->pc)];
        strcpy(stage->opcode, current_ins->opcode);
        stage->rd = current_ins->rd;
        stage->rs1 = current_ins->rs1;
        stage->rs2 = current_ins->rs2;
        stage->imm = current_ins->imm;
        stage->rd = current_ins->rd;

        if(stage->opcode[0])
        stage->busy = 0;

        /* Update PC for next instruction */
        cpu->pc += 4;

        if (stage->busy)
            {if (ENABLE_DEBUG_MESSAGES) print_empty("Fetch");}
        else
        {
            if (ENABLE_DEBUG_MESSAGES) print_stage_content("Fetch", stage);

            if (!cpu->stage[DRF].stalled){
            /* Copy data from fetch latch to decode latch*/
                if(!stage->flushed) cpu->stage[DRF] = cpu->stage[F];
                else stage->flushed = 0;
                stage->busy = 1;

            }
            else{
                stage->stalled = 1;
            }
        if (zero) {
                cpu->pc = cpu->stage[EX].buffer;
                zero = 0;
            }
        if (halting) {stage->busy = 1; stage->stalled = 1;}
        }
    }

    else
    {
        if (stage->busy)
            {if (ENABLE_DEBUG_MESSAGES) print_empty("Fetch");}

        else {
            if (ENABLE_DEBUG_MESSAGES) print_stage_content("Fetch", stage);
            if(!cpu->stage[DRF].stalled){
            /* Copy data from fetch latch to decode latch*/
            stage->stalled = 0;
            cpu->stage[DRF] = cpu->stage[F];
            stage->busy = 1;
            }
        }

    }

    return 0;
}

/*
 *  Decode Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
decode(APEX_CPU* cpu)
{
    CPU_Stage* stage = &cpu->stage[DRF];

    if (stage->busy)
       {if (ENABLE_DEBUG_MESSAGES) print_empty("Decode/RF");}

    else{
        if (ENABLE_DEBUG_MESSAGES)
                    print_stage_content("Decode/RF", stage);

    if ( !stage->stalled) {
        //if (cpu->stage[EX].busy) {stage->stalled = 1; b=1;}
        }

    else {
        if(stage->stalled){
            if (cpu->stage[EX1].stalled) a = 1;
            else {a =0;}
        }
    }

    /* Read data from register file for store */
    if (strcmp(stage->opcode, "STR") == 0) {
        stage->rs1_valid = 0;
        stage->rs2_valid = 0;
        stage->rs3_valid = 0;
        if(cpu->regs_valid[stage->rs1])
        {
            stage->rs1_value = cpu->regs[stage->rs1];
            stage->rs1_valid = 1;
        }
        if( cpu->regs_valid[stage->rs2]){
            stage->rs2_value = cpu->regs[stage->rs2];
            stage->rs2_valid = 1;
        }
        if( cpu->regs_valid[stage->rs3]){
            stage->rs3_value = cpu->regs[stage->rs3];
            stage->rs3_valid = 1;
        }

        if(!cpu->regs_valid[stage->rs1])
        { printf("1 not valid\n");
            if (cpu->stage[EX].rd == stage->rs1){
                stage->rs1_value = cpu->stage[EX].buffer;
                stage->rs1_valid = 1;printf("yes21\n");
            }
            else if (cpu->stage[MEM].rd == stage->rs1){
                stage->rs1_value = cpu->stage[MEM].buffer;
                stage->rs1_valid = 1;printf("no21\n");
            }
        }

        if(!cpu->regs_valid[stage->rs2])
        { printf("2 not valid\n");
            if (cpu->stage[EX+1].rd == stage->rs2){
                stage->rs2_value = cpu->stage[EX+1].buffer;
                stage->rs2_valid = 1;printf("yes22\n");
            }
            else if (cpu->stage[MEM+1].rd == stage->rs2){
                stage->rs2_value = cpu->stage[MEM+1].buffer;
                stage->rs2_valid = 1;printf("no22\n");
            }
        }

        if(!cpu->regs_valid[stage->rs3])
        { printf("3 not valid\n");
            if (cpu->stage[EX+1].rd == stage->rs3){
                stage->rs3_value = cpu->stage[EX+1].buffer;
                stage->rs3_valid = 1;printf("yes3\n");
            }
            else if (cpu->stage[MEM+1].rd == stage->rs3){
                stage->rs3_value = cpu->stage[MEM+1].buffer;
                stage->rs3_valid = 1;printf("no3\n");
            }
        }

        if(stage->rs1_valid && stage->rs2_valid && stage->rs3_valid){
        if (stage->stalled && a==0) stage->stalled = 0;
            if (!stage->stalled){
                if(!stage->flushed) {
                    cpu->stage[EX1] = cpu->stage[DRF];
                }
                else stage->flushed = 0;
                stage->busy = 1;
            }
        }
        else stage->stalled = 1;
    }

    /* No Register file read needed for MOVC */
    if (strcmp(stage->opcode, "MOVC") == 0) {
        if (stage->stalled && a==0) stage->stalled = 0;

        if (!stage->stalled){
                if(!stage->flushed) {
                    cpu->regs_valid[stage->rd] = 0;
                    cpu->stage[EX1] = cpu->stage[DRF];
                }
                else stage->flushed = 0;
                stage->busy = 1;
            }

    }


    if (strcmp(stage->opcode, "LOAD") == 0) {
        stage->rs1_valid = 0;
        if(cpu->regs_valid[stage->rs1])
        {
            stage->rs1_value = cpu->regs[stage->rs1];
            stage->rs1_valid = 1;
        }
        if(!cpu->regs_valid[stage->rs1])
        {
            if (cpu->stage[EX+1].rd == stage->rs1){
                stage->rs1_value = cpu->stage[EX].buffer;
                stage->rs1_valid = 1;
            }
            else if (cpu->stage[MEM+1].rd == stage->rs1){
                stage->rs1_value = cpu->stage[MEM].buffer;
                stage->rs1_valid = 1;
            }
        }
        if(stage->rs1_valid){
        if (stage->stalled && a==0) stage->stalled = 0;
            if (!stage->stalled){
                if(!stage->flushed) {
                    cpu->regs_valid[stage->rd] = 0;
                    cpu->stage[EX1] = cpu->stage[DRF];
                }
                else stage->flushed = 0;
                stage->busy = 1;
            }
        }
        else stage->stalled = 1;
    }

    if (strcmp(stage->opcode, "ADD") == 0 ||
        strcmp(stage->opcode, "MUL") == 0 ||
        strcmp(stage->opcode, "AND") == 0 ||
        strcmp(stage->opcode, "OR") == 0 ||
        strcmp(stage->opcode, "EX-OR") == 0 ||
        strcmp(stage->opcode, "LDR") == 0 ||
        strcmp(stage->opcode, "STORE") == 0 ||
        strcmp(stage->opcode, "SUB") == 0)
        {
        stage->rs1_valid = 0;
        stage->rs2_valid = 0;
        if(cpu->regs_valid[stage->rs1])
        {
            stage->rs1_value = cpu->regs[stage->rs1];
            stage->rs1_valid = 1;
        }
        if( cpu->regs_valid[stage->rs2]){
            stage->rs2_value = cpu->regs[stage->rs2];
            stage->rs2_valid = 1;
        }

        if(!cpu->regs_valid[stage->rs1])
        { printf("1 not valid\n");
            if (cpu->stage[EX].rd == stage->rs1){
                stage->rs1_value = cpu->stage[EX].buffer;
                stage->rs1_valid = 1;printf("yes21\n");
            }
            else if (cpu->stage[MEM].rd == stage->rs1){
                stage->rs1_value = cpu->stage[MEM].buffer;
                stage->rs1_valid = 1;printf("no21\n");
            }
        }

        if(!cpu->regs_valid[stage->rs2])
        { printf("2 not valid\n");
            if (cpu->stage[EX+1].rd == stage->rs2){
                stage->rs2_value = cpu->stage[EX+1].buffer;
                stage->rs2_valid = 1;printf("yes22\n");
            }
            else if (cpu->stage[MEM+1].rd == stage->rs2){
                stage->rs2_value = cpu->stage[MEM+1].buffer;
                stage->rs2_valid = 1;printf("no22\n");
            }
        }

        if(stage->rs1_valid && stage->rs2_valid){
        if (stage->stalled && a==0) stage->stalled = 0;
            if (!stage->stalled){
                if(!stage->flushed) {
                    if(strcmp(stage->opcode, "STORE") != 0) cpu->regs_valid[stage->rd] = 0;
                    cpu->stage[EX1] = cpu->stage[DRF];
                }
                else stage->flushed = 0;
                stage->busy = 1;
            }
        }

        else stage->stalled = 1;
    }


    if (strcmp(stage->opcode, "BZ") ==0 || strcmp(stage->opcode, "BNZ") ==0) {
        if (cpu->stage[EX+1].rd == cpu->code_memory[get_code_index(stage->pc - 4)].rd) forwarded =1;
        if(cpu->regs_valid[cpu->code_memory[get_code_index(stage->pc - 4)].rd] || forwarded==1){
        printf("branch taken\n");
        if (cpu->stage[EX].rd == cpu->code_memory[get_code_index(stage->pc - 4)].rd) stage->buffer = cpu->stage[EX+1].buffer;

        printf("ex buffer: %d\n",cpu->stage[EX].buffer);
            if (stage->stalled && a==0) stage->stalled = 0;
            if (!stage->stalled){
                if(!stage->flushed) {
                    cpu->stage[EX1] = cpu->stage[DRF];
                }
                else stage->flushed = 0;
                stage->busy = 1;
            }
        }

        else stage->stalled = 1;
    }

    if (strcmp(stage->opcode, "JUMP") ==0) {
        stage->rs1_valid = 0;
        if(cpu->regs_valid[stage->rs1])
        {
            stage->rs1_value = cpu->regs[stage->rs1];
            stage->rs1_valid = 1;
        }
        if(!cpu->regs_valid[stage->rs1])
        {
            if (cpu->stage[EX+1].rd == stage->rs1){
                stage->rs1_value = cpu->stage[EX].buffer;
                stage->rs1_valid = 1;
            }
            else if (cpu->stage[MEM+1].rd == stage->rs1){
                stage->rs1_value = cpu->stage[MEM].buffer;
                stage->rs1_valid = 1;
            }
        }
        if(stage->rs1_valid){
        if (stage->stalled && a==0) stage->stalled = 0;
            if (!stage->stalled){
                if(!stage->flushed) {
                    cpu->regs_valid[stage->rd] = 0;
                    cpu->stage[EX1] = cpu->stage[DRF];
                }
                else stage->flushed = 0;
                stage->busy = 1;
            }
        }
        else stage->stalled = 1;
    }

    if (strcmp(stage->opcode, "HALT") ==0) {
        cpu->stage[F].flushed =  1;
        halting=1;
        if (stage->stalled && a==0) stage->stalled = 0;
        if (!stage->stalled){
                if(!stage->flushed) {cpu->stage[EX1] = cpu->stage[DRF];}
                else stage->flushed = 0;
                stage->busy = 1;
            }
    }

}

//GZ
  return 0;
}

/*
 *  Execute Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */

int
execute1(APEX_CPU* cpu)
{
    CPU_Stage* stage = &cpu->stage[EX1];

    if (stage->flushed){
            stage->flushed = 0;
            //stage->stalled = 1;

            if (stage->busy)
           { if (ENABLE_DEBUG_MESSAGES) print_empty("Execute1");}
           else{
                print_stage_content("Execute1", stage);
           }

           stage->busy = 1;



        }
    else{

    if (!stage->busy && !stage->stalled && !stage->busy) {
    if (ENABLE_DEBUG_MESSAGES)
            print_stage_content("Execute1", stage);

    if (!stage->busy){
        /* Copy data from Execute latch to Memory latch*/
            cpu->stage[EX] = cpu->stage[EX1];
            stage->busy = 1;
        }
    }

    else {

        if (stage->busy)
           { if (ENABLE_DEBUG_MESSAGES) print_empty("Execute1");}

    }

    }
    stage->busy = 1;

    return 0;
}



int
execute2(APEX_CPU* cpu)
{
    CPU_Stage* stage = &cpu->stage[EX];
    if (stage->busy)
           { if (ENABLE_DEBUG_MESSAGES) print_empty("Execute2");}

    else{

    if (ENABLE_DEBUG_MESSAGES) print_stage_content("Execute2", stage);

    if (!stage->stalled && !stage->busy) {

        /* Store */
        if (strcmp(stage->opcode, "STORE") == 0) {
            stage->mem_address = stage->rs2_value + stage->imm;
        }

        if (strcmp(stage->opcode, "STR") == 0) {
            stage->mem_address = stage->rs2_value + stage->rs3_value;
        }

        if (strcmp(stage->opcode, "LOAD") == 0) {
            stage->mem_address = stage->rs1_value + stage->imm;
        }

        if (strcmp(stage->opcode, "LDR") == 0) {
            stage->mem_address = stage->rs1_value + stage->rs2_value;
        }

        /* MOVC */
        if (strcmp(stage->opcode, "MOVC") == 0) {
            stage->buffer = stage->imm + 0;
        }

        if (strcmp(stage->opcode, "ADD") == 0) {
            stage->buffer = stage->rs1_value + stage->rs2_value;
        }

        if (strcmp(stage->opcode, "SUB") == 0) {
            stage->buffer = stage->rs1_value - stage->rs2_value;
            printf("sub buffer: %d\n",stage->buffer);
        }

        if (strcmp(stage->opcode, "MUL") ==0) {
            stage->buffer = stage->rs1_value * stage->rs2_value;
        }

        if (strcmp(stage->opcode, "AND") ==0) {
            stage->buffer = stage->rs1_value & stage->rs2_value;
        }

        if (strcmp(stage->opcode, "OR") ==0) {
            stage->buffer = stage->rs1_value | stage->rs2_value;
        }

        if (strcmp(stage->opcode, "EX-OR") ==0) {
            stage->buffer = stage->rs1_value ^ stage->rs2_value;
        }


        if (strcmp(stage->opcode, "BZ") ==0) {
                if((cpu->regs_valid[cpu->code_memory[get_code_index(stage->pc - 4)].rd] && !cpu->regs[cpu->code_memory[get_code_index(stage->pc - 4)].rd]) ||
                (forwarded==1 && stage->buffer == 0))
                {
                printf("wb is %d\n",cpu->regs[cpu->code_memory[get_code_index(stage->pc - 4)].rd] );
                printf("BZ executed\n");printf("buffer is %d\n",stage->buffer);
                forwarded=0;
                zero = 1;
                stage->buffer = stage->pc + stage->imm;
                cpu->stage[F].flushed = 1;
                cpu->stage[DRF].flushed = 1;
                cpu->stage[EX1].flushed = 1;
                }
        }

        if (strcmp(stage->opcode, "BNZ") ==0) {
                if((cpu->regs_valid[cpu->code_memory[get_code_index(stage->pc - 4)].rd] && cpu->regs[cpu->code_memory[get_code_index(stage->pc - 4)].rd]) ||
                (forwarded==1 && stage->buffer != 0))
                {
                forwarded=0;
                zero = 1;
                stage->buffer = stage->pc + stage->imm;
                cpu->stage[F].flushed = 1;
                cpu->stage[DRF].flushed = 1;
                cpu->stage[EX1].flushed = 1;
                }
        }

        if (strcmp(stage->opcode, "JUMP") ==0) {
                zero = 1;
                stage->buffer = stage->rs1_value + stage->imm;
                cpu->stage[F].flushed = 1;
                cpu->stage[DRF].flushed = 1;
                cpu->stage[EX1].flushed = 1;
        }

        if (strcmp(stage->opcode, "HALT") ==0) {
        }

        /* Copy data from Execute latch to Memory latch*/
        cpu->stage[MEM1] = cpu->stage[EX];
        stage->busy = 1;

    }
}
    return 0;
}

/*
 *  Memory Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
memory1(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[MEM1];

  if (stage->flushed){

  stage->flushed=0;

  if (stage->busy)
           { if (ENABLE_DEBUG_MESSAGES) print_empty("Memory1");}
           else{
                print_stage_content("Memory1", stage);
           }

  }
  else{

  if (!stage->busy && !stage->stalled && !stage->busy) {

    if (ENABLE_DEBUG_MESSAGES)
        print_stage_content("Memory", stage);

    /* Copy data from decode latch to execute latch*/
    cpu->stage[MEM] = cpu->stage[MEM1];
    stage->busy = 1;


  }


  else if (stage->busy){
      if (ENABLE_DEBUG_MESSAGES)
      print_empty("Memory");
  }
}
  stage->busy = 1;
  return 0;
}

int
memory2(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[MEM];
  if (!stage->stalled && !stage->busy) {

    /* Store */
    if (strcmp(stage->opcode, "STORE") == 0) {

        cpu->data_memory[stage->mem_address] = stage->rs1_value;
    }

    if (strcmp(stage->opcode, "STR") == 0) {

        cpu->data_memory[stage->mem_address] = stage->rs1_value;
    }

    if (strcmp(stage->opcode, "LOAD") == 0 || strcmp(stage->opcode, "LDR") == 0) {

       stage->buffer =  cpu->data_memory[stage->mem_address] ;
    }

    if (ENABLE_DEBUG_MESSAGES)
        print_stage_content("Memory2", stage);

    /* Copy data from decode latch to execute latch*/
    cpu->stage[WB] = cpu->stage[MEM];
    stage->busy = 1;
  }


  else if (stage->busy){
      if (ENABLE_DEBUG_MESSAGES)
      print_empty("Memory2");
  }

  return 0;
}

/*
 *  Writeback Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
writeback(APEX_CPU* cpu)
{
    CPU_Stage* stage = &cpu->stage[WB];

    if ( !stage->stalled && !stage->busy) {
    /* Update register file */

    if (strcmp(stage->opcode, "MOVC") == 0) {
      cpu->regs[stage->rd] = stage->buffer;
      cpu->regs_valid[stage->rd] = 1;
    }

    if (strcmp(stage->opcode, "LOAD") == 0 || strcmp(stage->opcode, "LDR") == 0) {
      cpu->regs[stage->rd] = stage->buffer;
      cpu->regs_valid[stage->rd] = 1;
    }

    if (strcmp(stage->opcode, "ADD") == 0) {
      cpu->regs[stage->rd] = stage->buffer;
      cpu->regs_valid[stage->rd] = 1;
    }

    if (strcmp(stage->opcode, "SUB") == 0) {
      cpu->regs[stage->rd] = stage->buffer;
      cpu->regs_valid[stage->rd] = 1;
    }

    if (strcmp(stage->opcode, "MUL") == 0) {
      cpu->regs[stage->rd] = stage->buffer;
      cpu->regs_valid[stage->rd] = 1;
    }

    if (strcmp(stage->opcode, "AND") == 0) {
      cpu->regs[stage->rd] = stage->buffer;
      cpu->regs_valid[stage->rd] = 1;
    }

     if (strcmp(stage->opcode, "OR") == 0) {
      cpu->regs[stage->rd] = stage->buffer;
      cpu->regs_valid[stage->rd] = 1;
    }

     if (strcmp(stage->opcode, "EX-OR") == 0) {
      cpu->regs[stage->rd] = stage->buffer;
      cpu->regs_valid[stage->rd] = 1;
    }

    if (strcmp(stage->opcode, "HALT") == 0) {
        halted = 1;
    }

    if (ENABLE_DEBUG_MESSAGES)
      print_stage_content("Writeback", stage);

    cpu->ins_completed++;
    stage->busy = 1;

    }

  else if (stage->busy) {
    if (ENABLE_DEBUG_MESSAGES)
    print_empty("Writeback");

    }

  return 0;
}

/*
 *  APEX CPU simulation loop
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
APEX_cpu_run(APEX_CPU* cpu, int n)
{
    while (1) {

        /* All the instructions committed, so exit */
        //if (cpu->ins_completed == cpu->code_memory_size) {
        //  printf("(apex) >> Simulation Complete");
        //   break;
        // }

        if (cpu->clock==(n-1)) {
          printf("(apex) >> Simulation Complete");
           break;
         }


        if (halted) break;

        if (ENABLE_DEBUG_MESSAGES) {
            printf("\n--------------------------------");
            printf(" ClOCK CYCLE %d ", cpu->clock + 1);
            printf("--------------------------------\n\n");
        }


        writeback(cpu);
        memory2(cpu);
        memory1(cpu);
        execute2(cpu);
        execute1(cpu);
        decode(cpu);
        fetch(cpu);
        cpu->clock++;
    }

    return 0;
}


int
simulate(APEX_CPU* cpu,int n){

    ENABLE_DEBUG_MESSAGES = 0;

    APEX_cpu_run(cpu, n);

    printf("\n ======================= STATE OF ARCHITECTURAL REGISTER FILE ========================\n\n");

    for(int i = 0; i < 16; i++)
    {
        printf("|\tREG[%d]\t\t|", i);
        printf("\tValue = %d\t\t|", cpu->regs[i]);
        if (cpu->regs_valid[i]==0){
            printf("\tStatus = INVALID\t|");
        }
        else{
            printf("\tStatus = VALID\t\t|");
        }
        printf("\n");
    }



    printf("\n ======================== STATE OF DATA MEMORY ======================= \n\n");

    for(int i = 0; i <100; i++)
    {
        printf("|\tMEM(%d)\t\t|", i);
        printf("\tData Value = %d\t\t|", cpu->data_memory[i]);
        printf("\n");
    }
    return 0;
}

int
display(APEX_CPU* cpu,int n){

    ENABLE_DEBUG_MESSAGES = 1;

    APEX_cpu_run(cpu, n);

    printf("\n ======================= STATE OF ARCHITECTURAL REGISTER FILE ========================\n\n");

    for(int i = 0; i < 16; i++)
    {
        printf("|\tREG[%d]\t\t|", i);
        printf("\tValue = %d\t\t|", cpu->regs[i]);
        if (cpu->regs_valid[i]==0){
            printf("\tStatus = INVALID\t|");
        }
        else{
            printf("\tStatus = VALID\t\t|");
        }
        printf("\n");
    }



    printf("\n ======================== STATE OF DATA MEMORY ======================= \n\n");

    for(int i = 0; i <100; i++)
    {
        printf("|\tMEM(%d)\t\t|", i);
        printf("\tData Value = %d\t\t|", cpu->data_memory[i]);
        printf("\n");
    }
    return 0;
}












