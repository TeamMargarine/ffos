#include "FMHalSource.h"



#define LOG_TAG "FMHalSource"

#define LOG_DEBUG 0

#if LOG_DEBUG>=1
#define LOGD(...)  printf("FMSource: " __VA_ARGS__), printf("  \n")
#define LOGE(...) LOGD(__VA_ARGS__)
#endif


extern "C"
{
    #define FM_CARD_NAME "general-i2s"
    int get_snd_card_number(const char *card_name);
}


namespace android {

FMTrack::FMTrack(uint32_t bufferCount,uint32_t BufferUnitSize):mState(FMTrack::IDLE),overflow(1)
{
    ring= new RingBuffer(bufferCount,BufferUnitSize);
}

FMTrack::~FMTrack()
{
    delete ring;
}

uint8_t * FMTrack::GetBuf(uint32_t wait)
{
    return ring->GetBuf( wait);
}

void FMTrack::PutData(uint32_t wake)
{
    return ring->PutData( wake);
}

uint8_t *FMTrack::GetData(uint32_t wait)
{
    return ring->GetData( wait);
}


uint32_t FMTrack::GetDataCount()
{
    return ring->GetDataCount();
}

void FMTrack::PutBuf(uint32_t wake)
{
    return ring->PutBuf( wake);
}

uint32_t FMTrack::BufCount()
{
    return ring->BufCount( );
}

uint32_t FMTrack::BufUnitSize()
{
    return ring->BufUnitSize( );
}


int32_t   FMTrack:: SetState(int state)
{
    overflow = state;
    return 0;
}

int32_t   FMTrack:: GetState()
{
    return  overflow;
}

FMHalThread::FMHalThread(struct pcm *pcmHandle,uint32_t  readUnitSize):Thread(false),
pcmHandle(pcmHandle),readSize(readUnitSize),exit_var(0)
{
    readBuf=(uint8_t *)malloc(readSize);
}

FMHalThread::~FMHalThread()
{
    LOGD("~FMHalThread in");
    free(readBuf);
    readBuf=NULL;
     LOGD("~FMHalThread out");
}




int32_t   FMHalThread:: start( sp<FMTrack>  &track)
{
    int32_t  exist=false;
    int   need_signal=false;
    LOGD("FMHalThread: start in");
     sp <FMHalThread> strongMe = this;
     if(track == 0){
        LOGD("FMHalThread track is NULL");
        return -1;
     }
    AutoMutex lock(&mLock);
    for( size_t i=0;i<mTrack.size();i++) {
       if( mTrack[i] ==track ){
            exist=true;
            break;
       }
    }

    if(mTrack.size()==0){
        need_signal=true;
    }
    
    if( !exist){
        mTrack.push(track);
    }
    track->mState=FMTrack::ACTIVE;

    if(need_signal){
        mWaitCond.signal();
    }
    return 0;

}

int32_t   FMHalThread:: stop(sp<FMTrack>  &track)
{
    int32_t  exist=false;
    size_t i=0;
    sp <FMHalThread> strongMe = this;
    LOGD("FMHalThread: stop in");
    AutoMutex lock(&mLock);
    for(  i=0;i<mTrack.size();i++) {
        if( mTrack[i] == track ){
            exist=true;
            break;
        }
    }

    if( !exist){
        LOGE(" track to stop is not exist");
        return -1;
    }
    track->mState=FMTrack::IDLE;
    mTrack.removeAt(i);
    return 0;
}

 void FMHalThread::exit(){
    sp <FMHalThread> strongMe = this;
    LOGD("peter: FMHalThread exit in ");
    if(mTrack.size()==0){
        exit_var = 1;
        requestExit();
        mWaitCond.signal();
        mLock.unlock();
        LOGD("peter: FMHalThread exit in wait");
        requestExitAndWait();
        exit_var = 0;
        mLock.lock();
        LOGD("peter: FMHalThread exit in wait out");
    }
}
 void FMHalThread::onFirstRef()
{
    const size_t SIZE = 256;
    char buffer[SIZE];

    snprintf(buffer, SIZE, "FMHalThread Thread %p", this);
    run(buffer, PRIORITY_HIGHEST);
}


