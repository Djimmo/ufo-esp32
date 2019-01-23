#include <freertos/FreeRTOS.h>
#include "DisplayCharter.h"
#include "String.h"
#include <esp_log.h>
#include "nvs_flash.h"

static const char LOGTAG[] = "DisplayCharter";

DisplayCharter::DisplayCharter(){
	Init();
}
void DisplayCharter::Init(){
	for (__uint8_t i=0 ; i< mRingLedCount ; i++){
		mLedSet[i] = false;
		mLedRed[i] = 00;
		mLedGreen[i] = 00;
		mLedBlue[i] = 00;
	}
	mBackgroundRed = 0;
	mBackgroundGreen = 0;
	mBackgroundBlue = 0;

	offset = 0;
	whirlSpeed = 0;
	morphPeriod = 0;
	morphingState = 0;

	morphingPercentage = 0;
}


void DisplayCharter::SetLeds(__uint8_t pos, __uint8_t count, __uint8_t r, __uint8_t g, __uint8_t b){
	for (__uint8_t i=0 ; i<count ; i++){
		__uint8_t o = (pos + i) % mRingLedCount;
		mLedSet[o] = true;
		mLedRed[o] = r;
		mLedGreen[o] = g;
		mLedBlue[o] = b;
	}
}

void DisplayCharter::SetLeds(__uint8_t pos, __uint8_t count, __uint32_t color) {

	__uint8_t r = (color & 0xff0000) >> 16;
	__uint8_t g = (color & 0x00ff00) >> 8;
	__uint8_t b = (color & 0x0000ff);

	this->SetLeds(pos, count, r, g, b);

}


void DisplayCharter::SetBackground( __uint8_t r, __uint8_t g, __uint8_t b){
	mBackgroundRed = r;
	mBackgroundGreen = g;
	mBackgroundBlue = b;
}

void DisplayCharter::SetBackground( __uint32_t color){
	__uint8_t r = (color & 0xff0000) >> 16;
	__uint8_t g = (color & 0x00ff00) >> 8;
	__uint8_t b = (color & 0x0000ff);

	this->SetBackground(r, g, b);
}

void DisplayCharter::SetWhirl(__uint8_t wspeed, bool clockwise){
	whirlSpeed = wspeed;
	whirlTick = 0xFF - whirlSpeed;
	whirlClockwise = clockwise;
}

void DisplayCharter::SetMorph(__uint16_t period, __uint8_t mspeed){
	morphPeriod = period;
	morphPeriodTick = morphPeriod;

	if (mspeed > 10)
		morphSpeed = 10;
	else
		morphSpeed = mspeed;

	ESP_LOGD("DisplayCharter", "SetMorph %d, %d", period, mspeed);
}

__uint16_t DisplayCharter::ParseLedArg(String& argument, __uint16_t iPos){
	__uint8_t seg = 0;
	String pos;
	String count;
	String color;
	__uint16_t i=iPos;
	while ((i < argument.length()) && (seg < 3)){
		char c = argument.charAt(i);
		if (c == '|')
			seg++;
		else switch(seg){
			case 0:
				pos+= c;
				break;
			case 1:
				count += c;
				break;
			case 2:
				color += c;
		}
		i++;
	}
	//pos.trim();
	//count.trim();
	//color.trim();
	ESP_LOGD("DisplayCharter", "ParseLedArg %s, %s, %s", pos.c_str(), count.c_str(), color.c_str());


	if ((pos.length() > 0) && (count.length() > 0) && (color.length() == 6)){
		SetLeds(atoi(pos.c_str()), atoi(count.c_str()), strtol(color.substring(0, 2).c_str(), NULL, 16),
												  	  strtol(color.substring(2, 4).c_str(), NULL, 16),
													  strtol(color.substring(4, 6).c_str(), NULL, 16));
	}
	return i;
}

void DisplayCharter::ParseBgArg(String& argument){
	if (argument.length() == 6)
		SetBackground(	strtol(argument.substring(0, 2).c_str(), NULL, 16),
						strtol(argument.substring(2, 4).c_str(), NULL, 16),
						strtol(argument.substring(4, 6).c_str(), NULL, 16));
}

