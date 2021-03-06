
/**
	obj3.c


	Simulator Project for COP 4600

	Objective 3 project that implements loading program scripts into
	memory for the OS simulator.

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

void print_free_list( );

/**
	Service user logon events.

	<b> Important Notes:</b>
	\li Events of type LOGON_EVT will result in this call via 
	Interrupt_Handler() in simulator.c after interrupt() in obj1.c sets the
	global Event variable.

	<b> Algorithm: </b>
	\verbatim
	Allocate new Process Control Block with all fields initialized to 0

	Initialize specific PCB fields:
		Mark status as new
		Save terminal table position
		Set logon time
		Set user name 

	Save PCB in global terminal table

	Initialize list of programs user will run--call Get_Script()
	Execute first program of PCB--call Next_pgm()

	If objective 4 or higher, set scheduling flags:
		If no process is active in the CPU currently
			Turn off scheduling and CPU switches
		Otherwise,
			Turn on switches
	\endverbatim

	@retval None
 */
void
Logon_Service( )
{
	//Allocate new Process Control Block with all fields initialized to 0
	pcb_type *newPCB = (pcb_type*)malloc(sizeof(pcb_type));
    memset(newPCB, 0, sizeof(pcb_type));

	//Initialize specific PCB fields:
	//Mark status as new
	newPCB->status = NEW_PCB;
	
	//Save terminal table position
	newPCB->term_pos = Agent - 1;
	
	//Set logon time
	newPCB->logon_time = Clock;
	
	//Set user name 
	char agentId[3];
	char agentName[5] = "U0";
	sprintf(agentId, "0%d", Agent);
	strcat(agentName,agentId);
	agentName[strlen(agentName)] = '\0';
	strcpy(newPCB->username, agentName);
	
	//Save PCB in global terminal table
	Term_Table[newPCB->term_pos] = newPCB;
	
	//Initialize list of programs user will run--call Get_Script()
	Get_Script(newPCB);
	
	//Execute first program of PCB--call Next_pgm()
	if(Next_pgm(newPCB) == 0)
	{
         err_quit("Transition to next program unsuccessful");
    }
	//If objective 4 or higher, set scheduling flags:
	if(Objective >= 4)
	{
		//If no process is active in the CPU currently
		if(CPU.active_pcb != NULL)
		{
			//Turn off scheduling and CPU switches
			SCHED_SW = OFF;
            CPU_SW = OFF;
		}
		//Otherwise,
		else
		{
			//Turn on switches
			SCHED_SW = ON;
            CPU_SW = ON;
		}
	}
}

/**
	Read set of all programs that a user will run.
	
	<b> Important Notes:</b>
	\li The global variable Script_fp is a file pointer to the script file to
	be read. It is opened by the simulator when it is initialized. Hence, it
	should neither be opened nor closed in this function--only read from.
	\li The script file has no specific format, that is, the names for all
	programs that all users will run appear together in this file and are not
	necessarily on different lines. Script names for a single user must be
	repeatedly read from the file until encountering the script name "LOGOFF",
	which marks the last program the user will run.
	\li Since the number of scripts is unknown, it should be allocated to have
	Max_Num_Scripts elements.
	\li The given PCB's 'scripts' array, after allocation, must set each entry
	to the script ID corresponding to each script name. Even after Get_Script()
	stores all script IDs, the end of the scripts array is not explicitly
	stored; however, this can still be determined as the last valid entry of the
	array has the LOGOFF ID.

	<b> Algorithm: </b>
	\verbatim
	Allocate PCB's array of scripts
	Mark index of first script

	Read each script name in file
		Capitalize script name so that case does not matter

		Output name of the script to output file

		Determine script ID for script name read
			Compare script name to each name in Prog_Names array
				If names match
				Mark ID in PCB's scripts array
		Stop reading script names when "LOGOFF" script name encountered
	\endverbatim

	@param[in,out] pcb -- process control block for the script being read
	@retval None
 */
