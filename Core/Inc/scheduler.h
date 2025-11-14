/*
 * scheduler.h
 *
 *  Created on: Nov 8, 2025
 *      Author: DELL
 */

#ifndef INC_SCHEDULER_H_
#define INC_SCHEDULER_H_

#include <stdint.h>

#define SCH_TICK_MS 10

typedef void (*pFunction)(void);

typedef struct sTasks{
	void (*pTask)(void);//con tro ham
	uint32_t     Delay;
	uint32_t     Period;
	//uint32_t     RunMe;

	uint32_t     TaskID;

	struct sTasks* pNext;
	struct sTasks* pNextReady;

}sTasks;

#define SCH_MAX_TASKS    40

#define NO_TASK_ID      0xFFFFFFFF

void SCH_Init(void);

uint32_t SCH_Add_Task (void (*pFunction)(),
						 uint32_t DELAY,
						 uint32_t PERIOD);

void SCH_Update(void);

void SCH_Dispatch_Tasks(void);

//void SCH_Delete(uint32_t ID);
uint8_t SCH_Delete_Task(const uint32_t TASK_ID);

#endif /* INC_SCHEDULER_H_ */