void DisplayCharter::ParseWhirlArg(String& argument){
	__uint8_t seg = 0;
	String wspeed;

	for (__uint8_t i=0 ; i< argument.length() ; i++){
		char c = argument.charAt(i);
		if (c == '|'){
			if (++seg == 2)
				break;
		}
		else switch(seg){
			case 0:
				wspeed += c;
				break;
		}
	}
	//wspeed.trim();

	if (wspeed.length() > 0){
		SetWhirl(atoi(wspeed.c_str()), !seg);
	}
}

void DisplayCharter::ParseMorphArg(String& argument){
	__uint8_t seg = 0;
	String period;
	String mspeed;

	for (__uint8_t i=0 ; i< argument.length() ; i++){
		char c = argument.charAt(i);
		if (c == '|'){
			if (++seg == 2)
				break;
		}
		else switch(seg){
			case 0:
				period += c;
				break;
			case 1:
				mspeed += c;
				break;
		}
	}
  //period.trim();
  //mspeed.trim();

	if ((period.length() > 0) && (mspeed.length() > 0)){
		SetMorph(atoi(period.c_str()), atoi(mspeed.c_str()));
	}
}

void DisplayCharter::GetPixelColor(__uint8_t i, __uint8_t& ruR, __uint8_t& ruG, __uint8_t& ruB){

	if(! mLedSet[i]){
		ruR = mBackgroundRed;
		ruG = mBackgroundGreen;
		ruB = mBackgroundBlue;
	}
	else{
		ruR = (mLedRed[i] * (100 - morphingPercentage) / 100 + mBackgroundRed * morphingPercentage / 100);
		ruG = (mLedGreen[i] * (100 - morphingPercentage) / 100 + mBackgroundGreen * morphingPercentage / 100);
		ruB = (mLedBlue[i] * (100 - morphingPercentage) / 100 + mBackgroundBlue * morphingPercentage / 100);
	}
}

void DisplayCharter::Display(DotstarStripe &dotstar, bool send){

	//taskENTER_CRITICAL(&mMutex);

	if (send){
		__uint8_t r;
		__uint8_t g;
		__uint8_t b;
		for (__uint8_t i=0 ; i<mRingLedCount ; i++){
			GetPixelColor(i, r, g, b);
			dotstar.SetLeds(i, 1, r, g, b);
		}
		dotstar.SetStartPos(offset);
		dotstar.Show();
	}

   
	if (whirlSpeed){
		if (!whirlTick--){
			if (whirlClockwise){
				if (++offset >= mRingLedCount)
					offset = 0;
			}
			else{
				if (!offset)
					offset = mRingLedCount - 1;
				else
					offset--;
			}
			whirlTick = 0xFF - whirlSpeed;
		}
	}

	switch (morphingState){
		case 0:
			if (morphPeriod){
				if (!--morphPeriodTick){
					morphingState = 1;
					morphingPercentage = 0;
					morphSpeedTick = 11 - morphSpeed;

					morphPeriodTick = morphPeriod;
				}
			}
			break;
		case 1:
			if (!--morphSpeedTick){
				if (morphingPercentage < 100)
					morphingPercentage+=2;
				else
					morphingState = 2;

				morphSpeedTick = 11 - morphSpeed;
			}
			break;
		case 2:
			if (!--morphSpeedTick){
				if (morphingPercentage > 0)
					morphingPercentage-=2;
				else
					morphingState = 0;

				morphSpeedTick = 11 - morphSpeed;
			}
			break;
	}
	
}

bool DisplayCharter::Read(bool ring){
	nvs_handle h;

	if (nvs_flash_init() != ESP_OK)
		return false;
	if (nvs_open("LRConfig", NVS_READONLY, &h) != ESP_OK)
		return false;

	ReadInt(h, ring ? (const char*)"topRingLed" : (const char*)"bottomRingLed", mRingLedCount);

	nvs_close(h);

	return true;
}

