/************************************************************************

 This code forms the base of the operating system you will
 build.  It has only the barest rudiments of what you will
 eventually construct; yet it contains the interfaces that
 allow test.c and z502.c to be successfully built together.

 Revision History:
 1.0 August 1990
 1.1 December 1990: Portability attempted.
 1.3 July     1992: More Portability enhancements.
 Add call to SampleCode.
 1.4 December 1992: Limit (temporarily) printout in
 interrupt handler.  More portability.
 2.0 January  2000: A number of small changes.
 2.1 May      2001: Bug fixes and clear STAT_VECTOR
 2.2 July     2002: Make code appropriate for undergrads.
 Default program start is in test0.
 3.0 August   2004: Modified to support memory mapped IO
 3.1 August   2004: hardware interrupt runs on separate thread
 3.11 August  2004: Support for OS level locking
 4.0  July    2013: Major portions rewritten to support multiple threads
 4.20 Jan     2015: Thread safe code - prepare for multiprocessors
 ************************************************************************/

#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include             <stdlib.h>

//include the structures, global variables and functions
#include			"structuresStacksQueues.c"
//#include			"globalVariables.c"
//#include			"functions.c"

//  Allows the OS and the hardware to agree on where faults occur
extern void *TO_VECTOR[];

char *call_names[] = { "mem_read ", "mem_write", "read_mod ", "get_time ",
		"sleep    ", "get_pid  ", "create   ", "term_proc", "suspend  ",
		"resume   ", "ch_prior ", "send     ", "receive  ", "disk_read",
		"disk_wrt ", "def_sh_ar" };

/*typedef struct Create_Process_Struct {
	char process_name[20];
	long process_to_run;
	long priority;
	long *process_context;
	long *return_status;
} Create_Process_Struct; */

static int iteration = 0;
static int iteration3 = 0;
static int iteration5 = 0;
static int iteration7 = 0;
static int iteration9 = 0;
static int iteration11 = 0;

//int shadowDiskID = 1;

int SD = 1;
int SD3 = 2;
int SD5= 3;
int SD7 = 4;
int SD9 = 5;
int SD11 = 6;

int RD = 7;
int RDS = 1;

//int shadowSector = 1;

int SS = 1;
int SS3 = 1;
int SS5 = 1;
int SS7 = 1;
int SS9 = 1;
int SS11 = 1;

//int shadowTableRow = 0;

int STR = 0;
int STR3 = 0;
int STR5 = 0;
int STR7 = 0;
int STR9 = 0;
int STR11 = 0;

//int shadowTable[1024][3];

int ST[1024][3];
int ST3[1024][3];
int ST5[1024][3];
int ST7[1024][3];
int ST9[1024][3];
int ST11[1024][3];

//int victimFrames[64];

int VF[64];
int VF3[64];
int VF5[64];
int VF7[64];
int VF9[64];
int VF11[64];

void writeShadowTable(int vPgNo, int shadowTable[][3], int *shadowDiskID, int *shadowSector, int *shadowTableRow)
{
	int i;
	
	for (i = 0; i < 1024; i++)
	{
		if (shadowTable[i][0] == vPgNo)
		{
			break;
		}
	}

	if (i < 1024)
	{
		// virtual page entry already exists in shadow table. Need to update instead of insert
		if (shadowTable[i][0] == 0 && shadowTable[i][1] == 0 && shadowTable[i][2] == 0)
		{  
			// this will occur when all the entries in shadow table are  0 0 0
			// so for pg no 0 we we add disk ID and sector number
			// and move the shadowTableRow to the next value

			//shadowTable[i][0] = vPgNo;
			shadowTable[i][1] = *shadowDiskID; // we can simply assign the current disk ID and Sector number
			shadowTable[i][2] = *shadowSector; // as it is the 1st entry in the table.

			*shadowTableRow = *shadowTableRow + 1;
			*shadowSector = *shadowSector + 1;
			/*if (*shadowSector > 1599)
			{
				*shadowDiskID = *shadowDiskID + 1;
				*shadowSector = 1;
			}*/
		}
		// if any other pg no. entry is found in the shadow table,
		// then we don't need to assign a new disk ID and frame to that pg no.
		// as it already has one

	}
	else
	{	// the pg no. is not already in the shadow table
		// hence a new disk ID and sector needs to be assigned 

		if (*shadowSector > 1599) // this is corresponding process's dedicated disk's sector
		{	// dedicated disk is full
			// need to use reserved disks

			shadowDiskID = &RD;
			shadowSector = &RDS;
		}

		shadowTable[*shadowTableRow][0] = vPgNo;
		shadowTable[*shadowTableRow][1] = *shadowDiskID;
		shadowTable[*shadowTableRow][2] = *shadowSector;

		if (*shadowTableRow <= 1023)
		{
			*shadowTableRow = *shadowTableRow + 1;
		}
		else
		{
			// this should not ideally occur as the number of rows in shadow table = number of rows in Page table, i.e. 1024
			printf("ERROR!!! shadow table is full\n");
		}

		if (*shadowDiskID != 7 && *shadowDiskID != 8)
		{
			*shadowSector = *shadowSector + 1;
		}
		else if ((*shadowDiskID == 7 || *shadowDiskID == 8) && *shadowSector < 1599)
		{
			*shadowSector = *shadowSector + 1;
		}
		else if(*shadowDiskID == 7 && *shadowSector >= 1599)
 		{
			*shadowDiskID = 8;
			*shadowSector = 1;
		}
		else if(*shadowDiskID == 8 && *shadowSector >= 1599)
		{
			printf("ERROR!!! shadow disk ID 8 is also full\n");
		}

		
	}
	/*else
	{
		// we should not enter here
		printf("shadow table Row: %d; Terminating..\n", shadowTableRow);
	}*/


}

