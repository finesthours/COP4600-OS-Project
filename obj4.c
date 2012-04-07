/**
	obj4.c


	Simulator Project for COP 4600

	Objective 4 project that implements running all programs and handles
	I/O requests.

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

void Dump_Event_List();
void Dump_Event_List_Recursive(event_list *Event_List, int i);

/**
	Add given process to the end of the CPU ready queue.
	(i.e., pcb is added to the end of CPU.ready_q)

	<b> Important Notes:</b>
	\li The queue is doubly and circularly linked to allow efficient access to
	both the front and the end of the list. That is, the first node's previous
	link points to the last node in the list, and the last node's next link
	points to the first node in the list. For the special case in which the
	queue has a single node, all links point to itself.

	<b> Algorithm: </b>
	\verbatim
	Create and initialize pcb node

	If ready queue is empty
		Set ready queue to new node
		All links point to itself since it is circularly linked
	Otherwise, queue is non-empty
		Place node at end of queue
	\endverbatim

	@param pcb -- PCB to add to CPU's ready queue
	@retval None
 */
void
Add_cpuq( pcb_type *pcb )
{
	//Create and initialize pcb node
	pcb_list *pcbNode = (pcb_list*)malloc(sizeof(pcb_list));
	pcbNode->pcb = pcb;
	
	//If ready queue is empty
	if(CPU.ready_q == NULL)
	{
		//Set ready queue to new node
		CPU.ready_q = pcbNode;
		//All links point to itself since it is circularly linked
		pcbNode->next = pcbNode;
		pcbNode->prev = pcbNode;
	}
	
	//Otherwise, queue is non-empty
	else
	{
		//Place node at end of queue
		pcbNode->next = CPU.ready_q;
		pcbNode->prev = CPU.ready_q->prev;
		CPU.ready_q->prev->next = pcbNode;
		CPU.ready_q->prev = pcbNode;
	}
}

/**
	Add an I/O request block to the device's waiting queue.
	(i.e., rb is added to end of Dev_Table[dev_id].wait_q)

	<b> Important Notes:</b>
	\li The queue is doubly and circularly linked to allow efficient access to
	both the front and the end of the list. That is, the first node's previous
	link points to the last node in the list, and the last node's next link
	points to the first node in the list. For the special case in which the
	queue has a single node, all links point to itself.

	<b> Algorithm: </b>
	\verbatim
	Create and initialize request block node

	If device queue is empty
		Set device queue to new node
		All links point to itself since it is circularly linked
	Otherwise, queue is non-empty
		Place node at end of queue

	Record time when rb is enqueued
	\endverbatim

	@param dev_id -- index into device table to the device that I/O is
		being requested from
	@param rb -- request block to add to device's ready queue
	@retval None
 */
void
Add_devq( int dev_id, rb_type* rb )
{
	//Create and initialize request block node
	rb_list *rbNode = (rb_list*)malloc(sizeof(rb_list));
	rbNode->rb = rb;
	
	//If device queue is empty
	if(Dev_Table[dev_id].wait_q == NULL)
	{
		//Set device queue to new node
		Dev_Table[dev_id].wait_q = rbNode;
		//All links point to itself since it is circularly linked
		rbNode->next = rbNode;
		rbNode->prev = rbNode;
	}
	
	//Otherwise, queue is non-empty
	else
	{
		//Place node at end of queue
		rbNode->next = Dev_Table[dev_id].wait_q;
		rbNode->prev = Dev_Table[dev_id].wait_q->prev;
		Dev_Table[dev_id].wait_q->prev->next = rbNode;
		Dev_Table[dev_id].wait_q->prev = rbNode;
	}
	
	//Record time when rb is enqueued
	rb->q_time = Clock;
}

/**
	Add an I/O request block to the process's request block queue.
	(i.e., rb is added to end of pcb->rb_q)

	<b> Algorithm: </b>
	\verbatim
	Create and initialize request block node

	If request block queue is empty
		Set request block queue to new node
		All links point to itself since it is circularly linked
	Otherwise, queue is non-empty
		Place node at end of queue
	\endverbatim

	<b> Important Notes:</b>
	\li The queue is doubly and circularly linked to allow efficient access to
	both the front and the end of the list. That is, the first node's previous
	link points to the last node in the list, and the last node's next link
	points to the first node in the list. For the special case in which the
	queue has a single node, all links point to itself.

	@param pcb -- pcb requesting I/O service
	@param rb -- request block to add to pcb's ready queue
	@retval None
 */
