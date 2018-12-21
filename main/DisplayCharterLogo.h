#ifndef MAIN_DISPLAYCHARTERLOGO_H_
#define MAIN_DISPLAYCHARTERLOGO_H_

#include "DotstarStripe.h"
#include "String.h"

class DisplayCharterLogo
{
  public:
	DisplayCharterLogo();
	virtual ~DisplayCharterLogo();

	void Init();

	void SetLed(__uint8_t uLed, __uint8_t r, __uint8_t g, __uint8_t b);
	void ParseLogoLedArg(String &argument);

	void Display(DotstarStripe &dotstar);

	// Expose Led Colors
	__uint8_t GetLedRed(__uint8_t ledNo) { return mLedRed[ledNo]; };
	__uint8_t GetLedGreen(__uint8_t ledNo) { return mLedGreen[ledNo]; };
	__uint8_t GetLedBlue(__uint8_t ledNo) { return mLedBlue[ledNo]; };

  private:
	__uint8_t mLedRed[4];
	__uint8_t mLedGreen[4];
	__uint8_t mLedBlue[4];

	bool mbChanged;
	__uint8_t muSendAnywayCount;
};

#endif /* MAIN_DISPLAYCHARTERLOGO_H_ */
