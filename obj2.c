/**
	obj2.c


	Simulator Project for COP 4600

	Objective 2 project that implements loading and executing the boot
	program for the OS simulator.

	Revision List:
		Original Design:  Dr. David Workman, 1990
		Revised by Tim Hughey and Mark Stephens, 1993
		Revised by Wade Spires, Spring 2005
		Minor Revisions by Sean Szumlanski, Spring 2010
*/

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "osdefs.h"
#include "externs.h"

/**
	Simulate booting the operating system by loading the boot program into
	kernel memory.

	This function is called from simulator.c (the simulator driver)
	before processing the event list. Boot() reads in the file boot.dat to load
	the kernel and initialize Mem_Map for the first program execution. Note that
	boot.dat is already open when Boot() is called and can immediately be read
	using the file pointer Prog_Files[BOOT] (Prog_Files is an array of file
	pointers opened by the simulator).

	The upper half of Mem_Map always holds the address map for the operating
	system, i.e., Mem_Map[Max_Segments ... 2*Max_Segments - 1] (the lower half
	is reserved for user programs). The PROGRAM and SEGMENT statements in the
	boot file are used to initialize the appropriate entries in Mem_Map. The
	actual kernel script is loaded into Mem (array representing physical
	memory) starting at absolute memory location 0. The Free_Mem list
	and Total_Free must be updated as the program is loaded to account for
	the region of Mem that is occupied by the kernel. 

	The format of boot.dat is the same as the other "program.dat"
	files, so it is divided into two sections: segment table information and
	instructions. To load the OS program, Boot() calls the Get_Instr() routine
	to read and process each line in the data file. Once the line is read, it is
	then stored in Mem at the appropriate absolute memory address. Once
	the entire OS program is loaded, Boot() should call Display_pgm() to echo 
	the contents of Mem to the output file. 

	<b> Important Notes:</b>
	\li Boot() is only active during objective 2, so if the current objective is not 2, then simply return from the function.
	\li The boot file, boot.dat, is already open when Boot() is called and can immediately be read using the file pointer Prog_Files[BOOT] (Prog_Files is an array of file pointers opened by the simulator).
	\li Get_Instr() is used by the function Loader() in objective 3.

	<b> Algorithm: </b>
	\verbatim
	Read fields the "PROGRAM" and number of segments from boot file

	Read segment information from boot file:
		For each segment in boot file
			Read "SEGMENT size_of_segment access_bits" from boot file
			Set size of current segment
			Set access bits of current segment
			Go to next segment in Mem_Map

	Read instructions from boot file:
		Current position in main memory--incremented for each instruction read

		For each segment
			Set base pointer of segment to its location in main memory
			For each instruction in the segment
				Read instruction into memory--call Get_Instr()
				Increment current position in main memory
			Go to next segment in Mem_Map

		Adjust size of memory since boot program is loaded into memory:
			Decrement total amount of free memory (Total_Free)
			Decrement size of free memory block (Free_Mem->size)
			Move location of first free memory block (Free_Mem->base)

		Display each segment of memory
			Display segment Mem_Map[i + Max_Segments] since kernel resides in
			Upper half of Mem_Map; pass NULL as PCB since OS has no PCB
	\endverbatim

	@retval None
 */