void
Add_rblist( pcb_type* pcb, rb_type* rb)
{
	//Create and initialize request block node
	rb_list *rbNode = (rb_list*)malloc(sizeof(rb_list));
	rbNode->rb = rb;
	
	//If request block queue is empty
	if(pcb->rb_q == NULL)
	{
		//Set request block queue to new node
		pcb->rb_q = rbNode;
		//All links point to itself since it is circularly linked
		rbNode->next = rbNode;
		rbNode->prev = rbNode;
	}
	
	//Otherwise, queue is non-empty
	else
	{
		//Place node at end of queue
		rbNode->next = pcb->rb_q;
		rbNode->prev = pcb->rb_q->prev;
		pcb->rb_q->prev->next = rbNode;
		pcb->rb_q->prev = rbNode;
	}
	rbNode->rb->q_time = Clock;
}

/**
	Select the next process for allocation to the CPU according to the currently
	active scheduling algorithm.

	<b> Important Notes:</b>
	\li Currently, only first-come first-serve scheduling is being used.

	<b> Algorithm (FCFS): </b>
	\verbatim
		Update the number of processes serviced by the CPU

		Get first pcb in CPU's ready queue
		Remove first node in queue

		If not removing last pcb in ready queue
			Change head of ready queue

		Calculate time process was ready
		Increment total ready time for process
		Increment total CPU queue waiting time

		Reset the burst count for the next program
	\endverbatim

	@retval pcb -- next active process
 */
pcb_type*
Scheduler( )
{
	//Update the number of processes serviced by the CPU
	CPU.num_served++;
	
	if(CPU.ready_q == NULL) return NULL;

	//Get first pcb in CPU's ready queue
	pcb_list *pcbNode = CPU.ready_q;
	pcb_type *pcb = pcbNode->pcb;
	
	//Remove first node in queue
	CPU.ready_q = CPU.ready_q->next;
	
	//If not removing last pcb in ready queue
	if(pcbNode != CPU.ready_q)
	{
		//Change head of ready queue
		CPU.ready_q->prev->prev->next = CPU.ready_q;
		CPU.ready_q->prev = CPU.ready_q->prev->prev;
		//~ pcbNode->prev->next = pcbNode->next;
		//~ pcbNode->next->prev = pcbNode->prev;
		//~ CPU.ready_q = pcbNode->next;
	}
	else
	{
        CPU.ready_q = NULL;
	}

	pcb->run_time = Clock;
	
	//Calculate time process was ready
	time_type currTime = Clock;
    Diff_time(&pcb->ready_time, &currTime);
	
	//Increment total ready time for process
	Add_time(&currTime, &pcb->total_ready_time);
	
	//Increment total CPU queue waiting time
	Add_time(&currTime, &CPU.total_q_time);

	//Reset the burst count for the next program
	pcb->sjnburst = 0;  
	
	//return next active process
	return pcb;
}

/**
	Service event to start an I/O operation.

	<b> Important Notes:</b>
	\li Events of type SIO_EVT will result in this call via 
	Interrupt_Handler() in simulator.c after interrupt() in obj1.c sets the
	global Event variable.
	\li SIO is a non-blocking I/O operation. As such, the program causing the
	event will continue running after the event is serviced.
	\li Control is returned to the program after servicing this request since
	the process is not waiting for I/O to complete.
	\li An event to end I/O may or may not have been placed in the event list
	depending on whether or not the device was busy processing some other I/O
	request at the time Start_IO() was called.

	<b> Algorithm: </b>
	\verbatim
	Allocate and initialize new I/O request block--call Alloc_rb()

	Add rb to both the device's and pcb's queues--call both Add_devq() and Add_rblist()

	Initiate I/O operation on requested device--call Start_IO()

	Return control to interrupted process--turn CPU switch on and scheduling switch off
	\endverbatim

	@retval None
 */
void
Sio_Service( )
{
	//Allocate and initialize new I/O request block--call Alloc_rb()
	rb_type* rb = Alloc_rb();
	
	//Add rb to both the device's and pcb's queues--call both Add_devq() and Add_rblist()
	Add_devq(rb->dev_id, rb);
	Add_rblist(rb->pcb, rb);
	
	//Initiate I/O operation on requested device--call Start_IO()
	Start_IO(rb->dev_id);
	
	//Return control to interrupted process--turn CPU switch on and scheduling switch off
	CPU_SW = ON;
	SCHED_SW = OFF;
}

