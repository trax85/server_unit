#include "worker_instance_list.hpp"
#include "../worker_node/worker_registry.hpp"
#include "../include/debug_rp.hpp"

void WorkerInstanceStruct::update(std::string instanceId)
{
    for(auto i = instanceIdList.begin(); i != instanceIdList.end(); i++){
        if(!instanceId.compare(*i)){
            instanceIdList.erase(i);
            return;
        }
    }
}

void WorkerInstanceList::updateInstanceList(std::list<InstanceStruct> instanceList)
{
    instanceIdList.clear();
    for(auto i = instanceList.begin(); i != instanceList.end(); i++){
        instanceIdList.push_back((*i).instanceName);
    }
}

void WorkerInstanceList::addWorker(std::string workerUid)
{
    Worker *worker = globalWorkerRegistry.getWorkerFromUid(workerUid);
    if(!worker){
        DEBUG_ERR(__func__, "cannot find worker with worker UID:", workerUid);
        return;
    }
    worker->resetWorkerReady();
    WorkerInstanceStruct workerInstanceStruct(workerUid, instanceIdList);
    workerList.push_back(workerInstanceStruct);
    DEBUG_MSG(__func__, "worker:", workerUid, " added to tracking list");
}

void WorkerInstanceList::updateWorker(std::string workerUid, std::string instanceId)
{
    for(auto i = workerList.begin(); i != workerList.end(); i++){
        if(!workerUid.compare((*i).getWorkerUid())){
            (*i).update(instanceId);
            // All instance packets were acknowledged
            if((*i).getWorkerInstanceListSize() == 0){
                Worker *worker = globalWorkerRegistry.getWorkerFromUid((*i).getWorkerUid());
                worker->setWorkerReady();
                workerList.erase(i);
                DEBUG_MSG(__func__, "worker:", workerUid, " removed from tracking list");
            }
            return;
        }
    }
}