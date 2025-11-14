/*
 * scheduler.c
 */
#include "scheduler.h"
#include <stddef.h>

static sTasks SCH_tasks_G[SCH_MAX_TASKS];
static sTasks* g_TaskList_Head = NULL;
static sTasks* g_ReadyQueue_Head = NULL;
static sTasks* g_ReadyQueue_Tail = NULL;

static uint32_t g_current_TaskID_counter = 0;

static uint32_t Get_New_Task_ID(void){
	g_current_TaskID_counter++;
	if(g_current_TaskID_counter == NO_TASK_ID){
		// Bo qua gia tri NO_TASK_ID
		g_current_TaskID_counter++;
	}
	return g_current_TaskID_counter;
}

void SCH_Init(void){
	g_TaskList_Head = NULL;
	g_ReadyQueue_Head = NULL;
	g_ReadyQueue_Tail = NULL;
    g_current_TaskID_counter = 0; // Reset bo dem

	for (int i = 0; i < SCH_MAX_TASKS; i++){
		SCH_tasks_G[i].pTask = NULL;
		SCH_tasks_G[i].pNext = NULL;
		SCH_tasks_G[i].pNextReady = NULL;
		SCH_tasks_G[i].TaskID = NO_TASK_ID; // Danh dau ID la khong hop le
	}
}


// Ham nay tim slot (index) va gan ID cho task
static uint32_t SCH_Add_Task_Internal(pFunction pFunction, uint32_t DELAY,
                                      uint32_t PERIOD, uint32_t TaskID_To_Use)
{
    //Tim slot ranh
	uint32_t task_index = 0;
	while (task_index < SCH_MAX_TASKS && SCH_tasks_G[task_index].pTask != NULL){
		task_index++;
	}
	if(task_index == SCH_MAX_TASKS){
		return NO_TASK_ID; // Het slot ranh
	}

	uint32_t delay_in_ticks = DELAY / SCH_TICK_MS;
	sTasks* pNewTask = &SCH_tasks_G[task_index];

	pNewTask->pTask = pFunction;
	pNewTask->Period = PERIOD;
	pNewTask->pNext = NULL;
	pNewTask->pNextReady = NULL;
    pNewTask->TaskID = TaskID_To_Use; // gan ID duoc truyen vao

	// Chen vao danh sach lien ket cho
    if(g_TaskList_Head == NULL){
		pNewTask->Delay = delay_in_ticks;
		g_TaskList_Head = pNewTask;
		return pNewTask->TaskID;
	}

	if(delay_in_ticks < g_TaskList_Head->Delay){
		pNewTask->Delay = delay_in_ticks;
		g_TaskList_Head->Delay -= delay_in_ticks;
		pNewTask->pNext = g_TaskList_Head;
		g_TaskList_Head = pNewTask;
		return pNewTask->TaskID;
	}

	sTasks* pCurrent = g_TaskList_Head;
	delay_in_ticks -= pCurrent->Delay;

	while(pCurrent->pNext != NULL && delay_in_ticks > pCurrent->pNext->Delay){
		delay_in_ticks -= pCurrent->pNext->Delay;
		pCurrent = pCurrent->pNext;
	}

	pNewTask->Delay = delay_in_ticks;
	if(pCurrent->pNext != 0){
		pCurrent->pNext->Delay -= delay_in_ticks;
	}

	pNewTask->pNext = pCurrent->pNext;
	pCurrent->pNext = pNewTask;

	return pNewTask->TaskID; // Tra ve ID
}

// Ham nay tao ID moi va goi ham noi bo
uint32_t SCH_Add_Task(pFunction pFunction, uint32_t DELAY, uint32_t PERIOD){
    uint32_t newID = Get_New_Task_ID();
    return SCH_Add_Task_Internal(pFunction, DELAY, PERIOD, newID);
}