/**
	Allocate and initialize a new I/O request block.

	<b> Important Notes:</b>
	\li Calculating the device ID (i.e., its index in Dev_Table) for the rb is
	rather difficult. The calculation is complicated as the instruction for the
	device being requested must be retrieved from its location in Mem, which
	involves a great deal of indirection. For example, we may have the
	following:
	<pre>
			SIO       32  <-- instruction requesting I/O service
			DISK     300  <-- instruction with device ID that we need
			WIO        5  <-- next instruction to be fetched for process
	</pre>
	The device instruction must be found in memory using only the current
	pcb with the pcb's saved program counter incremented passed the point
	needed.
	\li We subtract 1 from the offset when calculating the device instruction
	since we increment offset by 2 in Cpu() after executing each SIO
	instruction. Hence, offset gives the instruction AFTER the device, while
	offset - 1 gives us the device.  For example, the following shows a sample
	situation
	<pre>
			SIO       32  <-- instruction requesting I/O service
			DISK     300  <-- instruction with device we need (offset - 1)
			WIO        0  <-- current offset postion
	</pre>
	\li When calculating the request ID for the rb, one is subtracted from the
	offset since 2 was added to PC in Cpu(). This is similar to the situation
	above for calculating the device ID.
	\li The number of bytes to transfer is obtained by using the operand of the
	device instruction.

	<b> Algorithm: </b>
	\verbatim
	Allocate request block

	Initialize request block's fields:
		Mark status as pending
		Retrieve process that owns this rb using the current agent ID

		Calculate and save the device's ID for the rb:
			Let saved_pc be the program counter from the pcb's saved CPU state
			Let device_seg be the segment from the pcb's segment table of the last executed instruction (retrieved from saved_pc)
			Let device_instr be the instruction from Mem at postion device_seg.base + saved_pc.offset - 1
			Finally, set rb->dev_id be the opcode at device_instr converted to its corresponding device ID (i.e., opcode - NUM_OPCODES)

		Set number of bytes to transfer

		Save logical address (segment/offset pair) of the device instruction using process's saved PC
	\endverbatim

	@retval rb -- new I/O request block
 */
rb_type*
Alloc_rb( )
{
	//Allocate request block
	rb_type *rb = (rb_type*)malloc(sizeof(rb_type));
	
	//Initialize request block's fields:
	//Mark status as pending
	rb->status = PENDING_RB;
	
	//Retrieve process that owns this rb using the current agent ID
	rb->pcb = Term_Table[Agent-1];
	
	//Calculate and save the device's ID for the rb:
	//Let saved_pc be the program counter from the pcb's saved CPU state
	addr_type saved_pc = rb->pcb->cpu_save.pc;
	
	//Let device_seg be the segment from the pcb's segment table of the last executed instruction (retrieved from saved_pc)
	segment_type device_seg = rb->pcb->seg_table[saved_pc.segment];
	
	//Let device_instr be the instruction from Mem at postion device_seg.base + saved_pc.offset - 1
	instr_type device_instr = Mem[device_seg.base + saved_pc.offset - 1];
	
	//Finally, set rb->dev_id be the opcode at device_instr converted to its corresponding device ID (i.e., opcode - NUM_OPCODES)
	rb->dev_id = device_instr.opcode - NUM_OPCODES;

	//Set number of bytes to transfer
	rb->bytes = device_instr.operand.bytes;
	
	//Save logical address (segment/offset pair) of the device instruction using process's saved PC
	saved_pc.offset--;
	rb->req_id = saved_pc;
	
	rb->q_time = Clock;
	
	//return new I/O request block
	return rb;
}