void
Boot( )
{
	/** if the current objective is not 2, then simply return from the function */
	if(Objective != 2)
		return;
		
	int numSegs, size_of_segment, access_bits, currSeg, currIns, currMem = 0;
	char buf[BUFSIZ];
	int test = 0;
	
	//Read fields the "PROGRAM" and number of segments from boot file
	if(fscanf(Prog_Files[BOOT], "%s %d", buf,&numSegs) == EOF)
	{
		printf("File Empty");
		return;
	}
		
	//Read segment information from boot file:
	//For each segment in boot file
	for(currSeg = 0; currSeg < numSegs; currSeg++)
	{
		//Read "SEGMENT size_of_segment access_bits" from boot file
		fscanf(Prog_Files[BOOT], "%s %d %x", buf, &size_of_segment, &access_bits);
		//Set access bits of current segment
		Mem_Map[currSeg + Max_Segments].access = access_bits;
		//Set size of current segment
		Mem_Map[currSeg + Max_Segments].size = size_of_segment;
		
		//Go to next segment in Mem_Map
	}
	
	//Read instructions from boot file:
	//Current position in main memory--incremented for each instruction read
	//For each segment
	for(currSeg = 0; currSeg < numSegs; currSeg++)
	{
		//Set base pointer of segment to its location in main memory
		Mem_Map[currSeg + Max_Segments].base = currMem;

		//For each instruction in the segment
		for(currIns = 0; currIns < Mem_Map[currSeg + Max_Segments].size; currIns++)
		{
			//Read instruction into memory--call Get_Instr()
			Get_Instr( BOOT, &Mem[currMem] );
			//Increment current position in main memory
			currMem++;
			
			//Go to next segment in Mem_Map			
		}
	}
	
	//Adjust size of memory since boot program is loaded into memory:
	
	//Decrement total amount of free memory (Total_Free)
	Total_Free -= currMem;
	//Decrement size of free memory block (Free_Mem->size)
	Free_Mem->size -= currMem;
	//Move location of first free memory block (Free_Mem->base)
	Free_Mem->base += currMem;
	
	//Display each segment of memory
	for(currSeg = 0; currSeg < numSegs; currSeg++)
	{
		//Display segment Mem_Map[i + Max_Segments] since kernel resides in
		//Upper half of Mem_Map; pass NULL as PCB since OS has no PCB
		Display_pgm( Mem_Map, (currSeg + Max_Segments), NULL );
	}
}

/**
	Read the next instruction from the file Prog_Files[prog_id] into
	instruction.  

	The format of the file is a series of statements of the form
	'opcode operand'. opcode may be an actual instruction, such as SIO, or a
	device, such as DISK. The operand will vary depending on the type of opcode
	read. Each instruction starts on a new line, but consecutive instructions
	may be separated by empty lines. 

	<b> Important Notes:</b>
	\li The operand field of an instr_type structure is defined as a C union, so
	it may represent one of many values but only one at a time. For example,
	instruction->operand.burst is set for an SIO instruction, but
	instruction->operand.count is set for a SKIP instruction.
	\li For device instruction, assign unique opcodes to each device by adding
	NUM_OPCODES to its corresponding dev_id so device codes do not conflict with
	regular op_codes
	\li Get_Instr() is used by Boot() in objective 2 and Loader() in objective
	3.

	<b> Algorithm: </b>
	\verbatim
	Read opcode and operand from file--store as strings initially Convert to uppercase so that case does not matter

	Search all op_code IDs to determine which op_code that the opcode_str matches with
		If found the correct op_code
			Save the opcode in the instruction

			Process the operand differently depending on the op_code type:
				If SIO, WIO, or END instruction
					Convert operand into burst count
				Else, if REQ or JUMP instruction
					Convert operand into segment and offset pair
				Else, if SKIP instruction
					Convert operand into skip count

	If no op_codes matched, the op_code may correspond to a device
		Search device table for the particular device
			If found the correct device ID
				Assign a unique opcode for the device (See note above)
				Convert operand into number of bytes to transfer
		If no devices matched, quit program
	\endverbatim 


	@param[in] prog_id -- index into Prog_Files to read instruction from
	@param[out] instruction -- location to store instruction read
 */
