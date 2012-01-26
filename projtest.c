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

	int c=0, timeIn, eventId, agentId;
	char str[128], *eventListToken, *eventName, *agentName, *timeInString;
	
	//~ time_type *sim_time = {0,0};
	//~ sim_time->seconds = 0;
	//~ sim_time->nanosec = 0;

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
		
		
		printf ("Line %d tokens: %s, %s, %d\n", c, eventName, agentName, timeIn);
		printf ("Line %d ids: %d, %d\n", c, eventId, agentId);
		printf ("Line %d time: %lu, %lu\n", c, sim_time.seconds, sim_time.nanosec);
		c++; 
		
		//Add event to event list using event ID, agent ID, and simulation time
		//~ Add_Event(eventId, agentId, &sim_time);
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
					agentId = agentNum;
					break;
				}
			}
		}
	}
	
	//~ printf("agentName = %s\n", agent_name);
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