/**
	Start processing I/O request for given device.

	<b> Important Notes:</b>
	\li Add_time() and Diff_time() should be used for calculating and
	incrementing many of the simulation time fields, such as the device's busy
	time. 
	\li The time at which I/O completes is obtained using rb->bytes and the
	speed of the device: device->bytes_per_sec and device->nano_per_byte.
	Using all these values, the seconds and nanoseconds fields of a simulation
	time structure can be filled in. Note that bytes per sec and nano per byte
	are, to some degree, reciprocals, which implies that one should divide by
	the former but multiply with the latter.
	\li When finally adding the ending I/O event to the event list, the
	device's ID must be converted into an agent ID. Recall in objective 1 that
	agent IDs for devices were set to directly follow terminal user IDs.

	<b> Algorithm: </b>
	\verbatim
	If the device is busy
		Do nothing

	If the device has no requests to service in its waiting queue
		Do nothing

	Get and remove first node from waiting list

	Mark rb as the current request block in the device; set rb's status as active

	Update time spent waiting in queue--calculate difference between current time and time it was enqueued and add this value to the current queue wait time--use Add_time() and Diff_time()

	Compute length of time that I/O will take (transfer time)--compute number of seconds and then use the remainder to calculate the number of nanoseconds

	Increment device's busy time by the transfer time--use Add_time()

	Compute time I/O ends--ending time is the current time + transfer time--use Add_time() and Diff_time()

	Update the number of IO requests served by the device

	Add ending I/O device interrupt to event list
	\endverbatim

	@param[in] dev_id -- ID of device
	@retval None
 */
void
Start_IO( int dev_id )
{
	
	//If the device is busy
    if(Dev_Table[dev_id].current_rb != NULL && Dev_Table[dev_id].current_rb->status == ACTIVE_RB)
	{
		//Do nothing
		return;
	}

	//If the device has no requests to service in its waiting queue
	if(Dev_Table[dev_id].wait_q == NULL)
	{
		//Do nothing
		return;
	}

	rb_list *rbNode = Dev_Table[dev_id].wait_q;

	//Get and remove first node from waiting list
	Dev_Table[dev_id].wait_q->prev->next = Dev_Table[dev_id].wait_q->next;
	Dev_Table[dev_id].wait_q->next->prev = Dev_Table[dev_id].wait_q->prev;
	Dev_Table[dev_id].wait_q = rbNode->next;
	
	if(rbNode == Dev_Table[dev_id].wait_q)
	{
		Dev_Table[dev_id].wait_q = NULL;
	}
	
	rbNode->next = rbNode;
	rbNode->prev = rbNode;
	
	//Mark rb as the current request block in the device; set rb's status as active
	Dev_Table[dev_id].current_rb = rbNode->rb;
	rbNode->rb->status = ACTIVE_RB;
	
	//Update time spent waiting in queue--calculate difference between current time and time it was enqueued and add this value to the current queue wait time--use Add_time() and Diff_time()
	time_type currTime = Clock;
    Diff_time(&rbNode->rb->q_time, &currTime);
    Add_time(&currTime, &Dev_Table[dev_id].total_q_time);

	//Compute length of time that I/O will take (transfer time)--compute number of seconds and then use the remainder to calculate the number of nanoseconds
	time_type transferTime;
    transferTime.seconds = rbNode->rb->bytes / Dev_Table[dev_id].bytes_per_sec;
    transferTime.nanosec = (rbNode->rb->bytes % Dev_Table[dev_id].bytes_per_sec)*Dev_Table[dev_id].nano_per_byte;
	
	//Increment device's busy time by the transfer time--use Add_time()
	Add_time(&transferTime, &Dev_Table[dev_id].total_busy_time);
	
	//Compute time I/O ends--ending time is the current time + transfer time--use Add_time() and Diff_time()
	time_type ioEndTime = transferTime;
    Add_time(&Clock, &ioEndTime);
	
	//Update the number of IO requests served by the device
	Dev_Table[dev_id].num_served++;

	//Add ending I/O device interrupt to event list
	Add_Event(EIO_EVT,(dev_id+Num_Terminals+1),&ioEndTime);
	
}

