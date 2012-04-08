/**
	obj5.c


	Simulator Project for COP 4600

	Objective 5 project that calculates OS statistics. 

	Revision List:
		Original Design:  Dr. David Workman, 1990
		Revised by Tim Hughey and Mark Stephens, 1993
		Revised by Wade Spires, Spring 2005
		Minor Revisions by Sean Szumlanski, Spring 2010
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "osdefs.h"
#include "externs.h"

/**
	Compute all simulation statistics and store them in the appropriate
	variables and data structures for display.  It is called by Clean_Up().

	<b> Algorithm: </b>
	\verbatim
	For each user
		Calculate total processing time
		Calculate total job blocked time
		Calculate total job wait time
		Calculate total job execution time
		Calculate efficiency for each process

	For each device
		Calculate response time for each device
		Calculate total idle time for each device
		Calculate utilization for each device

	Calculate average user execution time
	Calculate average user logon time
	Calculate average user blocked time
	Calculate average user wait time
	Calculate response time for CPU
	Calculate total idle time for CPU
	Calculate total utilization for CPU
	\endverbatim
 */
void
Calc_Stats( )
{
	int i;
	time_type runTime, devResponseTime, cpuResponseTime;
	
	//For each user
	for(i = 0; i < Num_Terminals; i++)
	{
		//Calculate total processing time
		Add_time(&Term_Table[i]->total_logon_time, &Tot_Logon);
		
		//Calculate total job blocked time
		Add_time(&Term_Table[i]->total_block_time,&Tot_Block);
		
		//Calculate total job wait time
		Add_time(&Term_Table[i]->total_ready_time, &Tot_Wait);

		//Calculate total job execution time
		Add_time(&Term_Table[i]->total_run_time, &Tot_Run);
		
		//Calculate efficiency for each process
		runTime = Term_Table[i]->total_run_time;
		Add_time(&Term_Table[i]->total_block_time, &runTime);
		Term_Table[i]->efficiency = 100.0 * Divide_time(&runTime, &Term_Table[i]->total_logon_time);
	}
	
	//For each device
	for(i = 0; i < Num_Devices; i++)
	{
		//Calculate response time for each device
		devResponseTime = Dev_Table[i].total_q_time;
        Add_time(&Dev_Table[i].total_busy_time, &devResponseTime);
        Average_time(&devResponseTime, Dev_Table[i].num_served, &Dev_Table[i].response); 

		//Calculate total idle time for each device
		Dev_Table[i].total_idle_time = Clock;
		Diff_time(&Dev_Table[i].total_busy_time, &Dev_Table[i].total_idle_time);
		
		//Calculate utilization for each device
        Dev_Table[i].utilize = 100 * Divide_time(&Dev_Table[i].total_busy_time, &Clock);
	}
	
	//Calculate average user execution time
	Average_time(&Tot_Run, Num_Terminals, &Avg_Run);

	//Calculate average user logon time
	Average_time(&Tot_Logon, Num_Terminals, &Avg_Logon);

	//Calculate average user blocked time
	Average_time(&Tot_Block, Num_Terminals, &Avg_Block);

	//Calculate average user wait time
	Average_time(&Tot_Wait, Num_Terminals, &Avg_Wait);

	//Calculate response time for CPU
	cpuResponseTime = CPU.total_q_time;
	Add_time(&CPU.total_busy_time, &cpuResponseTime);
	Average_time(&cpuResponseTime, CPU.num_served, &CPU.response);

	//Calculate total idle time for CPU
	CPU.total_idle_time = Clock;
	Diff_time(&CPU.total_busy_time, &CPU.total_idle_time);

	//Calculate total utilization for CPU
	CPU.utilize = 100.0 * Divide_time(&CPU.total_busy_time, &Clock);
}
