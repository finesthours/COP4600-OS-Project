/**
	obj1.c


	Simulator Project for COP 4600

	Objective 1 project that implements loading the event list for
	the OS simulator.

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

int  get_agent_id( char* );
int  get_event_id( char* );

/**
	Initialize the event list from the file logon.dat. This file normally
	contains only logon events for all terminals. However, for debugging
	purposes, logon.dat can contain events of any type.

	<b> Algorithm: </b>
	\verbatim
	Open logon file given by constant LOGON_FILENAME
	While not end of file
		Read event name, agent name, and time from each line in file
		Convert event name to event ID--call get_event_id() (must be written)
		Convert agent name to agent ID--call get_agent_id() (must be written)
		Convert time to simulation time--call Uint_to_time()
		Add event to event list using event ID, agent ID, and simulation time
	Close file
	\endverbatim

	@retval None
 */
void
Load_Events( )
{
	FILE *fp;
	time_type sim_time = {0,0};

	int eventId, agentId;
	char str[128], *eventListToken, *eventName, *agentName, *timeInString;
	long long int timeIn;

	//Open logon file given by constant LOGON_FILENAME
	if((fp = fopen(LOGON_FILENAME, "r")) == NULL)
	{
		printf("Cannot open file.\n");
		exit(1);
	}

	//While not end of file
	while (fgets(str, sizeof(str), fp) != NULL)
	{
		//Read event name, agent name, and time from each line in file
		eventListToken = strtok(str, " ");
		eventName = eventListToken;
		eventId = get_event_id( eventName );
		
		//convert agent name to upper case per OSSProject-1.ppt s17
		eventListToken = strtok('\0', ", ");
		agentName = eventListToken;
		agentId = get_agent_id( agentName );
		
		//Convert time to simulation time--call Uint_to_time()
		eventListToken = strtok('\0', ", ");
		timeInString = eventListToken;
		timeIn = atoll(timeInString);
		Uint_to_time(timeIn, &sim_time);
		
		//Add event to event list using event ID, agent ID, and simulation time
		Add_Event(eventId, agentId, &sim_time);
	}
	//~ while(Event_List)
	//~ {
		//~ printf("%d, %d, %lu, %lu\n", Event_List->event, Event_List->agent,  Event_List->time.seconds, Event_List->time.nanosec);
		//~ Event_List = Event_List->next;
	//~ }
	fclose(fp);
}

/**
	Add a new event into the list of operating system events.

	This function inserts a future event into the list Event_List at the proper
	time sequence. Event_List points to the event in the list having the
	smallest time (as defined by the function Compare_time).

	<b> Algorithm: </b>
	\verbatim
	Allocate a new event node
	Set the new event node's fields to the event, agent, and time passed in
	If the event list is empty
		Set the event list header node to the new event node
	Else, if the new event node should precede the event list header node
		Place the new node at the start of the list
	Otherwise, the new node goes in the middle or at the end of the list
		Traverse the event list until reaching the node that should precede the new node
		Add the new node after the node reached in the traversal--handle special case of new node being at the end of the list
	\endverbatim

	@param[in] event -- event to simulate
	@param[in] agent -- user agent causing event
	@param[in] time -- time event occurs
	@retval None
 */
void
Add_Event( int event, int agent, struct time_type* time )
{
	//Allocate a new event node
	event_list *newNode, *currNode, *old;
	newNode = (event_list *) malloc(sizeof(event_list));
	
	if(!newNode)
	{
		printf("\nERROR: no more memory");
		return;
	}
	
	//Set the new event node's fields to the event, agent, and time passed in
	newNode->event = event;
	newNode->agent = agent;
	newNode->time = *time;
	
	//If the event list is empty
	if(Event_List == NULL)
	{
		//Set the event list header node to the new event node
		newNode->next = NULL;
		newNode->prev = NULL;
		Event_List = newNode;
		return;
	}

	currNode = Event_List;
	old = NULL;
	
	while(currNode)
	{
		if(Compare_time(&newNode->time, &currNode->time) >= 0)
		{
			old = currNode;
			currNode = currNode->next;
		}
		else
		{
			if(currNode->prev)
			{
				currNode->prev->next = newNode;
				newNode->next = currNode;
				newNode->prev = currNode->prev;
				currNode->prev = newNode;
				return;
			}
			Event_List = newNode;
			Event_List->next = currNode;
			Event_List->prev = NULL;
			currNode->prev = Event_List;
			return;
		}
	}
	old->next = newNode;
	newNode->next = NULL;
	newNode->prev = old;
}