void readVictimFrameWriteDisk(int vPgNo, int phy_add, int shadowTable[][3])
{
	char victimFrameData[16];
	int i, diskID, sector,char_data[16];

	//int **shadowTable;

	/*switch (PID)
	{
	case 1: shadowTable = ST;
		break;
	default:
		break;
	}*/
	
	//Z502ReadPhysicalMemory(vPgNo, (char *)victimFrameData);

	// searching the diskID and Sector number from the shadow for this page number

	for (i = 0; i < 1024; i++)
	{
		if (shadowTable[i][0] == vPgNo)
		{
			diskID = shadowTable[i][1];
			sector = shadowTable[i][2];
			break;
		}
	}

	if (i < 1024)
	{
		Z502ReadPhysicalMemory(phy_add, (char *)char_data);

		DISK_WRITE(diskID, sector, (char*)(char_data));

		printf("victim frame: %d, written to disk ID: %d, sector: %d, data written: %d\n", phy_add, diskID, sector, char_data[0]);
	}
}

void readDiskForCurrentVirtualPage(int pgNo, int frameNo, int shadowTable[][3])
{
	int i, diskID, sector, char_data[16] = { 0 };

	/*int **shadowTable;

	switch (PID)
	{
	case 1: shadowTable = ST;
		break;
	default:
		break;
	}*/

	// searching the diskID and Sector number from the shadow for this page number
	for (i = 0; i < 1024; i++)
	{
		if (shadowTable[i][0] == pgNo)
		{
			diskID = shadowTable[i][1];
			sector = shadowTable[i][2];
			break;
		}
	}

	if (i < 1024)
	{
		DISK_READ(diskID, sector, (char*)(char_data));

		//printf("Read data is: %d\n", char_data[0]);

		SetMode(KERNEL_MODE);
		Z502WritePhysicalMemory(frameNo, (char*)(char_data));

		//Z502ReadPhysicalMemory(frameNo, (char *)char_data);

		printf("For Frame: %d, Read Disk: %d, Sector: %d, Read data is: %d\n",frameNo, diskID,sector, char_data[0]);
	}
	else
	{
		printf("IMP: page number not found in shadow table\n");
	}


}

/************************************************************************
 INTERRUPT_HANDLER
 When the Z502 gets a hardware interrupt, it transfers control to
 this routine in the OS.
 ************************************************************************/