void
Get_Instr( int prog_id, struct instr_type* instruction )
{
	char opcode_str[BUFSIZ], operand_str[BUFSIZ];
	int counter;
	char *p, *q;
	
	//Read opcode and operand from file--store as strings initially Convert to uppercase so that case does not matter
	fscanf(Prog_Files[prog_id], "%s %s ", opcode_str, operand_str);
	for (p = opcode_str; *p != '\0'; ++p)
	{
		*p = toupper(*p);
	}
	for (q = operand_str; *q != '\0'; ++q)
	{
		*q = toupper(*q);
	}
	
	//Search all op_code IDs to determine which op_code that the opcode_str matches with
	for(counter = 0; counter < NUM_OPCODES; counter++)
	{
		//If found the correct op_code
		if(strcmp(Op_Names[counter], opcode_str) == 0)
		{
			//Save the opcode in the instruction
			instruction->opcode = counter; 
			
			//Process the operand differently depending on the op_code type:
				
			//If SIO, WIO, or END instruction
			if(counter == 0 || counter == 1 || counter == 5)
			{
				//Convert operand into burst count
				instruction->operand.burst = strtoul(operand_str,NULL,10);
			}
				
			//Else, if REQ or JUMP instruction
			else if(counter == 2 || counter == 3)
			{
				//Convert operand into segment and offset pair
				sscanf(operand_str, "[%d, %d]", &instruction->operand.address.segment, &instruction->operand.address.offset);
			}
			
			//Else, if SKIP instruction
			else if(counter == 4)
			{
				//Convert operand into skip count
				instruction->operand.count = atoi(operand_str);
			}
			return;
		}
	}
	
	//If no op_codes matched, the op_code may correspond to a device
	
	//Search device table for the particular device
	for(counter = 0; counter < Num_Devices; counter++)
	{
		//If found the correct device ID
		if(strcmp(Dev_Table[counter].name, opcode_str) == 0)
		{
			//Assign a unique opcode for the device (See note above)
			instruction->opcode = counter + NUM_OPCODES; 
			//Convert operand into number of bytes to transfer
			instruction->operand.bytes = strtoul(operand_str,NULL,10);
			return;
		}
	}
		
	//If no devices matched, quit program
	err_quit("\nNO DEVICES MATCHED\n");
}

/**
	Simulate the Central Processing Unit.

	The CPU, in general, does the following:
	\li Fetches the instruction from memory (Mem)
	\li Interprets the instruction.
	\li Creates a future event based on the type of instruction and
	inserts the event into the event queue.

	Set_MAR() is called to define the next memory location for a Fetch().
	The first fetch from memory is based on CPU.state.pc. Instructions that
	create events are:
	\li WIO   							
	\li SIO							
	\li END							
	The SKIP and JUMP instructions are executed in "real-time" until
	one of the other instructions causes a new event.

	An addressing fault could be generated by a call to Fetch() or Write(), so
	Cpu() should just return in this case (the simulator will later call
	Abend_Service() from the interrupt handler routine to end the simulated
	program).

	<b> Important Notes:</b>
	\li Use a special agent ID, i.e., 0, to identify the boot program. For
	all other objectives, the agent ID should be retrieved from the CPU's
	currently running process--use this process's terminal position and add 1
	so that it does not conflict with boot.
	\li The only instructions processed by Cpu() are SIO, WIO, END, SKIP, and
	JUMP. No other instructions, such as REQ and device instructions, are
	encountered in Cpu(). This is by incrementing the program counter by 2 to
	bypass these unused instructions.
	\li A while(1) loop is needed to correctly evaluate SKIP and JUMP
	instructions since multiple SKIP and JUMP instructions may be encountered
	in a row. The loop is exited when an SIO, WIO, or END instruction is
	encountered.
	\li The function Burst_time() should be used to convert CPU cycles for SIO,
	WIO, and END instructions to simulation time (time_type).

	<b> Algorithm: </b>
	\verbatim
	Identify the agent ID for the currently running program:
		If CPU has no active PCB, then the program is boot
		Otherwise, the program is the active process running in the CPU, so use its ID

	Loop forever doing the following:
		Set MAR to CPU's program counter
		Fetch instruction at this address
		If fetch returns a negative value, a fault has occurred, so
			Return

		Determine type of instruction to execute
			If SIO, WIO, or END instruction
				If the objective is 3 or higher
					Increment total number of burst cycles for PCB
				Calculate when I/O event will occur using current time + burst time
				Add event to event list
				Increment PC by 2 to skip the next instruction--device instruction
				Return from Cpu() (exit from loop)

			If SKIP instruction
				If count > 0,
					Decrement count by 1
					Write this change to memory by calling Write()
					If write returns a negative value, a fault has occurred, so
						Return
					Increment the CPU's PC by 2 since the next instruction is to be skipped
				Otherwise, instruction count equals 0, so
					Increment the CPU's PC by 1 since the next instruction, JUMP, is to be executed
				Continue looping

			If JUMP instruction
				Set the PC for the CPU so that the program jumps to the address determined by the operand of instruction
				Continue looping
	\endverbatim 

	\retval None
 */