void
Get_Script( pcb_type *pcb )
{
	char *scriptIn = (char*)malloc(sizeof(char) * (BUFSIZ));
	int progIndex, scriptIndex = 0;
	
	//Allocate PCB's array of scripts
	prog_type *scriptArray;
	scriptArray = (prog_type *) malloc(sizeof(prog_type)*Max_Num_Scripts);
	//Mark index of first script
	pcb->current_prog = 0;
	pcb->script = scriptArray;
	
	print_out("\tScript for user %s is:\n\t", pcb->username);
	//Read each script name in file
	do
	{
		fscanf(Script_fp, "%s", scriptIn);
		//Capitalize script name so that case does not matter
		strcpy(scriptIn, strupr(scriptIn));

		//Output name of the script to output file
		print_out("%s ",scriptIn);
		
		//Determine script ID for script name read
		for(progIndex = 0; progIndex < NUM_PROGRAMS; progIndex++)
		{
			//Compare script name to each name in Prog_Names array
			//If names match
			
			if(strcmp(scriptIn, Prog_Names[progIndex]) == 0)
			{
				//~ printf("%s\n",Prog_Names[progIndex] );
				//Mark ID in PCB's scripts array
				scriptArray[scriptIndex] = progIndex;
				break;
			}
		}
		scriptIndex++;
	}while(strcmp(scriptIn,"LOGOFF")!=0);	//Stop reading script names when "LOGOFF" script name encountered
	print_out("\n\n");
}

/**
	Load the next program script for the given PCB into memory.

	<b> Important Notes:</b>
	\li Despite the name of the function, the "next" program to run may
	actually be the very first program script that the user wishes to run--hence
	the initial check as the first step of the algorithm.

	This function makes the transition to the next program in the script
	for (pcb), if there is one. A 1 is returned if the transition to the
	next program is successful, a 0 otherwise. The transition is possible
	only if the RB-list for the current program is empty and the script
	is not empty.

	<b> Algorithm: </b>
	\verbatim
	If the next program is not the first program the user will run
		And
	No unserviced I/O request blocks exist
		Deallocate memory used for the previous program--call Dealloc_pgm()

	If process has an unserviced I/O request block
		Do not load a new program

	If encountered last program script
		Mark PCB as terminated and do not load another program
		Calculate total time logged on

	Allocate and initialize a new segment table for the PCB's program--call Get_Memory()
	If failed to allocate space (pcb->seg_table is NULL)
		Do not load another program

	Load the next program into memory--call Loader()

	Clear (set to 0) PCB's area for saving CPU information during an interrupt

	Mark PCB as ready to run

	If objective 4 or greater
		Insert process into CPU's ready queue 

	Increment index position of program script for next call to Next_pgm()
	\endverbatim

	@param[in,out] pcb -- PCB for which to load the next program
	@retval was_new_prog_loaded -- whether a new program was loaded (1) or
	not (0)
 */
int
Next_pgm( pcb_type* pcb )
{
	//If the next program is not the first program the user will run
		//And
	//No unserviced I/O request blocks exist
	if(pcb->current_prog != 0 && pcb->rb_q == NULL)
	{
		//Deallocate memory used for the previous program--call Dealloc_pgm()
		Dealloc_pgm(pcb);
	}

	//If process has an unserviced I/O request block
	if(pcb->rb_q != NULL)
	{
		//Do not load a new program
		return 0;
	}
		
	//If encountered last program script
	if(pcb->script[pcb->current_prog] == LOGOFF)
	{
		time_type currTime = Clock;
		//Mark PCB as terminated and do not load another program
		pcb->status = TERMINATED_PCB;
		
		//Calculate total time logged on
		Diff_time(&pcb->logon_time, &currTime);
		pcb->total_logon_time.seconds = currTime.seconds;
		pcb->total_logon_time.nanosec = currTime.nanosec;
		print_out("\t\tUser %s has logged off.\n",pcb->username);
		return 0;
	}

	//Allocate and initialize a new segment table for the PCB's program--call Get_Memory()
	Get_Memory(pcb);
	
	//If failed to allocate space (pcb->seg_table is NULL)
	if(pcb->seg_table == NULL)
	{
		//Do not load another program
		return 0;
	}

	//Load the next program into memory--call Loader()
	Loader(pcb);
	
	//Clear (set to 0) PCB's area for saving CPU information during an interrupt
	pcb->cpu_save.mode = 0;
	pcb->cpu_save.pc.segment = 0;
	pcb->cpu_save.pc.offset = 0;
	
	//Mark PCB as ready to run
	pcb->status = READY_PCB;

	//If objective 4 or greater
	if(Objective >= 4)
	{
		//Insert process into CPU's ready queue 
		Add_cpuq(pcb);
	}

	//Increment index position of program script for next call to Next_pgm()
	pcb->current_prog++;
	
	return 1;
}