void InterruptHandler(void) {
	INT32 DeviceID; long timeOfDay;
//	INT32 Status; 
	INT32 LockResult;
	char Success[] = "      Action Failed\0        Action Succeeded";

	PCB_stack *tmp;
	int z;
	ready_queue *readyTmp;
	timer_queue *timerTmp;
	SP_INPUT_DATA SPData;    // Used to feed SchedulerPrinter


	MEMORY_MAPPED_IO mmio;       // Enables communication with hardware

	static BOOL remove_this_in_your_code = TRUE; /** TEMP **/
	static INT32 how_many_interrupt_entries = 0; /** TEMP **/

	/*READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));*/

	READ_MODIFY(MEMORY_INTERLOCK_BASE + 30, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	//Message("%s\n", &(Success[SPART * LockResult]));


	// Get cause of interrupt
	mmio.Mode = Z502GetInterruptInfo;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	MEM_READ(Z502InterruptDevice, &mmio);
	DeviceID = mmio.Field1;
	//Status = mmio.Field2;


	/** REMOVE THE NEXT SIX LINES **/
	how_many_interrupt_entries++; /** TEMP **/
	//if (remove_this_in_your_code && (how_many_interrupt_entries < 40)) {
		printf("Interrupt_handler: Found device ID %d with status %d\n",
				(int) mmio.Field1, (int) mmio.Field2);
	//}

		/*READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		printf("%s\n", &(Success[SPART * LockResult]));

		READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		printf("%s\n", &(Success[SPART * LockResult]));*/

	// Clear out this device - we're done with it
	mmio.Mode = Z502ClearInterruptStatus;
	mmio.Field1 = DeviceID;
	mmio.Field2 = mmio.Field3 = 0;
	MEM_WRITE(Z502InterruptDevice, &mmio);

	/*READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	printf("%s\n", &(Success[SPART * LockResult]));*/

	if (DeviceID == TIMER_INTERRUPT)
	{
		
		
		//popTimerQueue();
		GET_TIME_OF_DAY(&timeOfDay);

		

		// trying to get a lock
		READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		//Message("%s\n", &(Success[SPART * LockResult]));

		READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		//Message("%s\n", &(Success[SPART * LockResult]));

		

		updateTimerQueue(timeOfDay);

		/*READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		printf("%s\n", &(Success[SPART * LockResult]));

		READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		printf("%s\n", &(Success[SPART * LockResult]));
	*/
		
		popTimerQueue();

		//printf("timer interrupt\n");

		/*READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		printf("%s\n", &(Success[SPART * LockResult]));

		READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		printf("%s\n", &(Success[SPART * LockResult]));
	*/
		sort_timer_queue();

		

		//make_ready_to_run_after_timer_interrupt();

		/*READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult);
		printf("%s\n", &(Success[SPART * LockResult]));*/
		//make_ready_to_run();
	}
	READ_MODIFY(MEMORY_INTERLOCK_BASE + 30, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	//make_ready_to_run_after_timer_interrupt();
	/*tmp = GetCurrentRunningProcess();

	printf("%s\n", tmp->process_name);*/

	//Message("%s\n", &(Success[SPART * LockResult]));

	//Message("interrupt handler completed\n");

	//printf("1");

	

	if (DeviceID > TIMER_INTERRUPT && DeviceID < 11)
	{
		//CALL(50);
		//printf("calling add to ready queue from disk interrupt\n");
		addDiskToReady(DeviceID);
	}
}           // End of InterruptHandler


/************************************************************************
 FAULT_HANDLER
 The beginning of the OS502.  Used to receive hardware faults.

 Handling Privilege instructuions:
 DeviceID for Privileged_Instruction is changed to '9' in global.h
 So that Timer_interrupt(4) can be distinguished from Privilege_Instruction
 If Privilege Instruction is received, Terminate All routine is called
 ************************************************************************/

void FaultHandler(void) {
	INT32 DeviceID;
	INT32 Status; 
	long ErrorReturned;
	SP_INPUT_DATA SPData;    // Used to feed SchedulerPrinter
	PCB_stack *tmp;
	ready_queue* tmpReady;
	int phy_add;
	int bit_value, bv;
	int bit_pos = 0;
	int i, k, leastValue, vPgNo, validBit, TmpBit, tmp_phy_add, z, u;
	int frameNumber[6] = { 0 };
	int victimFrameSelected[6] = { 0 };

	int *itrn;
	int *victimFrames;
	int **shadowTable;
	int *shadowSector, *shadowDiskID, *shadowTableRow;

	MEMORY_MAPPED_IO mmio;       // Enables communication with hardware

	// Get cause of interrupt
	mmio.Mode = Z502GetInterruptInfo;
	MEM_READ(Z502InterruptDevice, &mmio);
	DeviceID = mmio.Field1;
	Status = mmio.Field2;
	
	
	printf("Fault_handler: Found vector type %d with value %d\n", DeviceID,
			Status);

	// Clear out this device - we're done with it
	mmio.Mode = Z502ClearInterruptStatus;
	mmio.Field1 = DeviceID;
	MEM_WRITE(Z502InterruptDevice, &mmio);
	
	if(DeviceID == 2)
	{ 
		if (Status < 1024 && Status >= 0)
		{
			tmp = GetCurrentRunningProcess();

			UINT16 *add = tmp->pageTable;

			validBit = (add[(UINT16)Status] & (1 << 15)) >> 15;
			TmpBit = (add[(UINT16)Status] & (1 << 12)) >> 12;

			printf("valid bit: %d\nTmp Bit: %d\n", validBit, TmpBit);

			switch (tmp->PID)
			{
			case 1:
				itrn = &iteration;
				victimFrames = VF;
				shadowTable = ST;
				shadowDiskID = &SD;
				shadowSector = &SS;
				shadowTableRow = &STR;
				break;

			case 3:
				itrn = &iteration3;
				victimFrames = VF3;
				shadowTable = ST3;
				shadowDiskID = &SD3;
				shadowSector = &SS3;
				shadowTableRow = &STR3;
				break;

			case 5:
				itrn = &iteration5;
				victimFrames = VF5;
				shadowTable = ST5;
				shadowDiskID = &SD5;
				shadowSector = &SS5;
				shadowTableRow = &STR5;
				break;

			case 7:
				itrn = &iteration7;
				victimFrames = VF7;
				shadowTable = ST7;
				shadowDiskID = &SD7;
				shadowSector = &SS7;
				shadowTableRow = &STR7;
				break;

			case 9:
				itrn = &iteration9;
				victimFrames = VF9;
				shadowTable = ST9;
				shadowDiskID = &SD9;
				shadowSector = &SS9;
				shadowTableRow = &STR9;
				break;

			case 11:
				itrn = &iteration11;
				victimFrames = VF11;
				shadowTable = ST11;
				shadowDiskID = &SD11;
				shadowSector = &SS11;
				shadowTableRow = &STR11;
				break;

			default:
				break;
			}
			
				if (*itrn <= 63)
				{
					printf("iteration should not be more than 63: %d\n", *itrn);
					phy_add = *itrn;

					if (validBit == 0 && TmpBit == 0) // May not need to check this
					{

						for (u = 0; u < 12; u++)
						{
							add[(UINT16)Status] &= ~(1 << u);
						}

						if (phy_add > 63)
						{
							printf("In loop of < 64\n");
							printf("PID: %d, phy_add: %d\n", tmp->PID, phy_add);
							TERMINATE_PROCESS(-2, &ErrorReturned);
						}

						// Below while loop will assign binary value of the physical page address
						// in a bit-wise fashion
						while (phy_add != 0 && phy_add != 1)
						{
							bit_value = phy_add % 2;
							add[(UINT16)Status] |= bit_value << bit_pos;
							phy_add = (int)phy_add / 2;
							bit_pos++;
						}

						// Below binary value assignment is completed
						add[(UINT16)Status] |= phy_add << bit_pos;

						// Next physical address that can be assigned is set
						//iteration++;

						*itrn = *itrn + 1;

						// Valid bit is set to 1
						add[(UINT16)Status] |= 1 << 15;


					}
					else
					{// we should not enter here
						printf("iteration is <= 63 but valid and/or 12th bit is not 0\n");
					}
				}

				// all 0-63 physical frames are used up
				// and the currrent virtual page number needs a frame
				// as its invalid and the corresponding frame data may or may not be written to disk
				else if (*itrn > 63 && validBit == 0)
				{
					//printf("iteration: %d\n", *itrn);

					leastValue = victimFrames[0];

					printf("least value: %d\n", leastValue);

					// this for loop will find the victim frame 
					for (i = 1; i < 64; i++)
					{
						if (victimFrames[i] < leastValue)
						{
							leastValue = victimFrames[i];
							victimFrames[i] = victimFrames[i] + 1;
							break;
						}
					}

					// all the victimFrames have same value
					// hence frame 0  is selected as victim
					if (i == 64)
					{
						i = 0;
						victimFrames[i] = victimFrames[i] + 1;
					}

					phy_add = i;

					tmp_phy_add = phy_add;

					printf("victim frame: %d\n", phy_add);

					z = 5;

					while (tmp_phy_add != 0 && tmp_phy_add != 1)
					{
						bv = tmp_phy_add % 2;
						victimFrameSelected[z] = bv;
						tmp_phy_add = (int)tmp_phy_add / 2;
						z--;
					}

					// Below binary value assignment is completed
					victimFrameSelected[z] = tmp_phy_add;

					/*for (int t = 0; t < 6; t++)
					{
						printf("victimFrameSelected[%d]: %d\n", t, victimFrameSelected[t]);
					}*/

					

					// finding virtual page number where frame number == phy add (victim frame)
					for (vPgNo = 0; vPgNo < 1024; vPgNo++)
					{
						//printf("vPgNo: %d\n", vPgNo);

						//printf("page table entry: %d\n", add[vPgNo]);

						//printf("at location 15: %d\n", (add[vPgNo] & (1 << 15)) >> 15);

						z = 5;

						for (k = 0; k < 6; k++)
						{
							//printf("k: %d, %d\n", k, (add[vPgNo] & (1 << k)) >> k);
							frameNumber[z] = (add[vPgNo] & (1 << k)) >> k;
							z--;
						}

						for (z = 0; z < 6; z++)
						{
							if (frameNumber[z] != victimFrameSelected[z])
							{
								break;
							}
						}

						int val15bit = (add[vPgNo] & (1 << 15)) >> 15;

						if (z == 6 && val15bit == 1)
						{
							break;
						}

					}

					if (vPgNo < 1024)
					{
						printf("Victim frame found at virtual page number: %d\n", vPgNo);

						add[vPgNo] |= 1 << 12; // 12th bit tells us if the virtual page is written to disk

						//add[vPgNo] |= 0 << 13;
						//add[vPgNo] |= 0 << 14; 
						add[vPgNo] &= ~(1 << 15); // we make the virtual page invalid as its frame value is written to disk

						for (int t = 0; t < 12; t++)
						{
							add[vPgNo] &= ~(1 << t); // the assigned victim frame is released
						}
						
						// write pg number, disk ID and sector to the shadow table
						writeShadowTable(vPgNo, shadowTable,shadowDiskID,shadowSector,shadowTableRow); 

						// Read the data at victim frame memory location and write it to disk
						readVictimFrameWriteDisk(vPgNo, phy_add, shadowTable); 

						//printf("victim virtual page: %d, victim frame: %d, current virtual page: %d \n", vPgNo,phy_add,Status);


						// write the victim frame address to the current virtual page, i.e. the new one 
						//for which we found the phy. frame
						tmp_phy_add = phy_add;

						for (u = 0; u < 12; u++)
						{
							add[(UINT16)Status] &= ~(1 << u);
						}

						if (phy_add > 63)
						{
							printf("PID: %d, phy_add: %d\n", tmp->PID, phy_add);
							TERMINATE_PROCESS(-2, &ErrorReturned);
						}

						while (phy_add != 0 && phy_add != 1)
						{
							bit_value = phy_add % 2;
							add[(UINT16)Status] |= bit_value << bit_pos;
							phy_add = (int)phy_add / 2;
							bit_pos++;
						}

						// Below binary value assignment is completed
						add[(UINT16)Status] |= phy_add << bit_pos;


						// the virtual page entry is invalid but 
						// the corresponding physical frame data is written to disk
						if (TmpBit == 1)
						{
							// So we read from disk and write to the victim frame selected
							readDiskForCurrentVirtualPage(Status, tmp_phy_add, shadowTable);
							add[(UINT16)Status] &= ~(1 << 12); // latest data is now in memory; hence 12th bit is cleared
						}

						// Valid bit is set to 1
						add[(UINT16)Status] |= 1 << 15;
					}
					else
					{// we should not be entering this part
						printf("virtual page number of the victim frame not found\n");
					}

				}
			
		}
		else
		{
			//printf("The address is out of address\n");
			TERMINATE_PROCESS(-2, &ErrorReturned);
		}
		
	}
	if (DeviceID == PRIVILEGED_INSTRUCTION)
	{
		//Message("Privilege Instruction called in User Mode..Exiting\n"); // Message

		memset(&SPData, 0, sizeof(SP_INPUT_DATA));
		strcpy(SPData.TargetAction, "Fault");

		tmp = GetCurrentRunningProcess();

		SPData.CurrentlyRunningPID = tmp->PID;
		SPData.TargetPID = 0;
		// The NumberOfRunningProcesses as used here is for a future implementation
		// when we are running multiple processors.  For right now, set this to 0
		// so it won't be printed out.
		SPData.NumberOfRunningProcesses = 0;

		SPData.NumberOfReadyProcesses = returnReadyQueueCount();   // Processes ready to run

		tmpReady = returnFrontReadyQueue();

		for (i = 0; i < SPData.NumberOfReadyProcesses; i++) {
			if (i == 0)
			{
				//tmpReady = front_ready_queue;
			}
			else
			{
				tmpReady = tmpReady->next_ready_process;
			}
			SPData.ReadyProcessPIDs[i] = tmpReady->current_ready_process_addr->PID;
		}

		CALL(SPPrintLine(&SPData));

		TERMINATE_PROCESS(-2, &ErrorReturned);
	}

} // End of FaultHandler

/************************************************************************
 SVC
 The beginning of the OS502.  Used to receive software interrupts.
 All system calls come to this point in the code and are to be
 handled by the student written code here.
 The variable do_print is designed to print out the data for the
 incoming calls, but does so only for the first ten calls.  This
 allows the user to see what's happening, but doesn't overwhelm
 with the amount of data.
 ************************************************************************/

void svc(SYSTEM_CALL_DATA *SystemCallData) {
	short call_type;
	static short do_print = 10;
	short i;
	SP_INPUT_DATA SPData;    SP_INPUT_DATA SPData2; // Used to feed SchedulerPrinter
	PCB_stack *tmp;
	//int i;
	ready_queue *readyTmp;
	timer_queue *timerTmp;
	disk_queue *tmp_disk;
	long ErrorReturned;

	MEMORY_MAPPED_IO mmio;
	SYSTEM_CALL_DATA *pop_process_data; 

	//Message("in SVC before call type: do_print: %d\n", do_print); // Message

	call_type = (short)SystemCallData->SystemCallNumber;
	if (do_print > 0) {
		printf("SVC handler: %s\n", call_names[call_type]);
		for (i = 0; i < SystemCallData->NumberOfArguments - 1; i++) {
			//Value = (long)*SystemCallData->Argument[i];
			printf("Arg %d: Contents = (Decimal) %8ld,  (Hex) %8lX\n", i,
				(unsigned long)SystemCallData->Argument[i],
				(unsigned long)SystemCallData->Argument[i]);
		}
		do_print--;

		//Message("in SVC above switch case\n"); // should be changed to Message

		switch (call_type) {
			// Get time service call
		case SYSNUM_GET_TIME_OF_DAY:   // This value is found in syscalls.h
			mmio.Mode = Z502ReturnValue;
			mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
			MEM_READ(Z502Clock, &mmio);
			*SystemCallData->Argument[0] = mmio.Field1;
			//Message("Time of the day: %d\n", *SystemCallData->Argument[0]); //Message
			break;

			// SLEEP system call
		case SYSNUM_SLEEP:
			CustomStartTimer(0, SystemCallData->Argument[0]);
			break;

		case SYSNUM_CREATE_PROCESS:
			//Message("Came in SVC Create Process Block\n"); //Message
														   /*Validate_Process_Data(SystemCallData);*/

			only_create_process(SystemCallData);
			//Message("ErrorReturned in SVC: %d\n\n\n", *SystemCallData->Argument[4]); //Message

			if (*SystemCallData->Argument[4] != ERR_SUCCESS)
			{
				//Message("%d\n", *SystemCallData->Argument[4]);
				//Message("Exiting!!! As error has occurred\n");
				return;
			}

			*SystemCallData->Argument[4] = (long)ERR_SUCCESS;
			break;

			// terminate system call
			// based on the value passed
			// either terminate all (for negative values)
			// or terminate process will be called (positive values)
		case SYSNUM_TERMINATE_PROCESS:

			//printf("entered terminate process correctly\n");
			//getch();
			//Message("in Terminate switch case\n");
			//Message("system argument 0: %ld, %d\n", SystemCallData->Argument[0], SystemCallData->Argument[0]);

			if ((int)SystemCallData->Argument[0] < 0) {
				//printf("entered Terminate all mode\n");
				if (returnReadyQueueCount() > 1)
				{
					//printf("ready count greater than 1\n");
					tmp = GetCurrentRunningProcess();
					SystemCallData->Argument[0] = tmp->process_context;
					pop_process(SystemCallData);
				}
				else
				{
					*SystemCallData->Argument[1] = ERR_SUCCESS;
					mmio.Mode = Z502Action;
					mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
					MEM_WRITE(Z502Halt, 0);
				}
				break;
			}
			else
			{
				//Message("entered pop\n");
				pop_process(SystemCallData);
				break;
			}

		case SYSNUM_SUSPEND_PROCESS:

			//Message("Suspending %d\n", SystemCallData->Argument[0]);
			CustomSuspendProcess(SystemCallData);

			break;

		case SYSNUM_RESUME_PROCESS:
			//Message("Resuming %d\n", SystemCallData->Argument[0]);
			CustomResumeProcess(SystemCallData);

			break;

		case SYSNUM_GET_PROCESS_ID:
			get_process_id(SystemCallData);
			break;

		case SYSNUM_CHANGE_PRIORITY:
			//Message("Changing priority for %d\n", SystemCallData->Argument[0]);
			CustomChangePriority(SystemCallData);
			break;

		case SYSNUM_DISK_WRITE:
			addToDiskQueue(SystemCallData, 'w');
			//CustomDiskWrite(SystemCallData);
			break;	

		case SYSNUM_DISK_READ:
			addToDiskQueue(SystemCallData, 'r');
			//CustomDiskRead(SystemCallData);
			break;

		default:
			//Message("ERROR!  call_type not recognized!\n");
			//Message("Call_type is - %i\n", call_type);
			break;
		}                                           // End of switch

	}
	else
	{
		switch (call_type) {
			// Get time service call
		case SYSNUM_GET_TIME_OF_DAY:   // This value is found in syscalls.h
			mmio.Mode = Z502ReturnValue;
			mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
			MEM_READ(Z502Clock, &mmio);
			*SystemCallData->Argument[0] = mmio.Field1;
			//Message("Time of the day: %d\n", *SystemCallData->Argument[0]); //Message
			break;

			// SLEEP system call
		case SYSNUM_SLEEP:
			//SetMode(KERNEL_MODE);
			CustomStartTimer(0, SystemCallData->Argument[0]);
			break;

		case SYSNUM_CREATE_PROCESS:
			//Message("Came in SVC Create Process Block\n"); //Message
														   /*Validate_Process_Data(SystemCallData);*/

			only_create_process(SystemCallData);
			//Message("ErrorReturned in SVC: %d\n\n\n", *SystemCallData->Argument[4]); //Message

			if (*SystemCallData->Argument[4] != ERR_SUCCESS)
			{
				//Message("%d\n", *SystemCallData->Argument[4]);
				//Message("Exiting!!! As error has occurred\n");
				return;
			}

			*SystemCallData->Argument[4] = (long)ERR_SUCCESS;
			break;

			// terminate system call
			// based on the value passed
			// either terminate all (for negative values)
			// or terminate process will be called (positive values)
		case SYSNUM_TERMINATE_PROCESS:

			//Message("entered terminate process correctly\n");
			//getch();
			//Message("in Terminate switch case\n");
			//Message("system argument 0: %ld, %d\n", SystemCallData->Argument[0], SystemCallData->Argument[0]);

			if ((int)SystemCallData->Argument[0] < 0) {
				//Message("entered Terminate all mode\n");
				//if (returnReadyQueueCount() > 1)
				
				tmp = GetCurrentRunningProcess();
				
				/*************************************************************************************************
				Below if statement checks if only process needs to be terminated or
				if the entire simulation needs to be terminated
				if the terminate argument in -1 and if it's not the 1st process then we terminate only the process
				else if it's the 1st process then terminate the simulation
				**************************************************************************************************/

				if(SystemCallData->Argument[0] == -1 && tmp->PID != 1)
				{
					//tmp = GetCurrentRunningProcess();
					/*printf("%s\n", tmp->process_name);
					SystemCallData->Argument[0] = tmp->process_context;
					pop_process(SystemCallData);*/
					tmp->processing_status = PROCESSED;

					/*mmio.Mode = Z502Action;
					mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
					MEM_WRITE(Z502Idle, &mmio);*/

					//make_ready_to_run_after_timer_interrupt();

					pop_process_data = (SYSTEM_CALL_DATA *) calloc(1, sizeof(SYSTEM_CALL_DATA));

					//printf("process to be popped: %d\n", tmp->process_context);

					//long *context = tmp->process_context;

					pop_process_data->Argument[0] = tmp->process_context;
					pop_process_data->Argument[1] = &ErrorReturned;

					//printf(pop_process_data->Argument[0]);

					pop_process(pop_process_data);

					svc_term_context_switch();
				}
				else
				{
					*SystemCallData->Argument[1] = ERR_SUCCESS;
					mmio.Mode = Z502Action;
					mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
					MEM_WRITE(Z502Halt, 0);
				}
				break;
			}
			else
			{
				//Message("entered pop\n");
				pop_process(SystemCallData);
				break;
			}

		case SYSNUM_SUSPEND_PROCESS:

			//Message("Suspending %d\n", SystemCallData->Argument[0]);
			CustomSuspendProcess(SystemCallData);

			break;

		case SYSNUM_RESUME_PROCESS:
			//Message("Resuming %d\n", SystemCallData->Argument[0]);
			CustomResumeProcess(SystemCallData);

			break;

		case SYSNUM_GET_PROCESS_ID:
			get_process_id(SystemCallData);
			break;

		case SYSNUM_CHANGE_PRIORITY:
			//Message("Changing priority for %d\n", SystemCallData->Argument[0]);
			CustomChangePriority(SystemCallData);
			break;

		case SYSNUM_DISK_WRITE:
			addToDiskQueue(SystemCallData, 'w');
			//CustomDiskWrite(SystemCallData);
			break;	

		case SYSNUM_DISK_READ:
			addToDiskQueue(SystemCallData, 'r');
			//CustomDiskRead(SystemCallData);
			break;

		default:
			//Message("ERROR!  call_type not recognized!\n");
			//Message("Call_type is - %i\n", call_type);
			break;
		}                                           // End of switch

	}                                           // End of switch
}

// End of svc


/************************************************************************
 osInit
 This is the first routine called after the simulation begins.  This
 is equivalent to boot code.  All the initial OS components can be
 defined and initialized here.
 ************************************************************************/

void osInit(int argc, char *argv[]) {
	void *PageTable = (void *)calloc(2, VIRTUAL_MEM_PAGES);
	INT32 i; char test_no_chr[10]; long status; long context;

	MEMORY_MAPPED_IO mmio;
	SYSTEM_CALL_DATA *Create_Process_Data = (SYSTEM_CALL_DATA *)calloc(1, sizeof(SYSTEM_CALL_DATA));


	// Demonstrates how calling arguments are passed thru to here       
	//printf( "inside osInit\n" );

	printf("Program called with %d arguments:", argc);
	for (i = 0; i < argc; i++)
		printf(" %s", argv[i]);
	printf("\n");
	printf("Calling with argument 'sample' executes the sample program.\n");
	// Here we check if a second argument is present on the command line.
	// If so, run in multiprocessor mode
	if (argc > 2) {
		printf("Simulation is running as a MultProcessor\n\n");
		mmio.Mode = Z502SetProcessorNumber;
		//Message("Z502SetProcessorNumber done successfully\n");
		mmio.Field1 = MAX_NUMBER_OF_PROCESSORS;
		//Message("MAX_NUMBER_OF_PROCESSORS done successfully\n");
		mmio.Field2 = (long)0;
		mmio.Field3 = (long)0;
		mmio.Field4 = (long)0;

		MEM_WRITE(Z502Processor, &mmio);   // Set the number of processors
	}
	else {
		printf("Simulation is running as a UniProcessor\n");
		printf("Add an 'M' to the command line to invoke multiprocessor operation.\n\n");
	}

	//          Setup so handlers will come to code in base.c   
	//printf("outside multiprocessor if block\n");

	TO_VECTOR[TO_VECTOR_INT_HANDLER_ADDR] = (void *)InterruptHandler;
	TO_VECTOR[TO_VECTOR_FAULT_HANDLER_ADDR] = (void *)FaultHandler;
	TO_VECTOR[TO_VECTOR_TRAP_HANDLER_ADDR] = (void *)svc;

	//  Determine if the switch was set, and if so go to demo routine. 

	PageTable = (void *)calloc(2, VIRTUAL_MEM_PAGES);

	//Below code does the following:
	//1. checks if the command line arguments were passed
	//2. calls the context for appropriate test in the switch case

	if ((argc > 1) && (strcmp(argv[1], "") != 0)) {
		strcpy(test_no_chr, argv[1]);

		if (test_no_chr[4] == '1')
		{
			switch (test_no_chr[5])
			{
			case 'a':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test1a";
				Create_Process_Data->Argument[1] = (long *)test1a;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				//Message("process_name: %s\n process_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//			Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");
				//if (Create_Process_Data->Argument[4] != ERR_SUCCESS)

				////Message("status:%d", status);
				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					//Message("%d Exiting!!! As error has occurred", status);
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			case 'b':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test1b";
				Create_Process_Data->Argument[1] = (long *)test1b;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				//Message("process_name: %s\nprocess_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");

				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					//Message("Exiting!!! As error has occurred");
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			case 'c':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test1c";
				Create_Process_Data->Argument[1] = (long *)test1c;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				//Message("process_name: %s\nprocess_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");

				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					//Message("Exiting!!! As error has occurred");
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			case 'd':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test1d";
				Create_Process_Data->Argument[1] = (long *)test1d;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				//Message("process_name: %s\nprocess_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");

				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					//Message("Exiting!!! As error has occurred");
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			case 'e':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test1e";
				Create_Process_Data->Argument[1] = (long *)test1e;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				//Message("process_name: %s\nprocess_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");

				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					//Message("Exiting!!! As error has occurred");
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			case 'f':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test1f";
				Create_Process_Data->Argument[1] = (long *)test1f;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				//Message("process_name: %s\nprocess_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");

				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					//Message("Exiting!!! As error has occurred");
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			case 'g':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test1g";
				Create_Process_Data->Argument[1] = (long *)test1g;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				//Message("process_name: %s\nprocess_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");

				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					//Message("Exiting!!! As error has occurred");
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			case 'h':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test1h";
				Create_Process_Data->Argument[1] = (long *)test1h;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				//Message("process_name: %s\n process_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//			Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");
				//if (Create_Process_Data->Argument[4] != ERR_SUCCESS)

				////Message("status:%d", status);
				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					//Message("%d Exiting!!! As error has occurred", status);
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			case 'k':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test1k";
				Create_Process_Data->Argument[1] = (long *)test1k;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				//Message("process_name: %s\nprocess_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");

				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					//Message("Exiting!!! As error has occurred");
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			case 'l':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test1l";
				Create_Process_Data->Argument[1] = (long *)test1l;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				//Message("process_name: %s\nprocess_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");

				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					//Message("Exiting!!! As error has occurred");
					return;
				}
				os_create_process(Create_Process_Data);
				break;
			}

		}

		if (test_no_chr[4] == '2')
		{
			switch (test_no_chr[5])
			{
			case 'a':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test2a";
				Create_Process_Data->Argument[1] = (long *)test2a;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				//Message("process_name: %s\n process_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//			Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");
				//if (Create_Process_Data->Argument[4] != ERR_SUCCESS)

				////Message("status:%d", status);
				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					//Message("%d Exiting!!! As error has occurred", status);
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			case 'b':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test2b";
				Create_Process_Data->Argument[1] = (long *)test2b;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				//Message("process_name: %s\nprocess_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");

				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					//Message("Exiting!!! As error has occurred");
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			case 'c':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test2c";
				Create_Process_Data->Argument[1] = (long *)test2c;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				//Message("process_name: %s\nprocess_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");

				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					//Message("Exiting!!! As error has occurred");
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			case 'd':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test2d";
				Create_Process_Data->Argument[1] = (long *)test2d;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				//Message("process_name: %s\nprocess_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");

				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					//Message("Exiting!!! As error has occurred");
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			case 'e':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test2e";
				Create_Process_Data->Argument[1] = (long *)test2e;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				//Message("process_name: %s\nprocess_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");

				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					//Message("Exiting!!! As error has occurred");
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			case 'f':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test2f";
				Create_Process_Data->Argument[1] = (long *)test2f;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				//Message("process_name: %s\nprocess_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");

				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					//Message("Exiting!!! As error has occurred");
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			case 'g':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test2g";
				Create_Process_Data->Argument[1] = (long *)test2g;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				//Message("process_name: %s\nprocess_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");

				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					//Message("Exiting!!! As error has occurred");
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			case 'h':
				Create_Process_Data->NumberOfArguments = 5;
				Create_Process_Data->Argument[0] = (long *) "test2h";
				Create_Process_Data->Argument[1] = (long *)test2h;
				Create_Process_Data->Argument[2] = 1;
				Create_Process_Data->Argument[3] = &context;
				Create_Process_Data->Argument[4] = &status;
				//Message("process_name: %s\n process_to_run: %d \n", Create_Process_Data->Argument[0], Create_Process_Data->Argument[1]);
				//			Validate_Process_Data(Create_Process_Data);
				//SuccessExpected(Create_Process_Data->Argument[4], "CREATE_PROCESS");
				//if (Create_Process_Data->Argument[4] != ERR_SUCCESS)

				////Message("status:%d", status);
				only_create_process(Create_Process_Data);

				if (status != ERR_SUCCESS)
				{
					//Message("%d Exiting!!! As error has occurred", status);
					return;
				}
				os_create_process(Create_Process_Data);
				break;

			}

		}


		/*		mmio.Mode = Z502InitializeContext;
		mmio.Field1 = 0;
		mmio.Field2 = (long) test1a;
		mmio.Field3 = (long) PageTable;

		MEM_WRITE(Z502Context, &mmio);   // Start of Make Context Sequence
		mmio.Mode = Z502StartContext;
		// Field1 contains the value of the context returned in the last call
		mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
		MEM_WRITE(Z502Context, &mmio);     // Start up the context
		*/

	} // End of handler for sample code - This routine should never return here

	  //  By default test0 runs if no arguments are given on the command line
	  //  Creation and Switching of contexts should be done in a separate routine.
	  //  This should be done by a "OsMakeProcess" routine, so that
	  //  test0 runs on a process recognized by the operating system.


	mmio.Mode = Z502InitializeContext;
	mmio.Field1 = 0;
	mmio.Field2 = (long)test0;
	mmio.Field3 = (long)PageTable;

	MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence
	mmio.Mode = Z502StartContext;
	// Field1 contains the value of the context returned in the last call
	// Suspends this current thread
	//Message("before START_NEW_CONTEXT_AND_SUSPEND\n");
	mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
	//Message("Outside START_NEW_CONTEXT_AND_SUSPEND\n");
	//printf("calling MEM_WRITE\n");
	//printf("%d %x %d", &mmio, &mmio, Z502Context);
	MEM_WRITE(Z502Context, &mmio);     // Start up the context
	//Message("after MEM_WRITE\n");
	//printf("osInit completed successfully\n");

}                                               // End of osInit
