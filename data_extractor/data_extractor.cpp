#include <fstream>
#include <sstream>
#include "../sink/sink_stack.hpp"
#include "../include/debug_rp.hpp"
#include "../services/sql_access.hpp"
#include "../sender_unit/instance.hpp"
#include "../sender_unit/user_data.hpp"
#include "../configs.h"
#include "data_extractor.hpp"

TaskPriority getTaskPriority(int priority)
{
    switch(priority){
        case 0: return HIGH_PRIORITY;
        case 1: return MEDIUM_PRIORITY;
        case 2:
        default:  return LOW_PRIORITY;
    }
}

std::string* DataExtractor::getFileData(std::string fileName, bool isInstance)
{
    std::string *resultData, extractedData;
    std::ostringstream buf; 
    std::fstream dataFile;
    if(isInstance)
        dataFile.open(INSTANCE_FILE_DATA_DIR + "" + fileName, std::ios::in);
    else
        dataFile.open(USER_FILE_DATA_DIR + "" + fileName, std::ios::in);
    if(dataFile){
        buf << dataFile.rdbuf();
        resultData = new std::string;
        *resultData = buf.str();
        DEBUG_MSG(__func__, "Data extracted form file was: ", buf.str());
        return resultData;
    }
    DEBUG_ERR(__func__, "No data was found in ", fileName);
    return NULL;
}

int DataExtractor::executeInstanceExtractor(std::list<std::string> idList, TaskPriority priority)
{
    InstanceStruct *instanceStruct = NULL;
    std::list<InstanceStruct*> instanceList;
    int j = 0;
    for(auto i = idList.begin(); i != idList.end(); i++,j++){
        std::string curInstanceName = *i, *resultData;
        if(!curInstanceName.empty()) {
            DEBUG_MSG(__func__, "instance id: ", j, " is empty");
            continue;
        }
        std::string algoTypeQuery = "SELECT " + INSTANCE_PRIO_COL_ID + " FROM " + INSTANCE_TABLE_NAME + " WHERE "
                        + INSTANCE_DAT_COL_ID + "="+ curInstanceName +";";
        std::uint8_t algoType = globalSqlAccess.sqlQueryDbGetInt(algoTypeQuery, INSTANCE_PRIO_COL_ID);
        instanceStruct = new InstanceStruct(curInstanceName, algoType, resultData);
        instanceList.push_back(instanceStruct);
    }

    return globalInstanceRegistery.update(instanceList);
}

int DataExtractor::executeUserTableExtractor(std::list<std::string> userTableNameList)
{
    UserDataTable *userTable = NULL;
    std::string curUserTableName;
    std::string tablePriorityQuery;
    int userTablePriority;
    std::string tableAlgoIdQuery, userTableAlgo;
    std::string *fileData;

    for(auto i = userTableNameList.begin(); i != userTableNameList.end(); i++){
        std::string userTableName = *i;
        if((*i).empty()){
            DEBUG_ERR(__func__, "Table fetch failed");
            continue;
        }
        
        tablePriorityQuery = "SELECT " + USERDAT_DAT_PRIORITY_COL_ID + " FROM " + USERDAT_TABLE_NAME + " WHERE " 
        + USERDAT_DAT_COL_ID + " '" + userTableName + "';";
        tableAlgoIdQuery = "SELECT " + USERDAT_ALGO_COL_ID + " FROM " + USERDAT_TABLE_NAME + " WHERE " 
        + USERDAT_DAT_COL_ID + " '" + userTableName + "';";
        
        userTablePriority = globalSqlAccess.sqlQueryDbGetInt(tablePriorityQuery);
        userTableAlgo = globalSqlAccess.sqlQueryDb(tablePriorityQuery);

        fileData = getFileData(curUserTableName, false);
        userTable = new UserDataTable(userTableName, getTaskPriority(userTablePriority), userTableAlgo, fileData);
        globalSenderSink.pushObject(userTable, getTaskPriority(userTablePriority));
    }
    DEBUG_MSG(__func__, "pushed user tables from DB to sender stack");
    return 0;
}