/**
	Generate clock interrupt.

	<b> Algorithm: </b>
	\verbatim
	Remove an event from Event_List
	Set Clock, Agent and Event to the respective fields in the removed event
	Write the event to the output file--call write_event()
	Deallocate the removed event item
	Save CPU.mode and CPU.pc into Old_State.
	Change New_State to CPU.mode and CPU.pc
	\endverbatim 

	@retval None
 */
void
Interrupt( )
{
	
	//Remove an event from Event_List
	event_list *node;
	int event, agent;
	time_type time;
	
	node = Event_List;
	Event_List = node->next;
	if(Event_List)
		Event_List->prev = NULL;
		
	//Set Clock, Agent and Event to the respective fields in the removed event
	time = node->time;
	agent = node->agent;
	event = node->event;
	
	//Write the event to the output file--call write_event()
	Write_Event( event, agent, &time );
	
	//Deallocate the removed event item
	free(node);
	
	//Save CPU.mode and CPU.pc into Old_State.
	Old_State = CPU.state;
	
	//Change New_State to CPU.mode and CPU.pc
	CPU.state = New_State;
	//~ printf("\n\nINTERRUPT!!!\n\n");
}

/**
	Write an event to the output file.
	
	Call print_out() for all output to file. The format of the output is:
	"  EVENT  AGENT  TIME (HR:xxxxxxxx MN:xx SC:xx MS:xxx mS:xxx NS:xxx )"

	<b> Algorithm: </b>
	\verbatim
	Convert the seconds field of time_type to hours, minutes, and seconds
	Convert the nanoseconds field to milliseconds, microseconds and nanoseconds
	Determine type of agent--user terminal or device:
		If agent ID <= Num_Terminals, then agent is user terminal:
			Agent name is of the form 'U0#' where # = agent ID
		Otherwise, agent is a device:
			Agent name is stored in Dev_Table[ agent - Num_Terminals - 1]
	Print formatted message using event name from Event_Names, agent name, and event times--use print_out()
	\endverbatim 

	@param[in] event -- event
	@param[in] agent -- agent
	@param[in] time -- time
	@retval None
 */
void
Write_Event( int event, int agent, struct time_type *time )
{
	unsigned long seconds = time->seconds, minutes = 0, hours = 0;
	unsigned long milliseconds = 0, microseconds = 0, nanoseconds = time->nanosec;
	char agentId[3];
	char agentName[5] = "U0";
	//~ char *agentName = "U005";
	char *agentDev;
	
	//Convert the seconds field of time_type to hours, minutes, and seconds
	hours = seconds/3600;
	seconds = seconds - hours*3600;
	minutes = seconds/60;
	seconds = seconds - minutes*60;
	
	//Convert the nanoseconds field to milliseconds, microseconds and nanoseconds
	milliseconds = nanoseconds/1000000;
	nanoseconds = nanoseconds - milliseconds*1000000;
	microseconds = nanoseconds/1000;
	nanoseconds = nanoseconds - microseconds*1000;
	
	//Determine type of agent--user terminal or device:
	//If agent ID <= Num_Terminals, then agent is user terminal:
	if(agent <= Num_Terminals)
	{
		//Agent name is of the form 'U0#' where # = agent ID
		//~ if(agent < 10)
			//~ sprintf(agentId, "00%d", agent);
		//~ else if (agent > 10 && agent < 100)
			sprintf(agentId, "0%d", agent);
		//~ else
			//~ sprintf(agentId, "%d", agent);
			
		strcat(agentName,agentId);
		agentName[strlen(agentName)] = '\0';
		//Print formatted message using event name from Event_Names, agent name, and event times--use print_out()
		print_out("  %s  %s  HR:%lu MN:%lu SC:%lu MS:%lu mS:%lu NS:%lu\n", Event_Names[event], agentName, hours, minutes, seconds, milliseconds, microseconds, nanoseconds);
	}
	
	//Otherwise, agent is a device:
	else
	{
		//Agent name is stored in Dev_Table[ agent - Num_Terminals - 1]
		agentDev = Dev_Table[ agent - Num_Terminals - 1].name;
		
		//Print formatted message using event name from Event_Names, agent name, and event times--use print_out()
		print_out("  %s  %s  HR:%lu MN:%lu SC:%lu MS:%lu mS:%lu NS:%lu\n", Event_Names[event], agentDev, hours, minutes, seconds, milliseconds, microseconds, nanoseconds);
	}
}