/**
	Service event to wait on an I/O operation.

	This function services requests to check an I/O operation for completion. 
	If the operation is complete, the process resumes execution immediately;
	otherwise, the process is blocked.

	<b> Important Notes:</b>
	\li Events of type WIO_EVT will result in this call via 
	Interrupt_Handler() in simulator.c after interrupt() in obj1.c sets the
	global Event variable.
	\li WIO, unlike SIO, is a blocking I/O operation. As such, the program
	causing the event will be blocked (removed from the ready queue or from the
	CPU) after this event is serviced. However, if I/O had already been
	started from an earlier SIO event, then the process may be able to
	continue, but only if the I/O request has already been completed.
	\li Retrieving the request instruction (REQ) from memory is done similarly
	to finding the device ID in Alloc_rb(). The saved CPU information is used
	to find the necessary segment in the PCB's segment table. Once the segment
	has been found, the segment base is added to the current PC offset (minus
	1) to get the position in memory that the request instruction is located
	at.
	\li The request instruction's operand, which is an address, should be
	passed to Find_rb() to find the needed request block as the request address
	holds information pointing to the device being waited on.

	<b> Algorithm: </b>
	\verbatim
	Retrieve pcb from the terminal table that is waiting for I/O

	Retrieve request instruction using saved CPU information

	Locate request block that process wants to wait for--call Find_rb()

	Determine status of request block
		If rb is still active or pending
			Block process since I/O not finished:
				Mark PCB as blocked; mark it 

				Turn CPU and scheduling switches on to schedule a new process

				Calculate active time for process and busy time for CPU

				Record time process was blocked

				Print output message giving blocked procese and burst count

		If rb has finished
			Delete it and keep running current process:
				Delete rb from pcb's list

				Turn CPU switch on to execute a process
				Turn scheduling flag off to use current process (i.e., not schedule a new process)

	\endverbatim
 */
void
Wio_Service( )
{
	//Retrieve pcb from the terminal table that is waiting for I/O
	//~ pcb_type *pcb = Term_Table[Agent-1];
	pcb_type *pcb = Term_Table[Agent - 1]->rb_q->rb->pcb;
	
	//Retrieve request instruction using saved CPU information
    //~ instr_type instr = Mem[pcb->seg_table[pcb->cpu_save.pc.segment].base + pcb->cpu_save.pc.offset-1];
    instr_type instr = Mem[pcb->seg_table[pcb->cpu_save.pc.segment].base + CPU.state.pc.offset-1];
	
	//Locate request block that process wants to wait for--call Find_rb()
	rb_type *rb = Find_rb(pcb, &instr.operand.address);

	//Determine status of request block
	//If rb is still active or pending
	if(rb->status == ACTIVE_RB || rb->status == PENDING_RB)
    {
		//Block process since I/O not finished:
		//Mark PCB as blocked; mark it 
		pcb->status == BLOCKED_PCB;

		//Turn CPU and scheduling switches on to schedule a new process
		CPU_SW = ON;
		SCHED_SW = ON;
        
		//Calculate active time for process and busy time for CPU
		time_type currTime = Clock;
        Diff_time(&pcb->run_time, &currTime);
        Add_time(&currTime, &(pcb->total_run_time));
        Add_time(&currTime, &(CPU.total_busy_time));

		//Record time process was blocked
		pcb->block_time = Clock;
		
		pcb->wait_rb = rb;

		//Print output message giving blocked procese and burst count
		print_out("\t\tUser %s is blocked for I/O.\n", pcb->username);
        print_out("\t\tCPU burst was %d instructions.\n\n",pcb->sjnburst);
	}
	
	//If rb has finished
	if(rb->status == DONE_RB)
    {
		//Delete it and keep running current process:
		//Delete rb from pcb's list
		Delete_rb(rb, pcb);
		
		//Turn CPU switch on to execute a process
		CPU_SW = ON; 
		
		//Turn scheduling flag off to use current process (i.e., not schedule a new process)
		SCHED_SW = OFF;
	}
}

/**
	Search the PCB's request block list to find the rb with the matching
	request ID.

	<b> Algorithm: </b>
	\verbatim
	Traverse the pcb's request block list 
		If the rb's request address matches the given requested address
			The needed rb has been found
	\endverbatim

  @param[in] pcb -- process whose request blocks are being searched
  @param[in] req_id -- address of request block searching for
  @retval rb -- request block with address matching req_id 
 */
rb_type*
Find_rb( pcb_type* pcb, struct addr_type* req_id )
{
	rb_list *rbNode = pcb->rb_q;

	//Traverse the pcb's request block list 
	do
	{
		//If the rb's request address matches the given requested address
		if(rbNode->rb->req_id.segment == req_id->segment && rbNode->rb->req_id.offset == req_id->offset)
		{
			//The needed rb has been found
			return rbNode->rb;
		}
		rbNode = rbNode->next;
	}while(rbNode != pcb->rb_q);
	err_quit("no rb found.\n");
}

