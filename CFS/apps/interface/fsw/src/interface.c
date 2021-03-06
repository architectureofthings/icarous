/*******************************************************************************
 ** File: Interface.c
 **
 ** Purpose:
 **   App to interface mavlink compatible autpilot (ardupilot), ground station
 **   and Icarous.
 **
 *******************************************************************************/
#define EXTERN
#include "interface.h"
#include "interface_table.h"
#include "interface_version.h"
#include "interface_perfids.h"
#include "icarous_msg.h"
#include "icarous_msgids.h"

CFE_EVS_BinFilter_t  INTERFACE_EventFilters[] =
{  /* Event ID    mask */
		{INTERFACE_STARTUP_INF_EID,       0x0000},
		{INTERFACE_COMMAND_ERR_EID,       0x0000},
}; /// Event ID definitions

/* Interface_AppMain() -- Application entry points */
void INTERFACE_AppMain(void){

	int32 status;
	uint32 RunStatus = CFE_ES_APP_RUN;

	INTERFACE_AppInit();

	status = OS_TaskCreate( &task_1_id, "Task 1", Task1, task_1_stack, TASK_1_STACK_SIZE, TASK_1_PRIORITY, 0);
	if ( status != OS_SUCCESS ){
		OS_printf("Error creating Task 1\n");
	}

	status = OS_TaskCreate( &task_2_id, "Task 2", Task2, task_2_stack, TASK_2_STACK_SIZE, TASK_2_PRIORITY, 0);
	if ( status != OS_SUCCESS ){
		OS_printf("Error creating Task 2\n");
	}

	while(CFE_ES_RunLoop(&RunStatus) == TRUE){
		status = CFE_SB_RcvMsg(&appdataInt.INTERFACEMsgPtr, appdataInt.INTERFACE_Pipe, 1);

		if (status == CFE_SUCCESS)
		{
			INTERFACE_ProcessPacket();
		}
	}

	appdataInt.runThreads = CFE_ES_RunLoop(&RunStatus);
	INTERFACE_AppCleanUp();

	CFE_ES_ExitApp(RunStatus);
}

void INTERFACE_AppInit(void){

	memset(&appdataInt,0,sizeof(appdataInt_t));
	appdataInt.runThreads = 1;

	int32 status;

	// Register the app with executive services
	CFE_ES_RegisterApp();

	// Register the events
	CFE_EVS_Register(INTERFACE_EventFilters,
			sizeof(INTERFACE_EventFilters)/sizeof(CFE_EVS_BinFilter_t),
			CFE_EVS_BINARY_FILTER);

	// Create pipe to receive SB messages
	status = CFE_SB_CreatePipe( &appdataInt.INTERFACE_Pipe, /* Variable to hold Pipe ID */
				    INTERFACE_PIPE_DEPTH,    /* Depth of Pipe */
				    INTERFACE_PIPE_NAME);    /* Name of pipe */

	
	//Subscribe to command messages and kinematic band messages from the SB	 
	CFE_SB_Subscribe(ICAROUS_COMMANDS_MID, appdataInt.INTERFACE_Pipe);
	CFE_SB_Subscribe(ICAROUS_VISBAND_MID, appdataInt.INTERFACE_Pipe);


	// Initialize all messages that this App generates
	CFE_SB_InitMsg(&wpdata,ICAROUS_WP_MID,sizeof(waypoint_t),TRUE);              
	CFE_SB_InitMsg(&wpreached,ICAROUS_WPREACHED_MID,sizeof(missionItemReached_t),TRUE);
	CFE_SB_InitMsg(&gfdata,ICAROUS_GEOFENCE_MID,sizeof(geofence_t),TRUE);
	CFE_SB_InitMsg(&startMission,ICAROUS_STARTMISSION_MID,sizeof(ArgsCmd_t),TRUE);
	CFE_SB_InitMsg(&resetIcarous,ICAROUS_RESET_MID,sizeof(NoArgsCmd_t),TRUE);
	CFE_SB_InitMsg(&traffic,ICAROUS_TRAFFIC_MID,sizeof(object_t),TRUE);	
	CFE_SB_InitMsg(&position,ICAROUS_POSITION_MID,sizeof(position_t),TRUE);	
	CFE_SB_InitMsg(&ack,ICAROUS_COMACK_MID,sizeof(CmdAck_t),TRUE);
	

	// Send event indicating app initialization
	CFE_EVS_SendEvent (INTERFACE_STARTUP_INF_EID, CFE_EVS_INFORMATION,
						"Interface Initialized. Version %d.%d",
						INTERFACE_MAJOR_VERSION,
						INTERFACE_MINOR_VERSION);


	// Register table with table services
	status = CFE_TBL_Register(&appdataInt.INTERFACE_tblHandle,
				  "InterfaceTable",
				  sizeof(InterfaceTable_t),
				  CFE_TBL_OPT_DEFAULT,
				  &InterfaceTableValidationFunc);

	// Load app table data
	status = CFE_TBL_Load(appdataInt.INTERFACE_tblHandle,CFE_TBL_SRC_FILE,"/cf/apps/intf_tbl.tbl");

	// Check which port to open from user defined parameters
	InterfaceTable_t *TblPtr;
	status = CFE_TBL_GetAddress(&TblPtr,appdataInt.INTERFACE_tblHandle);

	char apName[50],gsName[50];

	appdataInt.ap.id = 0;
	appdataInt.ap.portType = TblPtr->apPortType;
	appdataInt.ap.portin   = TblPtr->apPortin;
	appdataInt.ap.portout  = TblPtr->apPortout;
	memcpy(appdataInt.ap.target,TblPtr->apAddress,50);

	appdataInt.gs.id = 1;
	appdataInt.gs.portType = TblPtr->gsPortType;
	appdataInt.gs.portin   = TblPtr->gsPortin;
	appdataInt.gs.portout  = TblPtr->gsPortout;
	memcpy(appdataInt.gs.target,TblPtr->gsAddress,50);

	// Free table pointer
	status = CFE_TBL_ReleaseAddress(appdataInt.INTERFACE_tblHandle);

	if (appdataInt.ap.portType == SOCKET){
		InitializeSocketPort(&appdataInt.ap);
	}else if(appdataInt.ap.portType == SERIAL){
		//InitializeSerialPort();
	}

	if (appdataInt.gs.portType == SOCKET){
		InitializeSocketPort(&appdataInt.gs);
	}else if(appdataInt.gs.portType == SERIAL){
		//InitializeSocketPort();
	}

	status = OS_MutSemCreate( &appdataInt.ap.mutex_id, "Mutex1", 0);
	if ( status != OS_SUCCESS )
	{
		OS_printf("Error creating mutex1\n");
	}else
	  {
	     //OS_printf("MutexSem ID = %d\n", (int)ap.mutex_id);
	  }

	status = OS_MutSemCreate( &appdataInt.gs.mutex_id, "Mutex2", 0);
	if ( status != OS_SUCCESS )
	{
		OS_printf("Error creating mutex2\n");
	}else
	  {
	     //OS_printf("MutexSem ID = %d\n", (int)gs.mutex_id);
	  }

	appdataInt.waypoint_type = (int*)malloc(sizeof(int)*2);
}

