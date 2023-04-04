#include <list>
#include "../include/process.hpp"
#include "../include/debug_rp.hpp"
#include "../sched/task_pool.hpp"
#include "../sink/sink_stack.hpp"
#include "../lib/nlohmann/json-schema.hpp"
#include "../worker_node/worker_registry.hpp"
#include "../data_extractor/data_extractor.hpp"
#include "../packet_processor/out_data_registry.hpp"
#include "../packet_processor/packet_constructor.hpp"
#include "sender_core.hpp"
#include "instance.hpp"
#include "user_data.hpp"

using nlohmann::json_schema::json_validator;
using json = nlohmann::json;

SenderCoreData::SenderCoreData()
{
    newWorkerList = new std::list<std::uint64_t>;
    pendingPacketsList = new std::list<OutPacket*>;
    DEBUG_MSG(__func__, "inited sender core data");
}

void SenderCoreData::addWorker(std::uint64_t workerUid)
{
    newWorkerList->push_back(workerUid);
}

void SenderCoreData::addPackets(OutPacket *outData)
{
    pendingPacketsList->push_back(outData);
}

bool SenderCoreData::isNewWorkerListEmpty()
{
    if(newWorkerList->size() > 0)
        return false;
    return true;
}

bool SenderCoreData::isPendingPacketsListEmpty()
{
    if(pendingPacketsList->size() > 0)
        return false;
    return true;
}

std::list<std::uint64_t>* SenderCoreData::getWorkerList()
{
    return newWorkerList;
}

std::list<OutPacket*>* SenderCoreData::getPendingPacketsList()
{
    return pendingPacketsList;
}

Worker* findIdealWorker()
{
    std::list<Worker*> workerList = globalWorkerRegistry.getWorkerList();
    Worker *worker, *resWorker;
    int hSize = 0;
    std::uint64_t hWorkerUid = 0;

    for(auto i = workerList.begin(); i != workerList.end(); i++)
    {
        worker = *i;
        if(worker == NULL){
            DEBUG_ERR(__func__, "empty worker node");
            continue;
        }
        if(worker->getQueueSize() > hSize){
            hSize = worker->getQueueSize();
            hWorkerUid = worker->getWorkerUID();
        }
    }

    if(!hWorkerUid){
        DEBUG_ERR(__func__, "all workers are full");
        return NULL;
    }

    resWorker = globalWorkerRegistry.getWorkerFromUid(hWorkerUid);
    DEBUG_MSG(__func__, "worker with emptiest queue: ", hWorkerUid);

    return resWorker;
}

void pushUserDataToWorkerQueue()
{
    Worker *worker;
    ExportSinkItem sinkItem;
    UserDataTable* userData;
    json packet;

    worker = findIdealWorker();
    if(worker == NULL){
        return;
    }

    sinkItem = globalSenderSink.popObject();
    userData = (UserDataTable*)sinkItem.dataObject;
    packet = userData->toJson();
    worker->queuePacket(
        new OutPacket(
            PacketConstructor().create(SP_DATA_SENT, worker->getWorkerUID(), packet)
            , userData->userTable, true)
        );
    globalOutDataRegistry.assignWorker(userData->userTable, worker);
    globalOutDataRegistry.updateTaskStatus(userData->userTable, DATA_READY);
    DEBUG_MSG(__func__, "sender sink packet pushed to worker:", worker->getWorkerUID());
}

void pushInstanceToWorkerQueue(std::list<std::uint64_t> *workerList)
{
    std::uint64_t workerUID;
    Worker* worker;
    std::string tableId;
    std::list<json>* instanceJson = globalInstanceRegistery.toJson();

    for(auto i = workerList->begin(); i != workerList->end(); i++)
    {
        workerUID = *i;
        worker = globalWorkerRegistry.getWorkerFromUid(workerUID);
        if(worker == NULL){
            DEBUG_ERR(__func__, "could not find worker");
            continue;
        }
        for(auto j = instanceJson->begin(); j != instanceJson->end(); j++)
        {
            tableId = (*j)["body"]["instanceid"];
            globalOutDataRegistry.addTable(tableId, worker);
            worker->queuePacket(
                new OutPacket(
                    PacketConstructor().create(SP_DATA_SENT, worker->getWorkerUID(), *j)
                    , tableId, true)
                );
            globalOutDataRegistry.updateTaskStatus(tableId, DATA_READY);
        }
        DEBUG_MSG(__func__, "queued packet into worker UID: ", workerUID);
        workerList->pop_front();
    }
}

int pushPendingPackets(std::list<OutPacket*>* outPacketList)
{
    Worker *worker = findIdealWorker();
    
    while(outPacketList->size() > 0){
        worker->queuePacket(outPacketList->front());
        outPacketList->pop_front();
        worker = findIdealWorker();
        if(!worker){
            return -1;
        }
    }

    return 0;
}

int startSenderCore(void *data)
{
    int rc = 0;
    SenderCoreData *senderData = (SenderCoreData*)data;

    if(!senderData->isPendingPacketsListEmpty()){
        rc = pushPendingPackets(senderData->getPendingPacketsList());
        if(rc){
            DEBUG_MSG(__func__, "all worker queues are filled waiting to free-up or no worker present");
            return JOB_PENDING;
        }
        DEBUG_MSG(__func__, "pending packets pushed to queue");
    }

    if(!senderData->isNewWorkerListEmpty()){
        pushInstanceToWorkerQueue(senderData->getWorkerList());
        DEBUG_MSG(__func__, "cleared pending workerList");
    } else {
        pushUserDataToWorkerQueue();
        DEBUG_MSG(__func__, "pushed data into queue");
    }

    return JOB_PENDING;
}
int pauseSenderCore(void *data)
{
    return JOB_PENDING;
}
int endSenderCore(void *data)
{
    DEBUG_MSG(__func__, "sender instance is shutting down");
    return JOB_DONE;
}

struct process *senderCore = new process{
    .start_proc = startSenderCore,
    .pause_proc = pauseSenderCore,
    .end_proc = endSenderCore
};

int SenderCore::run()
{
    globalTaskPool.addTask(new taskStruct(senderCore, senderCoreData), MEDIUM_PRIORITY);
    return 0;
}