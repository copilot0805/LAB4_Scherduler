/*
 * scheduler.c
 */

#include "scheduler.h"
#include "main.h"

// Mảng "ao" (pool)
static sTasks SCH_tasks_G[SCH_MAX_TASKS];
// Con trỏ đầu danh sách
static sTasks* g_TaskList_Head = 0; // NULL

void SCH_Init(void){
	g_TaskList_Head = 0;
	for (int i = 0; i < SCH_MAX_TASKS; i++){
		SCH_tasks_G[i].pTask = 0;
		SCH_tasks_G[i].pNext = 0;
        SCH_tasks_G[i].RunMe = 0; // <-- Thêm: Đảm bảo RunMe cũng được reset
	}
}

uint32_t SCH_Add_Task(pFunction pFunction, uint32_t DELAY, uint32_t PERIOD){

    // Tìm slot rảnh
	uint32_t task_index = 0;
	while (task_index < SCH_MAX_TASKS && SCH_tasks_G[task_index].pTask != 0){
		task_index++;
	}

	if(task_index == SCH_MAX_TASKS){
		return NO_TASK_ID;
	}

    // Chuyển đổi DELAY (ms) sang SỐ TICK
	uint32_t delay_in_ticks = DELAY / SCH_TICK_MS;

	sTasks* pNewTask = &SCH_tasks_G[task_index];
	pNewTask->pTask = pFunction;
	pNewTask->Period = PERIOD; // Lưu chu kỳ (bằng MS)
	pNewTask->RunMe = 0;
	pNewTask->pNext = 0;
    // pNewTask->Delay (bằng SỐ TICK) sẽ được gán bên dưới

	// Chèn vào danh sách (dùng SỐ TICK)

    // Danh sách rỗng
	if(g_TaskList_Head == 0){
		pNewTask->Delay = delay_in_ticks;
		g_TaskList_Head = pNewTask;
		return task_index;
	}

	// Chèn vào đầu
	if(delay_in_ticks < g_TaskList_Head->Delay){
		pNewTask->Delay = delay_in_ticks;
		g_TaskList_Head->Delay = g_TaskList_Head->Delay - delay_in_ticks;

		pNewTask->pNext = g_TaskList_Head;
		g_TaskList_Head = pNewTask;
		return task_index;
	}

	// Chèn vào giữa hoặc cuối
	sTasks* pCurrent = g_TaskList_Head;

	// Trừ delay của tác vụ đầu tiên khỏi SỐ TICK
	delay_in_ticks -= pCurrent->Delay;

	// Tìm vị trí chèn
	while(pCurrent->pNext != 0 && delay_in_ticks > pCurrent->pNext->Delay){
		delay_in_ticks -= pCurrent->pNext->Delay;
		pCurrent = pCurrent->pNext;
	}

	// Đã tìm thấy vị trí (sau pCurrent)
	pNewTask->Delay = delay_in_ticks;

	if(pCurrent->pNext != 0){
		pCurrent->pNext->Delay -= delay_in_ticks;
	}


	// Chèn node mới
	pNewTask->pNext = pCurrent->pNext;
	pCurrent->pNext = pNewTask;

	return task_index;
}


void SCH_Update(void){
	if(g_TaskList_Head != 0) {
        if (g_TaskList_Head->Delay == 0) {
            while (g_TaskList_Head != 0 && g_TaskList_Head->Delay == 0) {
                g_TaskList_Head->RunMe = 1;

                sTasks* pTaskToRun = g_TaskList_Head;
                g_TaskList_Head = g_TaskList_Head->pNext;
                pTaskToRun->pNext = 0;
            }
        }

        if (g_TaskList_Head != 0) {
            g_TaskList_Head->Delay--;
        }
	}
}

void SCH_Dispatch_Tasks(void){
	for(int i = 0; i < SCH_MAX_TASKS; i++){
		if(SCH_tasks_G[i].RunMe > 0){

            // Lấy thông tin TRƯỚC KHI xóa
            pFunction pTaskToRun = SCH_tasks_G[i].pTask;
            uint32_t period_in_ms = SCH_tasks_G[i].Period;

			// Chạy tác vụ
			(*pTaskToRun)();

			// Reset cờ và "trả" slot về ao
			SCH_tasks_G[i].RunMe = 0;
            SCH_tasks_G[i].pTask = 0;

			// Nếu là tác vụ định kỳ, thêm lại bằng MILI GIÂY
			if(period_in_ms > 0){
				SCH_Add_Task(pTaskToRun, period_in_ms, period_in_ms);
			}
		}
	}
}

uint8_t SCH_Delete_Task(const uint32_t TASK_ID){
	if(TASK_ID >= SCH_MAX_TASKS || SCH_tasks_G[TASK_ID].pTask == 0) {
		return 0;
	}

	sTasks* pTaskToDelete = &SCH_tasks_G[TASK_ID];

	// Neu tac vu o dau ds
	if(pTaskToDelete == g_TaskList_Head){
		// Cap nhat delay cua node tiep theo neu co
		if(pTaskToDelete->pNext != 0){
			pTaskToDelete->pNext->Delay += pTaskToDelete->Delay;
		}
		g_TaskList_Head = pTaskToDelete->pNext;
	}
	// Neu tac vu o giua/cuoi
	else{
		sTasks* pCurrent = g_TaskList_Head;
		while(pCurrent != 0 && pCurrent->pNext != pTaskToDelete){
			pCurrent = pCurrent->pNext;
		}

		if (pCurrent == 0){
			if(pTaskToDelete->RunMe == 0){
				pTaskToDelete->pTask = 0;
				return 1;
			}
			return 0;
		}

		pCurrent->pNext = pTaskToDelete->pNext;

		if(pTaskToDelete->pNext != 0){
			pTaskToDelete->pNext->Delay += pTaskToDelete->Delay;
		}
	}

	// Danh dau slot trong ao la ranh
	pTaskToDelete->pTask = 0;
	pTaskToDelete->pNext = 0;
	pTaskToDelete->RunMe = 0;

	return 1;
}