/**
	Convert event name into event ID number.

	<b> Algorithm: </b>
	\verbatim
	Capitalize event name so that case does not matter
	Verify that name's length is shorter than constant the EVENT_NAME_LENGTH_MAX
	For event ID = 0 to NUM_EVENTS
		Compare event name to Event_Names[ event ID ]
		If they are equal
			Return event ID
		Otherwise,
			Continue in loop
	Report error if no event matched
	\endverbatim

	@param[in] event_name -- name of event
	@retval event -- event ID corresponding to event_name
 */
int
get_event_id( char* event_name )
{
	int eventId = -1, eventNum;
	char *p;
	//Capitalize event name so that case does not matter
	for (p = event_name; *p != '\0'; ++p)
	{
		*p = toupper(*p);
	}
	
	//Verify that name's length is shorter than constant the EVENT_NAME_LENGTH_MAX
	if(strlen(event_name) <= EVENT_NAME_LENGTH_MAX)
	{
		//For event ID = 0 to NUM_EVENTS
		for(eventNum = 0; eventNum < NUM_EVENTS; eventNum++)
		{
			//Compare event name to Event_Names[ event ID ]
			//If they are equal
			if(strcmp(event_name, Event_Names[eventNum]) == 0)
			{
				eventId = eventNum;
				break;
			}
		}
	}
	
	// return eventId
	return( eventId );
}

/**
	Convert agent name into agent ID number.

	<b> Algorithm: </b>
	\verbatim
	Capitalize agent name so that case does not matter
	Verify that the name's length is shorter than the constant DEV_NAME_LENGTH
	If name starts with 'U', then agent is a user terminal:
		Convert the name (except the initial 'U') into an integer--the agent ID
		Return the agent ID
	Otherwise, agent is a device:
		For agent ID = 0 to Num_Devices
			Compare agent name to the name at Dev_Table[ agent ID ]
			If they are equal
				Return agent ID + Num_Terminals + 1 since device agent IDs follow user agent IDs
			Otherwise,
				Continue in loop
		Report error if no agent name matched
	\endverbatim

	@param[in] agent_name -- name of agent
	@retval agent_id -- agent ID corresponding to agent_name
 */
int
get_agent_id( char* agent_name )
{
	int agentId = -1, agentNum;
	char *p;
	//Capitalize agent name so that case does not matter
	for (p = agent_name; *p != '\0'; ++p)
	{
		*p = toupper(*p);
	}
	
	//Verify that the name's length is shorter than the constant DEV_NAME_LENGTH
	if(strlen(agent_name) <= DEV_NAME_LENGTH)
	{
		//If name starts with 'U', then agent is a user terminal:
		if(agent_name[0] == 'U')
		{
			//Convert the name (except the initial 'U') into an integer--the agent ID
			agentId = atoi(strchr(agent_name, agent_name[1]));
		}
		
		//Otherwise, agent is a device:
		else
		{
			//For agent ID = 0 to Num_Devices
			for(agentNum = 0; agentNum < Num_Devices; agentNum++)
			{
				//Compare agent name to the name at Dev_Table[ agent ID ]
				//If they are equal
				if(strcmp(agent_name, Dev_Table[agentNum].name) == 0)
				{
					//Return agent ID + Num_Terminals + 1 since device agent IDs follow user agent IDs
					agentId = agentNum + Num_Terminals +1;
					break;
				}
			}
		}
	}
	
	// return agentId
	return( agentId );
}

/**
	Print debugging message about Event_List.

	<b> Important Notes:</b>
	\li The debugging message is only printed if the variable DEBUG_EVT is
	turned on (set to 1). This is done by adding the following line to
	config.dat: <br>
	DEBUG_EVT= ON
	\li The function is not necessary to implement as it serves only to help
	the programmer in debugging the implementation of common data structures.

	@retval None
 */
void
Dump_evt( )
{
	// if debug flag is turned on
	if( DEBUG_EVT )
	{
		/* print debugging message */
	}
	else // do not print any message
	{ ; }
}