void
Cpu( )
{
	time_type *eventTime;
	int counter;
	instr_type *instruction;
	instruction = (instr_type *) malloc(sizeof(instr_type));
	eventTime = (time_type *) malloc(sizeof(time_type));
	//Identify the agent ID for the currently running program:

	//If CPU has no active PCB, then the program is boot
	if(CPU.active_pcb == NULL)
	{
		Agent = 0;
	}
	//Otherwise, the program is the active process running in the CPU, so use its ID
	//use this process's terminal position and add 1
	//so that it does not conflict with boot.
	else
	{
		Agent = CPU.active_pcb->term_pos + 1;
	}
	//Loop forever doing the following:
	while(1)
	//~ for(counter = 0; counter < 3; counter++)
	{
		
		//Set MAR to CPU's program counter
		Set_MAR(&CPU.state.pc);
		//Fetch instruction at this address
		int fetch = Fetch(instruction);
		//If fetch returns a negative value, a fault has occurred, so
		if(fetch < 0)
		{
			//Return
			return;
		}

		//Determine type of instruction to execute
		
		//If SIO, WIO, or END instruction
		if(instruction->opcode == 0 || instruction->opcode == 1 || instruction->opcode == 5)
		{
			//If the objective is 3 or higher
			if(Objective >= 3)
			{
				//Increment total number of burst cycles for PCB
				CPU.active_pcb->sjnburst++;
				//~ CPU.active_pcb->sjnburst += instruction->operand.burst;
			}
			
			//Calculate when I/O event will occur using current time = clock + burst time 
			Burst_time(instruction->operand.burst, eventTime);
			Add_time(&Clock, eventTime);

			//Add event to event list
			int eventID;
			switch(instruction->opcode)
			{
				case SIO_OP:
					eventID = SIO_EVT;
					break;
				case WIO_OP:
					eventID = WIO_EVT;
					break;
				case END_OP:
					eventID = END_EVT;
					break;
			}
			Add_Event(eventID, Agent, eventTime);
			//Increment PC by 2 to skip the next instruction--device instruction
			CPU.state.pc.offset += 2;
			//Return from Cpu() (exit from loop)
			return;
		}

		//If SKIP instruction
		else if(instruction->opcode == 4)
		{
			//If count > 0,
			if(instruction->operand.count > 0)
			{
				//Decrement count by 1
				instruction->operand.count--;
				
				//Write this change to memory by calling Write()
				int write = Write(instruction);
				//If write returns a negative value, a fault has occurred, so
				if(write < 0)
				{
					//Return
					return;
				}
				//Increment the CPU's PC by 2 since the next instruction is to be skipped
				CPU.state.pc.offset += 2;
			}
			
			//Otherwise, instruction count equals 0, so
			else
			{
				//Increment the CPU's PC by 1 since the next instruction, JUMP, is to be executed
				CPU.state.pc.offset++;
			}
			//Continue looping
		}

		//If JUMP instruction
		else if(instruction->opcode == 3)
		{
			//Set the PC for the CPU so that the program jumps to the address determined by the operand of instruction
			CPU.state.pc.segment = instruction->operand.address.segment;
			CPU.state.pc.offset = instruction->operand.address.offset;
			//Continue looping
		}
	}
}

/**
	Simulate a context switch that places a user program in execution.

	<b> Algorithm: </b>
	\verbatim
	Copy the given state into the CPU's state
	Resume execution of the program at the new PC--just call Cpu()
	\endverbatim

	@param[in] state -- program state to load into CPU
 */
