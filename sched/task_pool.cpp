#include "task_pool.hpp"
#include "../include/debug_rp.hpp"

int TaskPool::addTask(taskStruct *task, TaskPriority loadType)
{
    taskPoolNode *node, *tailNode;
    if(taskPoolCount >= MAX_TASK_POOL_SIZE)
    {
        DEBUG_MSG(__func__,"Task Queue is full");
        return -1;
    }
    sem_wait(&taskPoolLock);
    if(headNode == NULL){
        headNode = new taskPoolNode();
        headNode->taskItem = task;
        headNode->taskType = loadType;
    } else {
        node = headNode;
        while(node){
            if(node->next == NULL){
                tailNode = new taskPoolNode();
                tailNode->taskItem = task;
                tailNode->taskType = loadType;
                node->next = tailNode;
                taskPoolCount++;
                sem_post(&taskPoolLock);
                DEBUG_MSG(__func__, "New node added at tail: ", taskPoolCount);
                return 0;
            }
            node = node->next;
        }
    }
    sem_post(&taskPoolLock);
    DEBUG_ERR(__func__,"could not add task to tail of task pool");
    return -1;
}

TaskNodeExport TaskPool::popTask()
{
    taskPoolNode *task;
    if(taskPoolCount <= 0){
        DEBUG_MSG(__func__, "The task pool is empty");
        return TaskNodeExport(NULL);
    }
    sem_wait(&taskPoolLock);
    task = headNode;
    headNode = task->next;
    sem_post(&taskPoolLock);
    DEBUG_MSG(__func__, "Popped item, total remaining items in queue: ",taskPoolCount);
    return TaskNodeExport(task);
}