#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "osdefs.h"
#include "externs.h"

#define LOGON_FILENAME      "logon.dat"

int  get_agent_id( char* );
int  get_event_id( char* );
void Add_Event( int event, int agent, struct time_type* time );
void Uint_to_time( unsigned long ul_time, struct time_type* sim_time );
int  Compare_time( struct time_type *time_1, struct time_type *time_2 );
event_list*   Event_List = NULL;  // pointer to head of event list
int  Num_Terminals    =  6;   // Number of terminals

// Names of events
char* Event_Names[] =
	{ "LOGON", "SIO", "WIO", "EIO", "END", "TIMER", "SEGFAULT", "ADRFAULT" };
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
int main(void)
{
	FILE *fp;
	time_type sim_time = {0,0};
	//~ event_list*   Event_List = NULL;  // pointer to head of event list
	int c=0, timeIn, eventId, agentId;
	char str[128], *eventListToken, *eventName, *agentName, *timeInString;

	//~ printf("%d, %d", Event_List->next->agent, Event_List->prev->agent);

	//Open logon file given by constant LOGON_FILENAME
	if((fp = fopen(LOGON_FILENAME, "r")) == NULL)
	{
		printf("Cannot open file.\n");
		exit(1);
	}

	//While not end of file
	while (fgets(str, sizeof(str), fp) != NULL)
	{
		//~ printf ("Line %d: %s", c, str);
		//~ c++;
		//Read event name, agent name, and time from each line in file
		eventListToken = strtok(str, " ");
		eventName = eventListToken;
		
		//convert event name to upper case per OSSProject-1.ppt s17
		//~ for (p = eventName; *p != '\0'; ++p)
		//~ {
			//~ *p = toupper(*p);
		//~ }
		eventId = get_event_id( eventName );
		
		//convert agent name to upper case per OSSProject-1.ppt s17
		eventListToken = strtok('\0', ", ");
		agentName = eventListToken;
		//~ for (p = agentName; *p != '\0'; ++p)
		//~ {
			//~ *p = toupper(*p);
		//~ }
		agentId = get_agent_id( agentName );
		
		//Convert time to simulation time--call Uint_to_time()
		eventListToken = strtok('\0', ", ");
		timeInString = eventListToken;
		timeIn = atoi(timeInString);
		Uint_to_time(timeIn, &sim_time);
		
		
		//~ printf ("Line %d tokens: %s, %s, %d \t ", c, eventName, agentName, timeIn);
		//~ printf ("ids: %d, %d\n", eventId, agentId);
		//~ printf ("Line %d time: %lu, %lu\n", c, sim_time.seconds, sim_time.nanosec);
		c++; 
		
		//Add event to event list using event ID, agent ID, and simulation time
		Add_Event(eventId, agentId, &sim_time);
	}
	
	//~ printf("Event List:\n");
	while(Event_List)
	{
		printf("%d, %d, %lu, %lu\n", Event_List->event, Event_List->agent,  Event_List->time.seconds, Event_List->time.nanosec);
		Event_List = Event_List->next;
	}
	fclose(fp);
	return 0;
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
	
	//printf("eventName = %s ", event_name);
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
			//~ printf("\n agent_name = %s\n", strchr(agent_name, agent_name[1]));
			agentId = atoi(strchr(agent_name, agent_name[1]));
		}
		
		//Otherwise, agent is a device:
		else
		{
			//For agent ID = 0 to Num_Devices
			for(agentNum = 0; agentNum < 5/*Num_Devices == 0!!!*/; agentNum++)
			{
				//Compare agent name to the name at Dev_Table[ agent ID ]
				//If they are equal
				if(strcmp(agent_name, "DISK"/*Dev_Table[agentNum] == NULL!!!*/) == 0)
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
		if(Compare_time(&newNode->time, &currNode->time) > 0)
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
			newNode->next = currNode;
			newNode->prev = NULL;
			currNode->prev = newNode;
			return;
		}
	}
	old->next = newNode;
	newNode->next = NULL;
	newNode->prev = old;
	//~ //Else, if the new event node should precede the event list header node
	//~ if(Compare_time(&newNode->time, &currNode->time) < 0)
	//~ {
		//~ //Place the new node at the start of the list
		//~ Event_List = newNode;
		//~ Event_List->next = currNode;
		//~ Event_List->prev = NULL;
		//~ currNode->prev = Event_List;
		//~ return;
	//~ }
	//~ 
	//~ //Otherwise, the new node goes in the middle or at the end of the list
	//~ else
	//~ {
		//~ //Traverse the event list 
		//~ while(currNode->next != NULL)
		//~ {
			//~ //until reaching the node that should precede the new node
			//~ if(Compare_time(&newNode->time, &currNode->time) < 0)
			//~ {
				//~ //Add the new node after the node reached in the traversal
				//~ newNode->prev = currNode->prev;
				//~ newNode->next = currNode;
				//~ currNode->prev->next = newNode;
				//~ currNode->prev = newNode;
