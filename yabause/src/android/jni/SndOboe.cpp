#include "SndOboe.h"

#include <oboe/Oboe.h>
#include <math.h>


using namespace oboe;

class SndOboeEngine: public oboe::AudioStreamDataCallback {
private:
    std::mutex         mLock;
    std::shared_ptr<oboe::AudioStream> mStream;

  const int NUMSOUNDBLOCKS =4;
  const int blockSize = 2940;

  u16 *stereodata16;
  u32 soundoffset;
  volatile u32 soundpos;
  u32 soundlen;
  u32 soundbufsize;
  u8 soundvolume;
  int muted = 0;    

  // Stream params
  static int constexpr kChannelCount = 2;
  static int constexpr kSampleRate = 44100;

public:
  

  static SndOboeEngine * instance_;
  static SndOboeEngine * getInstance(){
    if( instance_==NULL ) instance_ = new SndOboeEngine();
    return instance_; 
  }


    virtual ~SndOboeEngine() = default;

    int32_t startAudio() {
        std::lock_guard<std::mutex> lock(mLock);

        soundbufsize = blockSize * NUMSOUNDBLOCKS * 2 * 2;        
        stereodata16 = new u16[soundbufsize];
        memset( stereodata16, 0, sizeof(s16) * soundbufsize );
        soundpos = 0;

        oboe::AudioStreamBuilder builder;
        // The builder set methods can be chained for convenience.
        Result result = builder.setSharingMode(oboe::SharingMode::Exclusive)
                ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
                ->setChannelCount(kChannelCount)
                ->setSampleRate(kSampleRate)
		            ->setSampleRateConversionQuality(oboe::SampleRateConversionQuality::Medium)
                ->setFormat(oboe::AudioFormat::I16)
                ->setDataCallback(this)
                ->openStream(mStream);

	      if (result != Result::OK) return (int32_t) result;

        mStream->requestStart();

	      return (int32_t) result;
    }
   
    // Call this from Activity onPause()
    void stopAudio() {
        // Stop, close and delete in case not already closed.
        std::lock_guard<std::mutex> lock(mLock);

        delete [] stereodata16;

        if (mStream) {
            mStream->stop();
            mStream->close();
            mStream.reset();
        }
    }

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) override {

        std::lock_guard<std::mutex> lock(mLock);

        int i;
        u8* soundbuf = (u8*)stereodata16;
        u8 *stream = (u8 *)audioData;
        int len = numFrames * 2 * 2;

        // original code
        for (i = 0; i < len; i++)
        {
          if (soundpos >= soundbufsize)
            soundpos = 0;
          stream[i] = muted ? 0 : soundbuf[soundpos];
          soundpos++;
        }
 
        return oboe::DataCallbackResult::Continue;
    }


  void convert32uto16s(s32 *srcL, s32 *srcR, s16 *dst, u32 len) {
    u32 i;

    for (i = 0; i < len; i++)
    {
        // Left Channel
        if (*srcL > 0x7FFF) *dst = 0x7FFF;
        else if (*srcL < -0x8000) *dst = -0x8000;
        else *dst = *srcL;
        srcL++;
        dst++;

        // Right Channel
        if (*srcR > 0x7FFF) *dst = 0x7FFF;
        else if (*srcR < -0x8000) *dst = -0x8000;
        else *dst = *srcR;
        srcR++;
        dst++;
   }
   
  }

public: 
  void onUpdateAudio(u32 *leftchanbuffer, u32 *rightchanbuffer, u32 num_samples) {
    u32 copy1size=0, copy2size=0;
    int nextpos;

    std::lock_guard<std::mutex> lock(mLock);


   if ((soundbufsize - soundoffset) < (num_samples * sizeof(s16) * 2))
   {
      copy1size = (soundbufsize - soundoffset);
      copy2size = (num_samples * sizeof(s16) * 2) - copy1size;
   }
   else
   {
      copy1size = (num_samples * sizeof(s16) * 2);
      copy2size = 0;
   }

   convert32uto16s((s32 *)leftchanbuffer, (s32 *)rightchanbuffer, 
   (s16 *)(((u8 *)stereodata16)+soundoffset), copy1size / sizeof(s16) / 2);

   if (copy2size)
      convert32uto16s((s32 *)leftchanbuffer + (copy1size / sizeof(s16) / 2), 
      (s32 *)rightchanbuffer + (copy1size / sizeof(s16) / 2), (s16 *)stereodata16,
       copy2size / sizeof(s16) / 2);

   soundoffset += copy1size + copy2size;   
   soundoffset %= soundbufsize;


  }

  int getAudioSpace(){

    u32 freespace=0;

    if (soundoffset > soundpos)
      freespace = soundbufsize - soundoffset + soundpos;
    else
      freespace = soundpos - soundoffset;

    return (freespace / sizeof(s16) / 2);
  }

  void mute(){
    muted = 1;
  }

  void unmute(){
    muted = 0;
  }

  void setVolume( int vol ){
    if( vol == 0 ){
      muted = 1;
    }else{
      muted = 0;
    }
  }

};

SndOboeEngine * SndOboeEngine::instance_ = NULL;

extern "C" {

int SNDOboeInit(void);
void SNDOboeDeInit(void);
int SNDOboeReset(void);
int SNDOboeChangeVideoFormat(int vertfreq);
void SNDOboeUpdateAudio(u32 *leftchanbuffer, u32 *rightchanbuffer, u32 num_samples);
u32 SNDOboeGetAudioSpace(void);
void SNDOboeMuteAudio(void);
void SNDOboeUnMuteAudio(void);
void SNDOboeSetVolume(int volume);

SoundInterface_struct SNDOboe = {
SNDCORE_OBOE,
"Oboe Sound Interface",
SNDOboeInit,
SNDOboeDeInit,
SNDOboeReset,
SNDOboeChangeVideoFormat,
SNDOboeUpdateAudio,
SNDOboeGetAudioSpace,
SNDOboeMuteAudio,
SNDOboeUnMuteAudio,
SNDOboeSetVolume
};

int SNDOboeInit(void){

  SndOboeEngine * i = SndOboeEngine::getInstance();
  i->startAudio();

  return 0;
}

void SNDOboeDeInit(void){

  SndOboeEngine * i = SndOboeEngine::getInstance();
  i->stopAudio();

  return;
}

int SNDOboeReset(void){
  return 0;
}

int SNDOboeChangeVideoFormat(int vertfreq){
  return 0;
}

void SNDOboeUpdateAudio(u32 *leftchanbuffer, u32 *rightchanbuffer, u32 num_samples){

  SndOboeEngine * i = SndOboeEngine::getInstance();
  i->onUpdateAudio(leftchanbuffer,rightchanbuffer,num_samples);
  return;

}

u32 SNDOboeGetAudioSpace(void){

  SndOboeEngine * i = SndOboeEngine::getInstance();
  return i->getAudioSpace();
}

void SNDOboeMuteAudio(void){
  SndOboeEngine * i = SndOboeEngine::getInstance();
  return i->mute();

}

void SNDOboeUnMuteAudio(void){
  SndOboeEngine * i = SndOboeEngine::getInstance();
  return i->unmute();

}

void SNDOboeSetVolume(int volume){
  SndOboeEngine * i = SndOboeEngine::getInstance();
  return i->setVolume(volume);
}


}