void INTERFACE_AppCleanUp(){
	free((void*)appdataInt.waypoint_type);
}

void Task1(void){
	//OS_printf("Starting read task\n");
	OS_TaskRegister();
	while(appdataInt.runThreads){
		GetMAVLinkMsgFromAP();
	}
}

void Task2(void){
	//OS_printf("Starting write task\n");
	OS_TaskRegister();
	while(appdataInt.runThreads){
		GetMAVLinkMsgFromGS();
	}
}


void InitializeSocketPort(port_t* prt){
	int32_t                     CFE_SB_status;
	uint16_t                   size;

	memset(&prt->self_addr, 0, sizeof(prt->self_addr));
	prt->self_addr.sin_family      = AF_INET;
	prt->self_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	prt->self_addr.sin_port        = htons(prt->portin);

	// Open a UDP socket
	if ( (prt->sockId = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		OS_printf("couldn't open socket\n");
	}

	// Bind the socket to a specific port
	if (bind(prt->sockId, (struct sockaddr *)&prt->self_addr, sizeof(prt->self_addr)) < 0) {
		// Handle case where binding failed.
		OS_printf("couldn't bind socket\n");
	}

	// Setup output port address
	memset(&prt->target_addr, 0, sizeof(prt->target_addr));
	prt->target_addr.sin_family      = AF_INET;
	prt->target_addr.sin_addr.s_addr = inet_addr(prt->target);
	prt->target_addr.sin_port        = htons(prt->portout);

	fcntl(prt->sockId, F_SETFL, O_NONBLOCK | FASYNC);

	//OS_printf("%d,Address: %s,Port in:%d,out: %d\n",prt->sockId,prt->target,prt->portin,prt->portout);
}


int readPort(port_t* prt){

	OS_MutSemTake(prt->mutex_id);
	int n = 0;
	if (prt->portType == SOCKET){
		memset(prt->recvbuffer, 0, BUFFER_LENGTH);
		n = recvfrom(prt->sockId, (void *)prt->recvbuffer, BUFFER_LENGTH, 0, (struct sockaddr *)&prt->target_addr, &prt->recvlen);
	}else if(prt->portType == SERIAL){

	}else{

	}
	OS_MutSemGive(prt->mutex_id);

	return n;
}


void writePort(port_t* prt,mavlink_message_t* message){

	char sendbuffer[300];
	uint16_t len = mavlink_msg_to_send_buffer(sendbuffer, message);
	OS_MutSemTake(prt->mutex_id);
	if(prt->portType == SOCKET){
		int n = sendto(prt->sockId, sendbuffer, len, 0, (struct sockaddr*)&prt->target_addr, sizeof (struct sockaddr_in));
	}else if(prt->portType == SERIAL){

	}else{
		// unimplemented port type
	}
	OS_MutSemGive(prt->mutex_id);

}

int GetMAVLinkMsgFromAP(){
	int n = readPort(&appdataInt.ap);
	mavlink_message_t message;
	mavlink_status_t status;
	uint8_t msgReceived = 0;
	for(int i=0;i<n;i++){
		uint8_t cp = appdataInt.ap.recvbuffer[i];
		msgReceived = mavlink_parse_char(MAVLINK_COMM_0, cp, &message, &status);
		if(msgReceived){
			// Send message to ground station
			writePort(&appdataInt.gs,&message);

			// Send SB message if necessary
			ProcessAPMessage(message);
		}
	}

	return n;
}

int GetMAVLinkMsgFromGS(){
	int n = readPort(&appdataInt.gs);
	mavlink_message_t message;
	mavlink_status_t status;
	uint8_t msgReceived = 0;
	for(int i=0;i<n;i++){
		uint8_t cp = appdataInt.gs.recvbuffer[i];
		msgReceived = mavlink_parse_char(MAVLINK_COMM_1, cp, &message, &status);
		if(msgReceived){
			ProcessGSMessage(message);
		}
	}
	return n;
}

int32_t InterfaceTableValidationFunc(void *TblPtr){

  int32_t status = 0;

  return status;
}


