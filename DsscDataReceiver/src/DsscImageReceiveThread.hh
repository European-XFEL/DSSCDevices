/* 
 * File:   DsscImageReceiveThread.h
 * Author: kirchgessner
 *
 * Created on May 17, 2016, 10:51 AM
 */

#ifndef DSSCIMAGERECEIVETHREAD_H
#define	DSSCIMAGERECEIVETHREAD_H

#include <karabo/xms/ImageData.hh>

#include <unordered_map>


#include "DsscHDF5Writer.h"
#include "DsscDataReceiveThread.h"

class DsscImageReceiveThread : public DsscDataReceiveThread {
public:
    DsscImageReceiveThread();
    DsscImageReceiveThread(int udpPort, uint32_t numFrames);
    
    using DimsVec = std::vector<unsigned long long>;
    
    karabo::util::Hash getSendData(){ return sendData;}
    
    virtual ~DsscImageReceiveThread();
    
    
    void updateNumFramesToReceive(uint32_t numFrames);
    
    void packData();
            
    THeader getTHeader(const karabo::util::Hash & data) const;
    TTrailer getTTrailer(const karabo::util::Hash & data) const;
    
    void fillData(const THeader & header);    
    void fillData(const TTrailer & trailer);
    
    DsscTrainData getTrainData();  
    static std::string getDir(const std::string & name);
    
private:
    void resizeImageData();
    
    karabo::util::Hash sendData;    
    karabo::xms::ImageData imageData;
    
    static const std::unordered_map<std::string,std::string> directoryStructure;
    
};

#endif	/* DSSCIMAGERECEIVETHREAD_H */