bool DisplayCharter::ReadInt(nvs_handle h, const char* sKey, uint32_t& riValue){
	esp_err_t rc = nvs_get_u32(h, sKey, &riValue);
	
	if (rc != ESP_OK) {
		ESP_LOGW(LOGTAG, "nvs_get_u32(%s) failed: %s", sKey, esp_err_to_name(rc));
		return false;
	}
	return true;
}

bool DisplayCharter::Write(bool ring)
{
	nvs_handle h;

	if (nvs_flash_init() != ESP_OK)
		return false;
	if (nvs_open("LRConfig", NVS_READWRITE, &h) != ESP_OK)
		return false;
	//nvs_erase_all(h); //otherwise I need double the space

	if (!WriteInt(h, ring ? (const char*)"topRingLed" : (const char*)"bottomRingLed", mRingLedCount))
		return nvs_close(h), false;

	ESP_LOGI(LOGTAG, "ring %s to %u LEDs success", ring ? "topRingLed" : "bottomRingLed", mRingLedCount);
	nvs_commit(h);
	nvs_close(h);
	
	return true;
}


bool DisplayCharter::WriteInt(nvs_handle h, const char* sKey, uint32_t iValue){
	esp_err_t rc = nvs_set_u32(h, sKey, iValue);
	if (rc != ESP_OK) {
		ESP_LOGE(LOGTAG, "nvs_set_u32(%s) failed: %s", sKey, esp_err_to_name(rc));
		return false;
	}
	return true;
}





//------------------------------------------------------------------------------------------------------

/*

IPDisplay::IPDisplay(){
	ipspeed = 1100;
	displayCharter = 0;
}


void IPDisplay::ShowIp(const char* sIp, __uint8_t uLen, DisplayCharter* displayCh){
	displayCharter = displayCh;
	ipAddress = sIp;
	pos = 0;
	color = 0;
	colorValue[0] = 0xFF;
	colorValue[1] = 0x00;
	colorValue[2] = 0x00;
	tick = ipspeed;
	shortBreak = false;
}

void IPDisplay::ProcessTick()
{
	if (!displayCharter)
		return;
    
	if (!--tick){
		if (shortBreak){
			displayCharter->Init();
			displayCharter->SetLeds(0, 15, 0x30, 0x30, 0x30);
			tick = 100;
			shortBreak = false;
			return;
		}
		shortBreak = true;
		tick = ipspeed;

		if (pos >= ipAddress.length()){
			pos = 0;
			if (++color >= 3)
				color = 0;
			switch (color){
				case 0:
					colorValue[0] = 0xFF;
					colorValue[1] = 0x00;
					colorValue[2] = 0x00;
					break;
				case 1:
					colorValue[0] = 0x00;
					colorValue[1] = 0xFF;
					colorValue[2] = 0x00;
					break;
				case 2:
					colorValue[0] = 0x00;
					colorValue[1] = 0x00;
					colorValue[2] = 0xFF;
				 	break;
			}
		}
		displayCharter->Init();
		char c = ipAddress.at(pos);
		pos++;
    
		switch (c){
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
				displayCharter->SetLeds(0, c - '0', colorValue[0], colorValue[1], colorValue[2]);
				break;
			case '6':
			case '7':
			case '8':
			case '9':
				displayCharter->SetLeds(0, 5, colorValue[0], colorValue[1], colorValue[2]);
				displayCharter->SetLeds(8, c - '5', colorValue[0], colorValue[1], colorValue[2]);
				break;
			case '.':
				displayCharter->SetLeds(0, 1, 0xb0, 0xb0, 0xb0);
				displayCharter->SetLeds(5, 1, 0xb0, 0xb0, 0xb0);
				displayCharter->SetLeds(10, 1, 0xb0, 0xb0, 0xb0);
				break;
		}
	}
}
void IPDisplay::StopShowingIp(){ 
	if (displayCharter)
		displayCharter->Init();
	displayCharter = 0;
}*/
