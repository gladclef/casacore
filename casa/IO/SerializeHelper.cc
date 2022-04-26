#include "SerializeHelper.h"

namespace casacore { //# NAMESPACE CASACORE - BEGIN

SerializeHelper::SerializeHelper(Int64 softLimit, MemoryIO &buf) : buf(buf), index(0), origin(NULL)
{
    int ll = SerializeHelper::getLogLevel();
    if (ll >= 3) std::cerr << "[sh] SerializeHelper" << std::endl;
    this->softLimit = softLimit;
}

SerializeHelper::~SerializeHelper()
{
    int ll = SerializeHelper::getLogLevel();
    if (ll >= 2) std::cerr << "[sh] ~SerializeHelper";
    if (origin == NULL) {
        std::cerr << " delete instances" << std::endl;
        for(auto iter = instances.begin(); iter != instances.end(); ++iter){
            delete iter->second;
        }
        instances.clear();
    } else {
        if (ll >= 2) std::cerr << std::endl;
    }
}

int SerializeHelper::getLogLevel()
{
    static int ret = -1;
    if (ret != -1) return ret;

    ret = 0;
    if (const char* use_small_summaryminor_p = std::getenv("SERIALIZEHELPER_LOGLEVEL"))
    {
        string use_small_summaryminor(use_small_summaryminor_p);
        if (use_small_summaryminor.compare("1") == 0) {
            ret = 1;
        }
        if (use_small_summaryminor.compare("2") == 0) {
            ret = 2;
        }
        if (use_small_summaryminor.compare("3") == 0) {
            ret = 3;
        }
    }
    
    return ret;
}

#include <iostream>

SerializeHelper *SerializeHelper::getInstance(void *object, Int64& idx, Int64& avail)
{
    int ll = SerializeHelper::getLogLevel();
    if (ll >= 3) std::cerr << "[sh] getInstance [" << object << "]" << std::endl;
    // Check that this is the origin object.
    if (origin != NULL) {
        throw AipsError("Can't get SerializeHelper instances from any but the origin instance!");
    }

    // Find the instance
    auto search = instances.find(object);
    if (search != instances.end()) {
        if (ll >= 2) std::cerr << "[sh]  > return SerializeHelper " << search->second << " for [" << object << "]" << std::endl;
        idx = search->second->index;
        avail = search->second->getAvailable();
        return search->second;
    }

    // Add a new instance
    SerializeHelper *newSH = new SerializeHelper(softLimit, buf);
    if (ll >= 2) std::cerr << "[sh]  > create new SerializeHelper " << newSH << " for [" << object << "]" << std::endl;
    newSH->origin = this;
    instances.insert({object, newSH});
    idx = newSH->index;
    avail = newSH->getAvailable();
    return newSH;
}

inline Int64 SerializeHelper::getAvailable()
{
    Int64 used = buf.length();
    Int64 avail = softLimit - used;
    return avail;
}

Int64 SerializeHelper::update(Int64 index)
{
    int ll = SerializeHelper::getLogLevel();
    if (ll >= 3) std::cerr << "[sh] update ";
    this->index = index;
    Int64 avail = getAvailable();
    if (ll >= 3) std::cerr << "(" << avail << ", " << this->index << ")" << std::endl;
    // if (avail < -softLimit) { // TODO remove
    //     throw AipsError("Exceed hard limit during serialization!");
    // }
    return avail;
}

void SerializeHelper::clearMemory()
{
    int ll = SerializeHelper::getLogLevel();
    if (ll >= 3) std::cerr << "[sh] clearMemory" << std::endl;
    buf.clear();
}

void SerializeHelper::objectSerialized(void *object)
{
    int ll = SerializeHelper::getLogLevel();
    if (ll >= 3) std::cerr << "[sh] objectSerialized [" << object << "]" << std::endl;
    // Check that this is the origin object.
    if (origin != NULL) {
        throw AipsError("Can't get SerializeHelper instances from any but the origin instance!");
    }

    // Remove the reference to the object, thereby
    // making it available for serialization again.
    // We do this so that an object that is referenced multiple
    // times by a containing object will be written to an output
    // stream multiple times.
    auto search = instances.find(object);
    if (search != instances.end()) {
        if (ll >= 3) std::cerr << "[sh]     deleting " << search->second << std::endl;
        delete search->second;
        instances.erase(search);
    } else {
        std::stringstream ss;
        ss << "Can't find object " << object << " in instances";
        throw AipsError(ss.str());
    }
}

/*-****************************************************************************
 * Static methods
 *****************************************************************************/

SerializeHelper *SerializeHelper::getInstance(SerializeHelper *parent, void *object, Int64& idx, Int64& avail)
{
    int ll = SerializeHelper::getLogLevel();
    if (ll >= 3) std::cerr << "[sh] static getInstance [" << object << "] (sh: " << parent << ")" << std::endl;
    if (parent == NULL) {
        idx = -1;
        avail = LLONG_MAX;
        return NULL;
    }
    if (object == NULL) {
        throw AipsError("Can't get instance for a NULL object");
    }
    if (parent->origin == NULL) {
        return parent->getInstance(object, idx, avail);
    }
    return parent->origin->getInstance(object, idx, avail);
}

Int64 SerializeHelper::getIndex(SerializeHelper *instance)
{
    int ll = SerializeHelper::getLogLevel();
    if (ll >= 3) std::cerr << "[sh] static getIndex (" << instance << ")" << std::endl;
    if (instance == NULL) return -1;
    return instance->index;
}

Int64 SerializeHelper::getAvailable(SerializeHelper *instance)
{
    int ll = SerializeHelper::getLogLevel();
    if (ll >= 3) std::cerr << "[sh] static getAvailable (" << instance << ")" << std::endl;
    if (instance == NULL) return LLONG_MAX;
    return instance->getAvailable();
}

Int64 SerializeHelper::update(SerializeHelper *instance, Int64 index)
{
    int ll = SerializeHelper::getLogLevel();
    if (ll >= 3) std::cerr << "[sh] static update" << std::endl;
    if (instance == NULL) return LLONG_MAX;
    return instance->update(index);
}

void SerializeHelper::objectSerialized(SerializeHelper *instance, void *object)
{
    int ll = SerializeHelper::getLogLevel();
    if (ll >= 3) std::cerr << "[sh] static objectSerialized [" << object << "] (sh: " << instance << ")" << std::endl;
    if (instance == NULL) return;
    if (object == NULL) throw AipsError("Can't objectSerialized serialization on a NULL object");
    if (instance->origin == NULL) {
        instance->objectSerialized(object);
        return;
    }
    instance->origin->objectSerialized(object);
}

} //# NAMESPACE CASACORE - END