void
Exec_Program( struct state_type* state )
{
	//Copy the given state into the CPU's state
	CPU.state = *state;
	//~ printf("mode = %x\nseg = %d\noff = %d\n", CPU.state.mode, CPU.state.pc.segment, CPU.state.pc.offset);
	
	//Resume execution of the program at the new PC--just call Cpu()
	Cpu();
}

/**
	Simulate the Memory Management Unit (MMU) that translates the contents of
	the Memory Address Register (MAR) to a real physical address.

	Use the contents of MAR = [segment,offset] as the logical address to be
	translated to a physical address.

	<b> Algorithm: </b>
	\verbatim
	Set segment to the segment saved in the MAR:
		If in kernel mode (CPU's mode equals 1)
			Set segment to be in upper half of Mem_Map (MAR.segment+Max_Segments)
		Otherwise, in user mode (CPU's mode equals 0)
			Just use lower half of Mem_Map (MAR.segment)

	If illegal memory access (access == 0)
		Create seg. fault event at the current time using the CPU's active process's terminal position (+ 1) as the agent ID
		Return -1

	If address is out of the bounds
		Create address fault event at the current time using the CPU's active process's terminal position (+ 1) as the agent ID
		Return -1

	Compute physical memory address:
		Address = base address from segment in Mem_Map + offset saved in MAR

	Return address
	\endverbatim

	@retval address -- physical address into Mem obtained from logical address
		stored in MAR
 */
int
Memory_Unit( )
{
	//Set segment to the segment saved in the MAR:
	int currSeg, currAddress;

	//If in kernel mode (CPU's mode equals 1)
	if(CPU.state.mode == 1)
	{
		//Set segment to be in upper half of Mem_Map (MAR.segment+Max_Segments)
		currSeg = MAR.segment + Max_Segments;
	}
	
	//Otherwise, in user mode (CPU's mode equals 0)
	else
	{
		//Just use lower half of Mem_Map (MAR.segment)
		currSeg = MAR.segment;
	}
	
	//If illegal memory access (access == 0)
	if(Mem_Map[currSeg].access == 0)
	{
		//Create seg. fault event at the current time using the CPU's active process's terminal position (+ 1) as the agent ID
		Add_Event(SEGFAULT_EVT, CPU.active_pcb->term_pos + 1, &Clock);
		
		//Return -1
		return -1;
	}

	//If address is out of the bounds
	if(currSeg > (Max_Segments*2 - 1))
	{
		//Create address fault event at the current time using the CPU's active process's terminal position (+ 1) as the agent ID
		Add_Event(ADRFAULT_EVT, CPU.active_pcb->term_pos + 1, &Clock);
		//Return -1
		return -1;
	}
	
	//Compute physical memory address:
	
	//Address = base address from segment in Mem_Map + offset saved in MAR
	currAddress = Mem_Map[currSeg].base + MAR.offset;
	
	//Return address
	return currAddress;
}

/**
	Set Memory Address Register (MAR) to the value of the given address.

	This function must be called to define the logical memory address of the
	next Fetch(), Read(), or Write() operation in memory.

	<b> Algorithm: </b>
	\verbatim
	Set MAR to given address
	\endverbatim

	@param[in] addr -- address to set MAR to
 */
void
Set_MAR( struct addr_type* addr )
{
	//Set MAR to given address
	MAR = *addr;
}

/**
	Fetch next instruction from memory. 

	<b> Important Notes:</b>
	\li Fetch() is identical to Read()

	<b> Algorithm: </b>
	\verbatim
	Get current physical memory address--call Memory_Unit()
	If returned address < 0, then a fault occurred
		Return -1

	Set given instruction to instruction in memory at the current memory address
	Return 1 to signify that no errors occurred
	\endverbatim

	@param[out] instruction -- location to store instruction fetched from memory
	@retval status -- whether an error occurred or not (-1 or 1)
 */