/**
	Allocate space for a new by program by reading segment table information.

	<b> Important Notes:</b>
	\li The global variables Prog_Files and Prog_File_Names are arrays holding
	the file pointers to all program files and the respective program names.
	To access the needed program file, use the given PCB's current script ID to
	index into the PCB's script array. Then use this value to index into each
	array as each script entry holds a program ID of program the user is
	running. More simply put, just use pcb->script[ pcb->current_prog ] to
	index into either array (Prog_Files or Prog_File_Names) for the needed
	program.
	\li The file pointers held in Prog_Files are opened by the simulator when it
	is initialized. Hence, they should neither be opened nor closed in this
	function--only read from.
	\li The format of each program is the same as the other "program.dat"
	files as described in "intro.doc", so it is divided into two sections:
	segment table information and instructions. This is the same format as
	boot.dat in objective 2, except multiple programs may be in the same file
	as the same program type, such as the editor program, might be used by
	multiple users or multiple times by the same user.
	\li Once the segment table has been allocated and constructed, memory is
	allocated to each segment.
	\li Note that the call to Compact_mem() in the algorithm is guaranteed to
	free enough memory to load a new program since the total amount of free
	space is checked prior to the function call.

	<b> Algorithm: </b>
	\verbatim
	Read the fields "PROGRAM" and number of segments from program file

	Allocate pcb's segment table

	Read segment table information from program file:
		For each segment in user's program
			Read "SEGMENT size_of_segment access_bits" from program file
			Set size of current segment
			Set access bits of current segment
			Increment amount of memory used
			Go to next segment in user's segment table

	If program cannot fit into memory (total amount of free space < needed size)
		Clear user's segment table, which marks allocation failure
		Return without allocating a segment

	Allocate each segment:
		For each segment
			Try to allocate a new segment and save returned base pointer position--call Alloc_seg()

			If no space was available (invalid position returned)
				Compact memory to eliminate external fragmentation and try to get the base pointer again--call Alloc_seg()
	}
	\endverbatim

	@param[in,out] pcb -- process for which to allocate segment
	@retval None
 */
void
Get_Memory( pcb_type* pcb )
{
	int numSegs, size_of_segment, access_bits, currSeg, currIns, progSize = 0, baseMem;
	int currProg = pcb->script[ pcb->current_prog ];
	char buf[BUFSIZ];
	
	//Read the fields "PROGRAM" and number of segments from program file
	if(fscanf(Prog_Files[currProg], "%s %d ", buf,&numSegs) == EOF)
	{
		printf("File Empty");
		return;
	}
	
	//Allocate pcb's segment table
	pcb->num_segments = numSegs;
	pcb->seg_table = calloc(numSegs, sizeof(segment_type));

	//Read segment table information from program file:
	//For each segment in user's program
	for(currSeg = 0; currSeg < numSegs; currSeg++)
	{
		//Read "SEGMENT size_of_segment access_bits" from program file
		fscanf(Prog_Files[currProg], "%s %d %x ", buf, &size_of_segment, &access_bits);
		
		//Set size of current segment
		pcb->seg_table[currSeg].size = size_of_segment;
		
		//Set access bits of current segment
		pcb->seg_table[currSeg].access = access_bits;
		
		//Increment amount of memory used
		//Total_Free -= pcb->seg_table[currSeg].size;
		
		progSize += pcb->seg_table[currSeg].size;
		
		//Go to next segment in user's segment table
	}

	//If program cannot fit into memory (total amount of free space < needed size)
	if(Total_Free < progSize)
	{
		//Clear user's segment table, which marks allocation failure
		free(pcb->seg_table);
		pcb->seg_table = NULL;
		//Return without allocating a segment
		return;
	}

	//Allocate each segment:
	//For each segment
	for(currSeg = 0; currSeg < numSegs; currSeg++)
	{
		//Try to allocate a new segment and save returned base pointer position--call Alloc_seg()
		baseMem = Alloc_seg(pcb->seg_table[currSeg].size);
		//If no space was available (invalid position returned)
		if(baseMem < 0)
		{
			//Compact memory to eliminate external fragmentation and try to get the base pointer again--call Alloc_seg()
			Compact_mem();
            baseMem = Alloc_seg(pcb->seg_table[currSeg].size);
			return;
		}
		pcb->seg_table[currSeg].base = baseMem;
	}
}

