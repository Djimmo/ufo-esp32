#ifndef MAIN_DISPLAYCHARTER_H_
#define MAIN_DISPLAYCHARTER_H_

#include "DotstarStripe.h"
#include "String.h"
#include "nvs.h"

#define RING_LEDCOUNT 15 // Max 60

class DisplayCharter
{
  public:
    DisplayCharter();
    void Init();
    void SetLeds(__uint8_t pos, __uint8_t count, __uint8_t r, __uint8_t g, __uint8_t b);
    void SetLeds(__uint8_t pos, __uint8_t count, __uint32_t color);
    void SetBackground(__uint8_t r, __uint8_t g, __uint8_t b);
    void SetBackground(__uint32_t color);
    void SetWhirl(__uint8_t wspeed, bool clockwise);
    void SetMorph(__uint16_t period, __uint8_t mspeed);
    __uint16_t ParseLedArg(String& argument, __uint16_t iPos);
    void ParseBgArg(String& argument);
    void ParseWhirlArg(String& argument);
    void ParseMorphArg(String& argument);
    void Display(DotstarStripe &dotstar, bool send);

    bool Read(bool topRing);
    bool Write(bool topRing);   

    // Expose Led Colors
    __uint8_t GetLedRed(__uint8_t ledNo) {return mLedRed[ledNo];};
    __uint8_t GetLedGreen(__uint8_t ledNo) {return mLedGreen[ledNo];};
    __uint8_t GetLedBlue(__uint8_t ledNo) {return mLedBlue[ledNo];};
    
    // Expose Background Color
    __uint8_t GetBackgroundRed() {return mBackgroundRed;};
    __uint8_t GetBackgroundGreen() {return mBackgroundGreen;};
    __uint8_t GetBackgroundBlue() {return mBackgroundBlue;};
    
    // Expose Whirl Parameters
    __uint8_t GetWhirlSpeed() {return whirlSpeed;};
    bool GetWhirlClockwise() {return whirlClockwise;};
    __uint8_t GetWhirlTick() {return whirlTick;};

    // Expose Morph Parameters
    __uint8_t GetMorphState() {return morphingState;};
    __uint8_t GetMorphPeriod() {return morphPeriod;};
    __uint8_t GetMorphPeriodTick() {return morphPeriodTick;};
    __uint8_t GetMorphSpeed() {return morphSpeed;};
    __uint8_t GetMorphSpeedTick() {return morphSpeedTick;};
    __uint8_t GetMorphPercentage() {return morphingPercentage;};

    __uint32_t mRingLedCount = RING_LEDCOUNT;

  private:
    //__uint32_t GetPixelColor(__uint8_t i);
    void GetPixelColor(__uint8_t i, __uint8_t& ruR, __uint8_t& ruG, __uint8_t& ruB);
    
  private:
    bool ReadInt(nvs_handle h, const char* sKey, uint32_t& rbValue);
    bool WriteInt(nvs_handle h, const char* sKey, uint32_t iValue);

    bool mLedSet[RING_LEDCOUNT];
    __uint8_t mLedRed[RING_LEDCOUNT];
	__uint8_t mLedGreen[RING_LEDCOUNT];
	__uint8_t mLedBlue[RING_LEDCOUNT];
	//__uint32_t ledColor[RING_LEDCOUNT];
	__uint8_t mBackgroundRed;
	__uint8_t mBackgroundGreen;
	__uint8_t mBackgroundBlue;
    //__uint32_t backgroundColor;   
    __uint8_t whirlSpeed;
    bool whirlClockwise;
    __uint8_t offset;
    __uint8_t whirlTick;
    __uint8_t morphingState;
    __uint16_t morphPeriod;
    __uint16_t morphPeriodTick;
    __uint8_t morphSpeed;
    __uint8_t morphSpeedTick;
    __uint8_t morphingPercentage;
};


#endif