int
Fetch( struct instr_type* instruction )
{
	int physAddr;
	//Get current physical memory address--call Memory_Unit()
	physAddr = Memory_Unit();
	
	//If returned address < 0, then a fault occurred
	if(physAddr < 0)
	{
		//Return -1
		return -1;
	}
	
	//Set given instruction to instruction in memory at the current memory address
	*instruction = Mem[physAddr];
	
	//Return 1 to signify that no errors occurred
	return 1;
}

/**
	Read next instruction from memory. 

	<b> Important Notes:</b>
	\li Read() is identical to Fetch()

	<b> Algorithm: </b>
	\verbatim
	Get current physical memory address--call Memory_Unit()
	If returned address < 0, then a fault occurred
		Return -1

	Set given instruction to instruction in memory at the current memory address
	Return 1 to signify that no errors occurred
	\endverbatim

	@param[out] instruction -- location to store instruction read from memory
	@retval status -- whether an error occurred or not (-1 or 1)
 */
int
Read( struct instr_type* instruction )
{
	int physAddr;
	//Get current physical memory address--call Memory_Unit()
	physAddr = Memory_Unit();
	
	//If returned address < 0, then a fault occurred
	if(physAddr < 0)
	{
		//Return -1
		return -1;
	}
	
	//Set given instruction to instruction in memory at the current memory address
	*instruction = Mem[physAddr];
	
	//Return 1 to signify that no errors occurred
	return 1;
}

/**
	Write instruction to memory. 

	<b> Algorithm: </b>
	\verbatim
	Get current physical memory address--call Memory_Unit()
	If returned address < 0, then a fault occurred
		Return -1

	Set instruction in memory at the current memory address to given instruction
	Return 1 to signify that no errors occurred
	\endverbatim

	@param[in] instruction -- instruction to write to memory
	@retval status -- whether an error occurred or not (-1 or 1)
 */
int
Write( struct instr_type* instruction )
{
	int physAddr;
	//Get current physical memory address--call Memory_Unit()
	physAddr = Memory_Unit();
	
	//If returned address < 0, then a fault occurred
	if(physAddr < 0)
	{
		//Return -1
		return -1;
	}
	
	//Set instruction in memory at the current memory address to given instruction
	Mem[physAddr] = *instruction;
	
	//Return 1 to signify that no errors occurred
	return 1;
}

/**
	Display contents of program to output file.

	Refer to intro.doc for the proper output format.
	Only a single segment (seg_num) of the segment table is displayed per
	call of Display_pgm(). Multiple calls of Display_pgm() must be made to
	display the entire table.

	<b> Important Notes:</b>
	\li Call print_out() to print to the output file.
	\li For objective 2, the given pcb will be NULL, so print the name as
	"BOOT".
	\li For objectives other than 2, use
	Prog_Names[ pcb->script[ pcb->current_prog ] ] to get the program name.
	Prog_Names is a global array holding the names of all programs the
	simulator can run. pcb->script is an array of all the programs that the
	process has (or will) run, and pcb->current_prog is the index into
	pcb->script of the program that the user is currently running, that is, the
	one that needs to be displayed.
	\li If instruction is not a regular instruction, e.g., not SIO, WIO, etc.,
	then it is a device. The location of the device in Dev_Table (to get its
	name) is obtained by taking the opcode of the current memory position -
	NUM_OPCODES since NUM_OPCODES was added to device op_codes in Get_Instr().

	<b> Algorithm: </b>
	\verbatim
	Print segment header:
		If boot process
			Print "BOOT"
		Else, if user process
			Print name of user's program
	Print additional header information

	Display current segment of memory:
		Get base memory position of the first instruction in the segment

		For each instruction in the segment
			If opcode is an instruction, look for the appropriate one in Op_Names
				Display memory position and instruction code

				Determine instruction type to display operand differently:
					If SIO, WIO, or END instruction
						Display burst count

					Else, if REQ or JUMP instruction
						Display segment/offset pair

					Else, if SKIP instruction
						Display skip count
			Else, opcode is a device
				Look for it in devtable
				Display memory position, device name, and byte count

			Update base memory position to go to next instruction in segment
	\endverbatim

	@param[in] seg_table -- segment table to display
	@param[in] seg_num -- segment within table to display (seg_table[ seg_num ])
	@param[in] pcb -- process control block that the segment table belongs to
	(NULL if table belongs to boot process)
 */