/**
	Allocate segment of given size and return base memory pointer to this
	segment.

	This function searches the list of free memory segments for a segment
	of at least length 'size' and returns the address of this free segment.
	If the request cannot be satisfied, -1 is returned.

	<b> Important Notes:</b>
	\li The head of the free list is the pointed to by the global variable
	Free_Mem. The list is in order by base address of each free segment in the
	list. That is, if free node A has base address 5 and free node B has base
	address 10, then A precedes B in the list.
	\li The base addresses held by free list nodes are integer values denoting
	indices into memory. Hence, the index (array position) into memory (Mem)
	held by the free list node to be used should be returned, not '&Mem[i]'.

	<b> Algorithm: </b>
	\verbatim
	Search free list for a block large enough to hold the segment
		If block is large enough to hold segment
			Stop searching

	If no block was large enough
		Return invalid position

	Else, if block is an exact fit
		Use free block's base address for segment

		If block is first in list
			Set the head of the free list to the next node
		Else, segment is in middle or at end of list
			By-pass removed node in list

		Delete node
		Decrement total amount of free memory

	Else, if block is larger than needed
		Use free block's base address for segment

		Adjust node's size and starting position:
			Increment base address and decrement block size by given size

		Decrement total amount of free memory
	\endverbatim

	@param[in] size -- size of segment to allocate
	@retval segment_base -- base address segment should begin at in memory
 */
int
Alloc_seg( int size )
{
	seg_list *currBlock, *old;
	int segment_base;
	currBlock = Free_Mem;
	//Search free list for a block large enough to hold the segment
	while(currBlock)
	{
		//If block is large enough to hold segment
		if(currBlock->size >= size)
		{
			//Stop searching
			break;
		}
		old = currBlock;	
		currBlock = currBlock->next;
	}

	//If no block was large enough
	if(currBlock == NULL)
	{
		//Return invalid position
		return -1;
	}

	//Else, if block is an exact fit
	else if(currBlock->size == size)
	{
		//Use free block's base address for segment
		segment_base = currBlock->base;
		//If block is first in list
		if(currBlock == Free_Mem)
		{
			//Set the head of the free list to the next node
			Free_Mem = currBlock->next;
		}
		//Else, segment is in middle or at end of list
		else
		{
			//By-pass removed node in list
			old->next = currBlock->next;
		}

		//Delete node
		free(currBlock);
		//Decrement total amount of free memory
		Total_Free -= size;
		return segment_base;
	}
	//Else, if block is larger than needed
	else if(currBlock->size > size)
	{
		//Use free block's base address for segment
		segment_base = currBlock->base;
		
		//Adjust node's size and starting position:
		//Increment base address and decrement block size by given size
		currBlock->base += size;
		currBlock->size -= size;
		
		//Decrement total amount of free memory
		Total_Free -= size;
		return segment_base;
	}

	return -1;
}