 bool        FMHalThread:: threadLoop()
{
    int reval=0;
    sp <FMHalThread> strongMe = this;
    uint8_t * trackBuf=NULL;
    LOGD("threadLoop start");
    while (!exitPending()) {
            mLock.lock();
            if(mTrack.size()==0){
                LOGD("threadLoop in wait,refcount is %d ",this->getStrongCount());
                if(exit_var) {
                    mLock.unlock();
                    break;
                }
                mWaitCond.wait(mLock);
                LOGD("threadLoop in wait out");
            }
            mLock.unlock();
            LOGD("pcm_read threadloop in");
            reval=pcm_read(pcmHandle,readBuf,readSize);
            LOGD("pcm_read threadloop out result is %d",reval);
            if(reval) {
                LOGD("pcm_read error %d",reval);
                memset(readBuf,0,readSize);
                usleep(50);
            }
            mLock.lock();
            for(size_t i=0;i<mTrack.size();i++) {
                sp<FMTrack>   track  = mTrack[i];
                if(track != 0){
                    if(track->mState==FMTrack::ACTIVE) {
                    if(track->GetState()) {
                        trackBuf= track->GetBuf(false);
                        if(trackBuf){
                            memcpy(trackBuf,readBuf,readSize);
                            track->PutData(true);
                        }
                        else {
                            track->SetState(0);
                            LOGE("peter: no buffer for track %d and drop the pcm data first time",i);
                        }
                    }
                    else {
                        if(track->GetDataCount() <= (track->BufCount()*track->BufUnitSize()/2)) {
                            track->SetState(1);
                            LOGE("peter: no buffer for track %d no data and recover ok",i);
                        }
                        else
                            LOGE("peter: no buffer for track %d and drop the pcm data",i);
                    }
                    }
                }
            }    
            mLock.unlock();
    }
    LOGD("thread loop exit refcount is %d",this->getStrongCount());
    return false;
}

sp<FMHalThread>  FMHalSource::FMThread=NULL;
 Mutex            FMHalSource::mLock;
struct pcm * FMHalSource::pcmHandle=NULL;
uint32_t  FMHalSource::openCount=0;