void SCH_Update(void){
	if(g_TaskList_Head != NULL) {
        if (g_TaskList_Head->Delay == 0) {
            while (g_TaskList_Head != NULL && g_TaskList_Head->Delay == 0) {
                sTasks* pTaskToMove = g_TaskList_Head;
                g_TaskList_Head = pTaskToMove->pNext;
                pTaskToMove->pNext = NULL;

                pTaskToMove->pNextReady = NULL;
                if (g_ReadyQueue_Head == NULL) {
                    g_ReadyQueue_Head = pTaskToMove;
                    g_ReadyQueue_Tail = pTaskToMove;
                } else {
                    g_ReadyQueue_Tail->pNextReady = pTaskToMove;
                    g_ReadyQueue_Tail = pTaskToMove;
                }
            }
        }

        if (g_TaskList_Head != NULL) {
            g_TaskList_Head->Delay--;
        }
	}
}

// Phai giu lai ID cu khi them lai
void SCH_Dispatch_Tasks(void){

    if (g_ReadyQueue_Head != NULL) {

        sTasks* pTaskToRun = g_ReadyQueue_Head;

        g_ReadyQueue_Head = pTaskToRun->pNextReady;
        if (g_ReadyQueue_Head == NULL) {
            g_ReadyQueue_Tail = NULL;
        }
		pTaskToRun->pNextReady = NULL;

        // lay thong tin truoc khi tra ve ao
        pFunction pTaskFunc = pTaskToRun->pTask;
        uint32_t period_ms = pTaskToRun->Period;
        uint32_t task_id_to_keep = pTaskToRun->TaskID; // lay ID cu

        // tra ve ao ngay lap tuc
        // (De phong pTaskFunc goi SCH_Add_Task va lay mat slot nay)
        pTaskToRun->pTask = NULL;
        pTaskToRun->TaskID = NO_TASK_ID;

        // Chay tac vu
        (*pTaskFunc)();

        // Xu ly sau khi chay
        if (period_ms > 0) {
            // Them lai tac vu va GIU LAI ID CU
            SCH_Add_Task_Internal(pTaskFunc, period_ms, period_ms, task_id_to_keep);
        }
    }
}

// Bay gio phai tim bang TaskID, KHONG PHAI index
uint8_t SCH_Delete_Task(const uint32_t TASK_ID){
	if(TASK_ID == NO_TASK_ID) return 0;

    // --- Tim kiem 1: Trong danh sach cho
    sTasks* pCurrent = g_TaskList_Head;
    sTasks* pPrev = NULL;
    while(pCurrent != NULL && pCurrent->TaskID != TASK_ID) {
        pPrev = pCurrent;
        pCurrent = pCurrent->pNext;
    }

    if (pCurrent != NULL) { // Tim thay
        if (pPrev == NULL) { // Neu la Head
            g_TaskList_Head = pCurrent->pNext;
        } else { // Neu la o giua/cuoi
            pPrev->pNext = pCurrent->pNext;
        }
        // Don delay lai
        if (pCurrent->pNext != NULL) {
            pCurrent->pNext->Delay += pCurrent->Delay;
        }

        // tra ve ao
        pCurrent->pTask = NULL;
        pCurrent->pNext = NULL;
        pCurrent->TaskID = NO_TASK_ID;
        return 1;
    }

    // --- Tim kiem 2: Neu khong thay, tim trong hang doi san sang
    pCurrent = g_ReadyQueue_Head;
    pPrev = NULL;
    while(pCurrent != NULL && pCurrent->TaskID != TASK_ID) {
        pPrev = pCurrent;
        pCurrent = pCurrent->pNextReady;
    }

    if (pCurrent != NULL) { // Tim thay
        if (pPrev == NULL) { // Neu la Head
            g_ReadyQueue_Head = pCurrent->pNextReady;
        } else { // Neu la o giua/cuoi
            pPrev->pNextReady = pCurrent->pNextReady;
        }
        // Cap nhat Tail neu xoa o duoi
        if (pCurrent == g_ReadyQueue_Tail) {
            g_ReadyQueue_Tail = pPrev;
        }

        // tra ve ao
        pCurrent->pTask = NULL;
        pCurrent->pNextReady = NULL;
        pCurrent->TaskID = NO_TASK_ID;
        return 1;
    }

	return 0; // Khong tim thay tac vu
}