/**
	Load user program from file into memory.

	<b> Important Notes:</b>
	\li Get_Instr() from objective 2 is used to read each instruction from the
	program file. Recall that Get_Instr() accepts two parameters: a program ID
	and an instruction. The program ID is an index into the array Prog_Files for
	the program file to read. This index is obtained in the same manner as in
	Get_Memory()--use pcb->script[ pcb->current_prog ]. The instruction is a
	pointer to memory at the position in which to store the instruction for the
	segment, i.e., &Mem[ base + offset ], where the base and offset are from
	the user's segment, should be passed to Get_Instr().
	\li The segment table, pcb->seg_table, contains the length of each segment
	and where it should be loaded into memory. The number of segments is given
	by pcb->num_segments.

	<b> Algorithm: </b>
	\verbatim
	For each segment in the user's segment table
		For each instruction in the segment
			Read instruction from program file into memory--call Get_Instr()
		Display each segment of program
	\endverbatim

	@retval None

	@param[in,out] pcb -- process for which to load segments
 */
void
Loader( pcb_type* pcb )
{
	int currSeg = 0, currInst = 0;
	//~ printf("numsegs = %d\n segsize = %d\n", pcb->num_segments, pcb->seg_table[currSeg].size);
	
	//For each segment in the user's segment table
	for(currSeg = 0; currSeg < pcb->num_segments; currSeg++)
	{
		//For each instruction in the segment
		for(currInst = 0; currInst < pcb->seg_table[currSeg].size; currInst++)
		{
			//Read instruction from program file into memory--call Get_Instr()
			Get_Instr(pcb->script[pcb->current_prog], &Mem[pcb->seg_table[currSeg].base + currInst]);
		}
		//Display each segment of program
		//~ printf("segsize = %d\n", pcb->seg_table[currSeg].size);
		Display_pgm(pcb->seg_table, currSeg, pcb);
	}
	print_out("\t\tProgram number %d, %s, has been loaded for user %s.\n\n",pcb->current_prog+1, Prog_Names[pcb->script[pcb->current_prog]], pcb->username);
}

/**
	Deallocate user's segments and segment table for current program.

	<b> Algorithm: </b>
	\verbatim
	For each segment
		Deallocate the segment--call Dealloc_seg()
	Free segment table
	\endverbatim

	@param[in,out] pcb -- process for which to deallocate segments
	@retval None
 */
void
Dealloc_pgm( pcb_type* pcb )
{
	int currSeg;
	//For each segment
	for(currSeg = 0; currSeg < pcb->num_segments; currSeg++)
	{
		//Deallocate the segment--call Dealloc_seg()
		Dealloc_seg(pcb->seg_table[currSeg].base, pcb->seg_table[currSeg].size);
	}
	//Free segment table
	free(pcb->seg_table);
}

/**
	Deallocate segment at given base position and size. 

	<b> Important Notes:</b>
	\li The elements in the free list are ordered by the address of the segment
	they represent.

	<b> Algorithm: </b>
	\verbatim
	Allocate and initialize a new free segment
	Increment total amount of free memory

	If free list is empty
		Set new segment as the start of the free list

	Else, if new segment goes at start of list
		Set new segment as the start of the free list

	Otherwise, new segment goes in the middle or at the end of the list:
		Search free list to find segments to the left and right of the segment being freed
			If previous segment's base < new segment's base < next segment's base
				Insert new segment between two segments in list

		If search failed, then the new segment belongs at end of the list
			Add new segment to end of the list

	Merge adjacent segments into a single, larger segment--call Merge_seg()
	\endverbatim

	@param[in] base -- position in memory (Mem array)
	@param[in] size -- size of memory block being freed
	@retval None
 */