 int FMHalSource::mChannel=1;
 int FMHalSource::mSamplerate=32000;
 int FMHalSource::mPeriodSize=160;
 int FMHalSource::mPeriodCount=10;

#define FM_PEROID_SIZE          (160*4*2)
#define FM_PERIOD_COUNT   8


FMHalSource::FMHalSource(int samplerate,int channel):ply_len(0),ply_buf(NULL),mTrack(0),state(IDLE),is_ready(false)
{
        int unitBytes=0;
       struct pcm_config cfg={0};
       cfg.channels=channel;
       cfg.period_count=FM_PERIOD_COUNT;
       cfg.period_size=FM_PEROID_SIZE;
       cfg.rate=samplerate;
       cfg.format =  PCM_FORMAT_S16_LE;
        LOGD("FMHalSource construct in samplerate is %d, channel is %d",samplerate,channel);
        if((channel!=2) ||(samplerate != 32000)) {
            cfg.rate=32000;
            cfg.channels=2;
        }
        AutoMutex lock(&mLock);
        if(!pcmHandle) {           
            int card=get_snd_card_number((const char *)FM_CARD_NAME);
            if(card < 0){
                LOGE("can not find the fm card");
                return;
            }
            pcmHandle = pcm_open(card, 1, PCM_IN , &cfg);
             if (!pcm_is_ready(pcmHandle)) {
                LOGE("FM  cannot open pcm_in driver: %s", pcm_get_error(pcmHandle));
                pcm_close(pcmHandle);
                pcmHandle = NULL;
            }          
            mPeriodSize=FM_PEROID_SIZE;
            mPeriodCount=FM_PERIOD_COUNT;
            mChannel= channel;
            mSamplerate = samplerate;          
        }
        else{
            if(  (mChannel != channel) ||
                  (mSamplerate !=  samplerate)) {
                  //todo
            }
        }
        unitBytes= pcm_frames_to_bytes(pcmHandle, mPeriodSize);
         mTrack = new FMTrack(mPeriodCount*4,unitBytes);
         
        if(FMThread == NULL) {
            FMThread = new FMHalThread(pcmHandle,  unitBytes);
        }
        openCount++;
}


FMHalSource:: ~FMHalSource()
{
    int result=-1;
    LOGD("~FMHalSource in openCount is %d",openCount);
    AutoMutex lock(&mLock);
    openCount--;
    if(!openCount) {
        if(state == START) {
            result= FMThread->stop(mTrack);
        }
        FMThread->exit();
        FMThread=NULL;        
        if(pcmHandle) {
            pcm_close(pcmHandle);
            pcmHandle = NULL;
        }             
    }
    mTrack=NULL;
    LOGD("~FMHalSource out");
}



int32_t FMHalSource::start(void)
{
    int unitBytes=0;
    int result=-1;
    sp <FMHalSource> strongMe = this;
    LOGD("start start in");
    AutoMutex lock(&mLock);
    if(FMThread != 0) {
        AutoMutex lock(&mLock_l);
        result= FMThread->start(mTrack);
        if(!result){
            state=START;
        }   
    }
    return result;
}



int32_t FMHalSource::stop(void)
{
        int result=-1;
        LOGD("start stop in");
        sp <FMHalSource> strongMe = this;
        AutoMutex lock(&mLock);
        if(FMThread != 0) {
             AutoMutex lock(&mLock_l);
             result= FMThread->stop(mTrack);
             if(!result){
                if(ply_buf){
                    mTrack->PutBuf(false);
                    ply_buf=NULL;
                }
                ply_len=0;   
                state = STOP;
             }
         }
        return result;
}




int32_t FMHalSource::read(void *  buf, uint32_t  bytes)
{
    size_t  left=bytes;  
    uint8_t * cur_ptr=(uint8_t *)buf;
    uint32_t copy_count=0;
     sp <FMHalSource> strongMe = this;
 //  LOGD("peter: read in bytes %d, buffered bytes is %d",bytes,mTrack->GetDataCount());
    AutoMutex lock(&mLock_l);
    if(state !=START){
	memset(buf,0,bytes);
        return bytes;
    }

    if(!is_ready){
	if(mTrack->GetDataCount() < (mTrack->BufCount()*mTrack->BufUnitSize()/2)) {
	    memset(buf,0,bytes);
	    LOGE("peter: fmhalsource read in not ready fill in zero data");
	    //usleep(20);
	    return bytes;
	}
	else{
	    is_ready=true;
	    LOGE("peter: fmhalsource is ready");
	}
    }

    while(left){
        if(!ply_len){
            if(ply_buf) {
                mTrack->PutBuf(true);
                ply_buf=NULL;
            }
           ply_buf= mTrack->GetData(false);
           if(!ply_buf){
                break;
           }
           ply_len=mTrack->BufUnitSize();
        }     
        if(ply_len&&ply_buf){
            copy_count= ply_len>left?left:ply_len;
            memcpy(cur_ptr,ply_buf,copy_count);
            cur_ptr+=copy_count;
            ply_len-=copy_count;
            ply_buf +=copy_count;
            left-=copy_count;
        }
    }

    if(left>4){
	int i=0;
	int16_t *buf_left=(int16_t *)buf+(bytes-left)/2;
	for(i=0;i<left/4;i++) {
	    buf_left[2*i]=last_l;
	    buf_left[2*i+1]=last_r;
	}
	is_ready = false;
	LOGE("peter: error, no data to read and fill with 0");
    }
    if(bytes > 4) {
	last_r = *((int16_t *)buf+bytes/2-1);
	last_l = *((int16_t *)buf+bytes/2 -2);
    }

    //LOGD("peter: read out,bytes read %d",(bytes-left));
    return bytes;
}





}

