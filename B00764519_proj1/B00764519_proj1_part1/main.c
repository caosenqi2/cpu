/*
 *  main.c
 *
 *  Author :
 *  Gaurav Kothari (gkothar1@binghamton.edu)
 *  State University of New York, Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"

int
main(int argc, char const* argv[])
{
  if (argc != 4) {
    fprintf(stderr, "APEX_Help : Usage %s <input_file>\n ...", argv[0]);
    exit(1);
  }

  APEX_CPU* cpu = APEX_cpu_init(argv[1]);
  if (!cpu) {
    fprintf(stderr, "APEX_Error : Unable to initialize CPU\n");
    exit(1);
  }
  
  
  
  if(strcmp(argv[2], "simulate") == 0)
  simulate(cpu,atoi(argv[3]));
  
  if(strcmp(argv[2], "display") == 0)
  display(cpu,atoi(argv[3]));

  //APEX_cpu_run(cpu);
  //APEX_cpu_stop(cpu);
  return 0;
}