void
Dealloc_seg( int base, int size )
{
	seg_list *newSeg, *currSeg = Free_Mem, *old = NULL;
	
	//Allocate and initialize a new free segment
	newSeg = (seg_list*) malloc(sizeof(seg_list));
	newSeg->base = base;
	newSeg->size = size;
	
	//Increment total amount of free memory
	Total_Free += size;

	//If free list is empty
	if(Free_Mem == NULL)
	{
		//Set new segment as the start of the free list
		newSeg->next = NULL;
		Free_Mem = newSeg;
	}
	
	else
	{
		while(currSeg)
		{
			if(currSeg->base > base)
			{
				//if new segment goes at start of list
				if(old == NULL)
				{
					//Set new segment as the start of the free list
					Free_Mem = newSeg;
					Free_Mem->next = currSeg;
					Merge_seg(old, newSeg, currSeg);
					return;
				}
				
				//Otherwise, new segment goes in the middle or at the end of the list:
				//Search free list to find segments to the left and right of the segment being freed
				//If previous segment's base < new segment's base < next segment's base
				//Insert new segment between two segments in list
				else
				{
					old->next = newSeg;
					newSeg->next = currSeg;
					Merge_seg(old, newSeg, currSeg);
					return;
				}
				
				old = currSeg;
				currSeg = currSeg->next;
			}
			//If search failed, then the new segment belongs at end of the list
			//Add new segment to end of the list
			else if(currSeg->next == NULL)
			{
				currSeg->next = newSeg;
				newSeg->next = NULL;
				Merge_seg(old, newSeg, newSeg->next);
				return;
			}
			old = currSeg;
			currSeg = currSeg->next;
		}
	}
}

/**
	Merge adjacent free blocks into a single, larger block (if possible).

	For example, suppose the newly freed segment starts at position 5 and ends
	at 10 while the next free segment starts at 11 and ends at 20. Then these
	two segments would be merged into a single segment starting at 5 and ending
	at 20.

	<b> Important Notes:</b>
	\li The elements in the free list are ordered by the address of the segment
	they represent.
	\li The total amount of free memory does not change due to merging.
	\li The new segment added to memory, via Dealloc_seg(), is given as the
	second parameter. Either prev_seg or next_seg (or both) may be NULL, but
	the algorithm accounts for this and does not attempt to merge.

	<b> Algorithm: </b>
	\verbatim

	If the previous free segment is not invalid
		And
	The new free segment follows directly after the previous segment
		Add the new segment's size to the previous segment
		Set previous segment's next link to new segment's next link
		Free node representing new segment
		Set new segment pointer to previous segment

	If the next free segment is not invalid
		And
	The new free segment directly precedes the next segment
		Add the next segment's size to the new segment
		Set new segment's next link to next segment's next link
		Free node representing next segment
	\endverbatim

	@param[in,out] prev_seg -- segment before new_seg to potentially merge
	with new_seg
	@param[in,out] new_seg -- new segment added to memory
	@param[in,out] next_seg -- segment after new_seg to potentially merge with
	new_seg
	@retval None
 */
void
Merge_seg( seg_list* prev_seg, seg_list* new_seg, seg_list* next_seg )
{
	//If the previous free segment is not invalid
		//And
	//The new free segment follows directly after the previous segment
	if(prev_seg != NULL && (prev_seg->base + prev_seg->size) == new_seg->base)
	{
		//Add the new segment's size to the previous segment
		prev_seg->size += new_seg->size;
		
		//Set previous segment's next link to new segment's next link
		prev_seg->next = new_seg->next;
		
		//Free node representing new segment
		free(new_seg);
		
		//Set new segment pointer to previous segment
		new_seg = prev_seg;
	}

	//If the next free segment is not invalid
		//And
	//The new free segment directly precedes the next segment
	if(next_seg != NULL && (new_seg->base + new_seg->size) == next_seg->base)
	{
		//Add the next segment's size to the new segment
		new_seg->size += next_seg->size;
		
		//Set new segment's next link to next segment's next link
		new_seg->next = next_seg->next;
		
		//Free node representing next segment
		free(next_seg);
	}
}

/**
	Service end program events.

	<b> Important Notes:</b>
	\li Events of type END_EVT will result in this call via 
	Interrupt_Handler() in simulator.c after interrupt() in obj1.c sets the
	global Event variable.
	\li The PCB status DONE_PCB is not the same as TERMINATED_PCB. Only the
	current program is ended, but the user's PCB may still be active as the user
	may be running multiple programs.

	<b> Algorithm: </b>
	\verbatim
	Retrieve PCB associated with program from terminal table
	Mark pcb as done
	Remove PCB from CPU's active process

	Calculate active time for process and busy time for CPU
   Record time process became blocked

	If objective 4 or higher
		Deallocate all I/O request blocks associated with PCB--call Purge_rb()

	If the PCB has no outstanding I/O request blocks
		Load the next program for the user--call Next_pgm()

	If objective 4 or higher
		Turn both the scheduling and CPU switches on in order to retrieve the next process to run
	\endverbatim

	@retval None
 */