/**
  Delete given request block from process's list of request blocks. The
  request block itself is also deallocated if found.

	<b> Algorithm: </b>
	\verbatim
	Traverse the pcb's request block list 
		If the rb has been found
			If deleting last remaining node
				Delete list
			Otherwise, remove node from middle of list
				By-pass node in list

				If deleting front of queue
					Change head of list
			Deallocate both the list node and the request block
	\endverbatim

  @param[in] rb -- request block to remove from process's list
  @param[in] pcb -- process from which to remove request block
 */
void
Delete_rb( rb_type* rb, pcb_type* pcb )
{
	rb_list *rbNode = pcb->rb_q;

	//Traverse the pcb's request block list 
	while(rbNode)
	{
		//If the rb has been found
		if(rbNode->rb->req_id.segment == rb->req_id.segment && rbNode->rb->req_id.offset == rb->req_id.offset)
		{
			//If deleting front of queue
			if(rbNode == pcb->rb_q)
			{
				//Change head of list
				//~ pcb->rb_q->prev->next = pcb->rb_q->next;
				pcb->rb_q = pcb->rb_q->next;
			}
			//If deleting last remaining node
			else if(rbNode == rbNode->next)
			{
				//Delete list
				free(rbNode->rb);
                free(rbNode);
				pcb->rb_q == NULL;
				return;
			}
			//Otherwise, remove node from middle of list
			else
			{
				//By-pass node in list
				rbNode->prev->next = rbNode->next;
				rbNode->next->prev = rbNode->prev;
			}
			//Deallocate both the list node and the request block
			free(rbNode->rb);
			free(rbNode);
			return;
		}
		rbNode = rbNode->next;
	}
}

/**
	Service an I/O interrupt event from a device.

	<b> Important Notes:</b>
	\li Events of type EIO_EVT will result in this call via 
	Interrupt_Handler() in simulator.c after interrupt() in obj1.c sets the
	global Event variable.

	<b> Algorithm: </b>
	\verbatim
	Retrieve device that caused the I/O interrupt from the device table
	Retrieve request block issued to device that is now complete
	Retrieve process that issued the request block to the device

	Mark that device no longer has a current request block
	Mark rb's status as done

	Service next I/O request block for this device--call Start_IO()

	Determine current status of process
		If process is ready to be ran or is already running
			Turn off both switches

		If process is blocked due to I/O
			If process was waiting on this rb 
				The process can now run:
					Delete rb from pcb's waiting list--call Delete_rb()
					Mark pcb as ready to run
					Add pcb to CPU's ready queue--call Add_cpuq()

			Record time process became ready
			Calculate and record time process was blocked

			If CPU is has no currently active process
				Turn on both switches to schedule a new process to run
			Otherwise, CPU is already busy, so
				Turn off both switches

		If the process is done, but only waiting for I/O to complete.

			Delete rb from pcb's waiting list--call Delete_rb()

			Load next program for pcb--call Next_pgm()
				If one was available
					Mark pcb's status as ready to run

			Record time process became ready
			Calculate and record time process was blocked

			If CPU is idle
				Turn on CPU and scheduling switches to run a new process
			Otherwise, CPU is already busy, so
				Turn off both switches

	\endverbatim
}
 */