//~ 
				//~ return;
			//~ }
			//~ currNode = currNode->next;
		//~ }
		//~ 
		//~ //--handle special case of new node being at the end of the list
		//~ if(currNode->next == NULL)
		//~ {
			//~ currNode->next = newNode;
			//~ newNode->next = NULL;
			//~ newNode->prev = currNode;
		//~ }
	//~ }
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
	char agentName[5] = "U";
	char *agentDev;
	
	//Convert the seconds field of time_type to hours, minutes, and seconds
	hours = seconds/3600;
	seconds = seconds - hours*3600;
	minutes = seconds/60;
	seconds = seconds - minutes*60;
	
	//~ printf("hours = %lu\nminutes = %lu\nseconds = %lu\n", hours, minutes, seconds);
	
	//Convert the nanoseconds field to milliseconds, microseconds and nanoseconds
	milliseconds = nanoseconds/1000000;
	nanoseconds = nanoseconds - milliseconds*1000000;
	microseconds = nanoseconds/1000;
	nanoseconds = nanoseconds - microseconds*1000;
	
	//~ printf("milliseconds = %lu\nmicroseconds = %lu\nnanoseconds = %lu\n", milliseconds, microseconds, nanoseconds);
	
	//Determine type of agent--user terminal or device:
	//If agent ID <= Num_Terminals, then agent is user terminal:
	if(agent <= Num_Terminals)
	{
		//Agent name is of the form 'U0#' where # = agent ID
		if(agent < 10)
			sprintf(agentId, "00%d", agent);
		else if (agent > 10 && agent < 100)
			sprintf(agentId, "0%d", agent);
		else
			sprintf(agentId, "%d", agent);
			
		strcat(agentName,agentId);
		
		//Print formatted message using event name from Event_Names, agent name, and event times--use print_out()
		//~ print_out("  %s  %s  HR:%lu MN:%lu SC:%lu MS:%lu mS:%lu NS:%lu\n", Event_Names[event], agentName, hours, minutes, seconds, milliseconds, microseconds, nanoseconds);
		printf("  %s  %s  HR:%lu MN:%lu SC:%lu MS:%lu mS:%lu NS:%lu\n", Event_Names[event], agentName, hours, minutes, seconds, milliseconds, microseconds, nanoseconds);
	}
	
	//Otherwise, agent is a device:
	else
	{
		//Agent name is stored in Dev_Table[ agent - Num_Terminals - 1]
		agentDev = "DISK";//Dev_Table[ agent - Num_Terminals - 1] == NULL !!!!!
		
		//Print formatted message using event name from Event_Names, agent name, and event times--use print_out()
		//~ print_out("  %s  %s  HR:%lu MN:%lu SC:%lu MS:%lu mS:%lu NS:%lu\n", Event_Names[event], agentDev, hours, minutes, seconds, milliseconds, microseconds, nanoseconds);
		printf("  %s  %s  HR:%lu MN:%lu SC:%lu MS:%lu mS:%lu NS:%lu\n", Event_Names[event], agentDev, hours, minutes, seconds, milliseconds, microseconds, nanoseconds);
	}
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
	
}

void
Uint_to_time( unsigned long ul_time, struct time_type* sim_time )
{
	//~ switch( Time_Unit )
	//~ {
		//~ case MIN:
			//~ sim_time->seconds = ul_time * 60;
			//~ sim_time->nanosec = 0;
			//~ break;
		//~ case SEC:
			sim_time->seconds = ul_time;
			sim_time->nanosec = 0;
			//~ break;
		//~ case MSEC:
			//~ sim_time->seconds = ul_time / 1000;
			//~ sim_time->nanosec = (ul_time - 1000 * sim_time->seconds) * MSEC;
			//~ break;
		//~ case mSEC:
			//~ sim_time->seconds = ul_time / 1000000;
			//~ sim_time->nanosec = (ul_time - 1000000 * sim_time->seconds) * mSEC;
			//~ break;
		//~ case NSEC:
			//~ sim_time->seconds = ul_time / 1000000000;
			//~ sim_time->nanosec = ul_time - 1000000000 * sim_time->seconds;
			//~ break;
		//~ default:
			//~ break;
	//~ }
}

/**
	Compare two simulation times: time_1 < time_2.

	@param[in] time_1 -- time to use in comparision
	@param[in] time_2 -- time to use in comparision
	@retval Either -1, 0, or +1 depending on if time_1 is respectively less
	than, equal to, or greater than time_2.
 */
int
Compare_time( struct time_type *time_1, struct time_type *time_2 )
{
	int result;

	// compare seconds
	result = ( time_1->seconds < time_2->seconds ) ? -1 : 1;

	// if seconds are equal, then compare nano-seconds
	if( time_1->seconds == time_2->seconds )
	{
		result = ( time_1->nanosec < time_2->nanosec ) ? -1 : 1;

		// if nanoseconds are also equal, then time_1 and time_2 are equal
		if( time_1->nanosec == time_2->nanosec )
		{
			result = 0;
		}
	}

	return( result );
}