void
Display_pgm( segment_type* seg_table, int seg_num, pcb_type* pcb )
{	
	
	int counter1, counter2;
	//Print segment header:
	
	//If boot process
	if(Objective == 2)
	{
		//Print "BOOT"
		print_out("        SEGMENT #%d OF PROGRAM BOOT OF PROCESS BOOT\n", seg_num);
	}
	
	//Else, if user process
	else
	{
		//Print name of user's program
		print_out("        SEGMENT #%d OF PROGRAM %s OF PROCESS %s\n", seg_num, Prog_Names[ pcb->script[ pcb->current_prog ] ], pcb->username);
	}
	
	//Print additional header information
	print_out("      ACCBITS: %02X  LENGTH: %d\n", seg_table[seg_num].access, seg_table[seg_num].size);
	print_out("        MEM ADDR  OPCODE  OPERAND\n        --------  ------  -------\n");
	
	//Display current segment of memory:
	//Get base memory position of the first instruction in the segment
	int base = seg_table[seg_num].base;

	//For each instruction in the segment
	for(counter1 = 0; counter1 < seg_table[seg_num].size; counter1++)
	{
		//If opcode is an instruction, look for the appropriate one in Op_Names
		if(Mem[base].opcode < NUM_OPCODES)
		{ 
			//Display memory position and instruction code
			print_out("               %d  %s", base, Op_Names[Mem[base].opcode]);
			
			//Determine instruction type to display operand differently:			
			//If SIO, WIO, or END instruction
			if(Mem[base].opcode == 0 || Mem[base].opcode == 1 || Mem[base].opcode == 5)
			{
				//Display burst count
				print_out("     %lu\n", Mem[base].operand);
				printf("opcode = %d\tburst = %lu\n", Mem[base].opcode, Mem[base].operand);
			}
			
			//Else, if REQ or JUMP instruction
			else if(Mem[base].opcode == 2 || Mem[base].opcode == 3)
			{
				//Display segment/offset pair
				if(Mem[base].opcode == 2)
					print_out("     [%d,%d]\n", Mem[base].operand.address.segment, Mem[base].operand.address.offset);
				else if(Mem[base].opcode == 3)
					print_out("    [%d,%d]\n", Mem[base].operand.address.segment, Mem[base].operand.address.offset);
			}
			
			//Else, if SKIP instruction
			else if(Mem[base].opcode == 4)
			{
				//Display skip count
				print_out("    %d\n", Mem[base].operand);
			}		
		}
		
		//Else, opcode is a device
		else
		{
			//Look for it in devtable
			for(counter2 = 0; counter2 < Num_Devices; counter2++)
			{
				//If found the correct device ID
				if(strcmp(Dev_Table[counter2].name, Dev_Table[Mem[base].opcode - NUM_OPCODES].name) == 0)
				{
					//Display memory position, device name, and byte count
					print_out("               %d  %s    %lu\n", base, Dev_Table[Mem[base].opcode - NUM_OPCODES].name, Mem[base].operand);
				}
			}
		}
		
		//Update base memory position to go to next instruction in segment
		base++;
	}
	print_out("\n");
}

/**
	Print debugging message about Mem.

	<b> Important Notes:</b>
	\li The debugging message is only printed if the variable DEBUG_MEM is
	turned on (set to 1). This is done by adding the following line to
	config.dat: <br>
	DEBUG_MEM= ON
	\li The function is not necessary to implement as it serves only to help
	the programmer in debugging the implementation of common data structures.

	@retval None
 */
void
Dump_mem( segment_type* seg_tab )
{
	// if debug flag is turned on
	if( DEBUG_MEM )
	{
		/* print debugging message */
	}
	else // do not print any message
	{ ; }
}