void
End_Service( )
{
	//Retrieve PCB associated with program from terminal table
	pcb_type *PCB;
	PCB = Term_Table[Agent-1];
	
	//Mark pcb as done
	PCB->status = DONE_PCB;
	
	//Remove PCB from CPU's active process
	CPU.active_pcb = NULL;
	
	time_type *activeTime;
	*activeTime = Clock; 
	
	//Calculate active time for process and busy time for CPU
    Diff_time(&(PCB->run_time), activeTime);
    Add_time(activeTime, &(PCB->total_run_time));
    Add_time(activeTime, &(CPU.total_busy_time));
	
	print_out("\t\tProgram number %d, %s, has ended for user %s.\n", PCB->current_prog, Prog_Names[PCB->script[PCB->current_prog-1]], PCB->username);
    print_out("\t\tCPU burst was %u instructions.\n\n",PCB->sjnburst);
	
	//Record time process became blocked
	PCB->block_time = Clock;
	
	//If objective 4 or higher
	if(Objective >= 4)
	{
		//Deallocate all I/O request blocks associated with PCB--call Purge_rb()
		Purge_rb(PCB);
	}

	//If the PCB has no outstanding I/O request blocks
	if(PCB->rb_q == NULL)
	{
		//Load the next program for the user--call Next_pgm()
		Next_pgm(PCB);
	}
	
	//If objective 4 or higher
	if(Objective >= 4)
	{
		//Turn both the scheduling and CPU switches on in order to retrieve the next process to run
		SCHED_SW = ON;
        CPU_SW = ON;
	}
}

/**
	Service segmentation fault and address fault events.

	<b> Important Notes:</b>
	\li This function services causes an abnormal end to the simulated program.
	\li Events of type SEGFAULT_EVT and ADRFAULT_EVT will result in this call
	via Interrupt_Handler() in simulator.c after interrupt() in obj1.c sets the
	global Event variable.
	\li Use print_out() function for output.

	<b> Algorithm: </b>
	\verbatim
	Print output message indicating that process was terminated due to some abnormal condition
	Call End_Service() to handle terminating process.
	\endverbatim

	@retval None
 */
void
Abend_Service( )
{
	//Print output message indicating that process was terminated due to some abnormal condition
	print_out("Process terminated due to some abnormal condition\n");
    
	//Call End_Service() to handle terminating process.
	End_Service();
}

/**
	Print contents of free list.

	<b> Important Notes:</b>
	\li This function should be useful for debugging and verifying memory
	allocation and deallocation functions, especially Merge_seg().

	<b> Algorithm: </b>
	\verbatim
	For each free block
		Print base position and last position in block
	\endverbatim
 */
void print_free_list( )
{
	int currBlock = 0;
	seg_list* currNode = Free_Mem;
	//For each free block
	while(currNode)
	{
		//Print base position and last position in block
		printf("Free Block: %d Base: %d End: %d\n", currBlock, Free_Mem->base, (Free_Mem->base + Free_Mem->size - 1));
		currNode = currNode->next;
		currBlock++;
	}
}

/**
	Print debugging message about Term_Table.

	<b> Important Notes:</b>
	\li The debugging message is only printed if the variable DEBUG_PCB is
	turned on (set to 1). This is done by adding the following line to
	config.dat: <br>
	DEBUG_PCB= ON
	\li The function is not necessary to implement as it serves only to help
	the programmer in debugging the implementation of common data structures.

	@retval None
 */
void
Dump_pcb( )
{
	// if debug flag is turned on
	if( DEBUG_PCB )
	{
		/* print debugging message */
	}
	else // do not print any message
	{ ; }
}