void
Eio_Service( )
{
	//Retrieve device that caused the I/O interrupt from the device table
	device_type *device = &(Dev_Table[Agent-Num_Terminals-1]);
	
	//Retrieve request block issued to device that is now complete
	rb_type *currRB = device->current_rb;
	
	//Retrieve process that issued the request block to the device
	pcb_type *currPCB = currRB->pcb;
	
	//Mark that device no longer has a current request block
	device->current_rb = NULL;
	
	//Mark rb's status as done
	currRB->status = DONE_RB;
	
	//Service next I/O request block for this device--call Start_IO()
	Start_IO(Agent - Num_Terminals - 1);
	
	//Determine current status of process
	//If process is ready to be ran or is already running
	if(currPCB->status == READY_PCB || currPCB->status == ACTIVE_PCB)
    {
		//Turn off both switches
		CPU_SW = OFF;
        SCHED_SW = OFF;
    }
		
	//If process is blocked due to I/O
	else if(currPCB->status == BLOCKED_PCB)
    {
		//If process was waiting on this rb 
		if(currPCB->wait_rb == currRB)
        {
			//The process can now run:
			//Delete rb from pcb's waiting list--call Delete_rb()
			Delete_rb(currRB, currPCB);
			
			//Mark pcb as ready to run
			currPCB->status = READY_PCB;
			
			currPCB->wait_rb = NULL;
			
			//Add pcb to CPU's ready queue--call Add_cpuq()
			Add_cpuq(currPCB);
		}

		//Record time process became ready
		currPCB->ready_time = Clock;
		
		//Calculate and record time process was blocked
		time_type currTime = Clock;
        Diff_time(&(currPCB->block_time), &currTime);
        Add_time(&currTime, &(currPCB->total_block_time));

		//If CPU is has no currently active process
		if(CPU.active_pcb == NULL)
		{
			//Turn on both switches to schedule a new process to run
			CPU_SW = ON;
			SCHED_SW = ON;
        }
        //Otherwise, CPU is already busy, so
        else
        {
			//Turn off both switches
			CPU_SW = OFF;
			SCHED_SW = OFF;
		}
	}
	
	//If the process is done, but only waiting for I/O to complete.
	if(currPCB->status == DONE_PCB)
    {
		//Delete rb from pcb's waiting list--call Delete_rb()
		Delete_rb(currRB,currPCB);

		//Load next program for pcb--call Next_pgm()
		//If one was available
		if(Next_pgm(currPCB) != 0)
        {
			//Mark pcb's status as ready to run
            currPCB->status == READY_PCB;
        }

		//Record time process became ready
		currPCB->ready_time = Clock;
		
		//Calculate and record time process was blocked
		time_type currTime = Clock;
        Diff_time(&(currPCB->block_time), &currTime);
        Add_time(&currTime, &(currPCB->total_block_time));
		
		//If CPU is idle
		if(CPU.active_pcb == NULL)
		{
			//Turn on CPU and scheduling switches to run a new process
			CPU_SW = ON;
			SCHED_SW = ON;
		}
		//Otherwise, CPU is already busy, so
		else
        {
			//Turn off both switches
			CPU_SW = OFF;
			SCHED_SW = OFF;
		}
	}
}

/**
	Remove all I/O request blocks from the PCB's list that have completed.

	<b> Important Notes:</b>
	\li Request blocks that have been processed will have the status DONE_RB.
	\li The request block list is circularly linked.

	<b> Algorithm: </b>
	\verbatim
	If list is not empty
		Traverse pcb's list of request blocks removing all finished rbs:
			Start at second node in the list and end when wraps around to first
				Save previous node and current node

				If rb is done
					Remove node from middle of list

		Handle special removal cases:
			If first rb in list is done
				If deleting last remaining node
					Delete list
				Remove node from front of list
				Change head of list
	\endverbatim

  @param pcb -- process from which to remove completed I/O request blocks
 */
void
Purge_rb( pcb_type* pcb )
{
	rb_list *nextNode, *prevNode, *iterNode;
	//If list is not empty
	if(pcb->rb_q)
	{
		//Traverse pcb's list of request blocks removing all finished rbs:
		//Start at second node in the list and end when wraps around to first
		iterNode = pcb->rb_q->next;	
		while(iterNode != pcb->rb_q)
		{
			//Save previous node and current node
			prevNode = iterNode->prev;
			nextNode = iterNode->next;
			
			//If rb is done
			if(iterNode->rb->status == DONE_RB)
			{
				//Remove node from middle of list
				prevNode->next = nextNode;
                nextNode->prev = prevNode;
                free(iterNode->rb);
                free(iterNode);
			}
			iterNode = iterNode->next;
		}
		
		//Handle special removal cases:
		//If first rb in list is done
		if(pcb->rb_q->rb->status == DONE_RB)
		{
			//If deleting last remaining node
			if(pcb->rb_q->next == pcb->rb_q)
			{
				//Delete list
				//~ free(pcb->rb_q->rb);
                //~ free(pcb->rb_q);
                pcb->rb_q = NULL;
			}
			else
			{
				//Remove node from front of list
				iterNode = pcb->rb_q;
                iterNode->prev->next = iterNode->next;
                iterNode->next->prev = iterNode->prev;
				
				//Change head of list
				pcb->rb_q = pcb->rb_q->next;
				
				free(iterNode->rb);
                free(iterNode);
			}
		}
	}
}

/**
	Prepare the scheduled program for execution and transfer control to it.

	<b> Important Notes:</b>
	\li As a result of this function, the next active program, CPU.active_pcb,
	is loaded into memory and executed. The program will run until either it
	issues an end event to exit the program or an I/O instruction is issued.

	<b> Algorithm: </b>
	\verbatim
	Initialize and load the lower half of the kernel segment table with the segment table of the current user (located at CPU.active_pcb)--call Load_Map()

	Execute the program associated with the saved CPU state of the CPU's active process--call Exec_Program()
	\endverbatim

	@retval None
 */
void
Dispatcher( )
{
	//Initialize and load the lower half of the kernel segment table with the segment table of the current user (located at CPU.active_pcb)--call Load_Map()
	Load_Map(CPU.active_pcb->seg_table, CPU.active_pcb->num_segments);

	//Execute the program associated with the saved CPU state of the CPU's active process--call Exec_Program()
	Exec_Program(&CPU.active_pcb->cpu_save);
} 

/**
	Load the dispatched program's segment table into the kernel's segment table.

	<b> Important Notes:</b>
	\li Recall that the kernel's segment table is represented by the global
	variable Mem_Map.
	\li The first half of Mem_Map is used for user programs (the second half is
	reserved for kernel processes, such as the boot program).

	<b> Algorithm: </b>
	\verbatim
	For each user segment in the kernel's segment table
		Reset segment's access mode to kernel mode so that any attempted access will result in segmentation fault (in simulated program)
	For each segment in user's program
		Copy segment into user segment in kernel's segment table
	\endverbatim

	@retval None
 */
void
Load_Map( segment_type *seg_tab, int num_segments )
{
	int currSeg;
	//For each user segment in the kernel's segment table
	for(currSeg = 0; currSeg < Max_Segments; currSeg++)
	{
		//Reset segment's access mode to kernel mode so that any attempted access will result in segmentation fault (in simulated program)
		Mem_Map[currSeg].access = 0;
	}
	//For each segment in user's program
	for(currSeg = 0; currSeg < num_segments; currSeg++)
	{
		//Copy segment into user segment in kernel's segment table
		Mem_Map[currSeg] = seg_tab[currSeg];
	}
}

/**
	Print debugging message about pcb.rb_q.

	<b> Important Notes:</b>
	\li The debugging message is only printed if the variable DEBUG_RBLIST is
	turned on (set to 1). This is done by adding the following line to
	config.dat: <br>
	DEBUG_RBLIST= ON
	\li The function is not necessary to implement as it serves only to help
	the programmer in debugging the implementation of common data structures.

	@retval None
 */
void
Dump_rblist( )
{
	// if debug flag is turned on
	if( DEBUG_RBLIST )
	{
		/* print debugging message */
	}
	else // do not print any message
	{ ; }
}

/**
	Print debugging message about device.wait_q.

	<b> Important Notes:</b>
	\li The debugging message is only printed if the variable DEBUG_DEVQ is
	turned on (set to 1). This is done by adding the following line to
	config.dat: <br>
	DEBUG_DEVQ= ON
	\li The function is not necessary to implement as it serves only to help
	the programmer in debugging the implementation of common data structures.

	@retval None
 */
void
Dump_devq( )
{
	// if debug flag is turned on
	if( DEBUG_DEVQ )
	{
		/* print debugging message */
	}
	else // do not print any message
	{ ; }
}

/**
	Print debugging message about CPU.ready_q.

	<b> Important Notes:</b>
	\li The debugging message is only printed if the variable DEBUG_CPUQ is
	turned on (set to 1). This is done by adding the following line to
	config.dat: <br>
	DEBUG_CPUQ= ON
	\li The function is not necessary to implement as it serves only to help
	the programmer in debugging the implementation of common data structures.

	@retval None
 */
void
Dump_cpuq( )
{
	// if debug flag is turned on
	if( DEBUG_CPUQ )
	{
		/* print debugging message */
	}
	else // do not print any message
	{ ; }
}

void
Dump_Event_List()
{
  puts("");
  Dump_Event_List_Recursive(Event_List, 1);
  puts("");
}

void
Dump_Event_List_Recursive(event_list *Event_List, int i )
{
  if(Event_List == NULL)
    return;

  printf("[%d: %s %d, %d %d] ", i, Event_Names[Event_List->event], Event_List->agent, Event_List->time.seconds, Event_List->time.nanosec);
  Dump_Event_List_Recursive(Event_List->next, i+1);
}
