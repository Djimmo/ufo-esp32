#include <freertos/FreeRTOS.h>
#include "Ufo.h"
#include "DynatraceIntegration.h"
#include "DynatraceMonitoring.h"
#include "DynatraceAction.h"
#include "AWSIntegration.h"
#include "DotstarStripe.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include <esp_log.h>

static const char* LOGTAG = "Ufo";


extern "C"{
	void app_main();
}


void task_function_webserver(void *pvParameter)
{
	((Ufo*)pvParameter)->TaskWebServer();
	vTaskDelete(NULL);
}

void task_function_display(void *pvParameter)
{
	((Ufo*)pvParameter)->TaskDisplay();
	vTaskDelete(NULL);
}


//----------------------------------------------------------------------------------------


Ufo::Ufo() : mStripeLevel1(15, GPIO_NUM_16, GPIO_NUM_18), mStripeLevel2(15, GPIO_NUM_16, GPIO_NUM_17), mStripeLogo(4, GPIO_NUM_16, GPIO_NUM_19),
	mMqtt(*this)
{
	mServer.SetUfo(this);
	mWifi.SetConfig(&mConfig);
	mWifi.SetStateDisplay(&mStateDisplay);
	mbApiCallReceived = false;
}

Ufo::~Ufo() {
}

void Ufo::Start(){
	ESP_LOGI(LOGTAG, "===================== Dynatrace UFO ========================");
	ESP_LOGI(LOGTAG, "Firmware Version: %s", FIRMWARE_VERSION);
	ESP_LOGI(LOGTAG, "Start");

	SetId();
	mConfig.Read();
	if (mConfig.msAPSsid == "UFO")
		mConfig.msAPSsid = mId;
	mConfig.msUfoId = mId;
	if (mConfig.msHostname == "UFO")
		mConfig.msHostname = mId;

	DynatraceAction* dtStartup = dt.enterAction("Startup");

	mbButtonPressed = !gpio_get_level(GPIO_NUM_0);
	mStateDisplay.SetAPMode(mConfig.mbAPMode);
	mApiStore.Init();

	gpio_pad_select_gpio(10);
	gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
	gpio_set_pull_mode(GPIO_NUM_0, GPIO_PULLUP_ONLY);

	gpio_pad_select_gpio(16);
	gpio_set_direction(GPIO_NUM_16, GPIO_MODE_OUTPUT);
	gpio_pad_select_gpio(17);
	gpio_set_direction(GPIO_NUM_17, GPIO_MODE_OUTPUT);
	gpio_pad_select_gpio(18);
	gpio_set_direction(GPIO_NUM_18, GPIO_MODE_OUTPUT);
	gpio_pad_select_gpio(19);
	gpio_set_direction(GPIO_NUM_19, GPIO_MODE_OUTPUT);

	xTaskCreatePinnedToCore(&task_function_webserver, "Task_WebServer", 12288, this, 5, NULL, 0); //Ota update (upload) just works on core 0
	xTaskCreate(&task_function_display, "Task_Display", 4096, this, 5, NULL);

	// Dynatrace Monitoring
	dt.Init(this, &mAws);

	if (mConfig.mbAPMode){
		if (mConfig.muLastSTAIpAddress){
			char sBuf[16];
			sprintf(sBuf, "%d.%d.%d.%d", IP2STR((ip4_addr*)&mConfig.muLastSTAIpAddress));
			ESP_LOGD(LOGTAG, "Last IP when connected to AP: %d : %s", mConfig.muLastSTAIpAddress, sBuf);
		}
		mWifi.StartAPMode(mConfig.msAPSsid, mConfig.msAPPass, mConfig.msHostname);
		dt.Shutdown();
	}
	else{
		DynatraceAction* dtWifi = dt.enterAction("Start Wifi", dtStartup);	
		if (mConfig.msSTAENTUser.length())
			mWifi.StartSTAModeEnterprise(mConfig.msSTASsid, mConfig.msSTAENTUser, mConfig.msSTAPass, mConfig.msSTAENTCA, mConfig.msHostname);
		else
			mWifi.StartSTAMode(mConfig.msSTASsid, mConfig.msSTAPass, mConfig.msHostname);
	
		dt.leaveAction(dtWifi);
		// Dynatrace API Integration
		mDt.Init(this, &mDisplayCharterLevel1, &mDisplayCharterLevel2);
		// AWS communication layer
		mAws.Init(this);
	}
	mMqtt.Init();

	mDisplayCharterLevel1.Read(true);
	mDisplayCharterLevel2.Read(false);
	dt.leaveAction(dtStartup);

}

void Ufo::TaskWebServer(){

	while (1){
		if (mWifi.IsConnected()){
			ESP_LOGI("Ufo", "starting Webserver");
			mServer.StartUfoServer();
		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void Ufo::TaskDisplay(){
	__uint8_t uSendState = 0;
	while (1){
		if (mWifi.IsConnected() && (mbApiCallReceived || (mDt.IsActive() && mStateDisplay.IpShownLongEnough()))){
			if (!uSendState){
				mDisplayCharterLevel1.Display(mStripeLevel1, true);
				mDisplayCharterLevel2.Display(mStripeLevel2, true);
				uSendState++;
			}
			else{
				mDisplayCharterLevel1.Display(mStripeLevel1, false);
				mDisplayCharterLevel2.Display(mStripeLevel2, false);
				if (uSendState >= 5)
					uSendState = 0;
				else
					uSendState++;
			}
		}
		else
			mStateDisplay.Display(mStripeLevel1, mStripeLevel2);

		mDisplayCharterLogo.Display(mStripeLogo);

		if (!gpio_get_level(GPIO_NUM_0)){
			if (!mbButtonPressed){
				ESP_LOGI("Ufo", "button pressed");
				mDisplayCharterLevel1.SetLeds(0, 15, 0x440044);
				mDisplayCharterLevel2.SetLeds(0, 15, 0x440044);
				mDisplayCharterLevel1.Display(mStripeLevel1, true);
				mDisplayCharterLevel2.Display(mStripeLevel2, true);
				vTaskDelay(200);
				mConfig.ToggleAPMode();
				mConfig.Write();
				if (mConfig.mbAPMode){
					ESP_LOGI("Ufo", "enter AP mode");
				} else {
					ESP_LOGI("Ufo", "enter standard mode");					
				}
				esp_restart();
			}
		}
		else
			mbButtonPressed = false;

		vTaskDelay(1);
	}
}

void Ufo::InitLogoLeds(){
	mStripeLogo.SetLeds(0, 1, 0, 100, 255);
	mStripeLogo.SetLeds(1, 1, 125, 255, 0);
	mStripeLogo.SetLeds(2, 1, 0, 255, 0);
	mStripeLogo.SetLeds(3, 1, 255, 0, 150);
	mStripeLogo.Show();
	mStripeLevel1.SetLeds(0, 1, 0, 100, 255);
	mStripeLevel1.SetLeds(1, 1, 125, 255, 0);
	mStripeLevel1.SetLeds(2, 1, 0, 255, 0);
	mStripeLevel1.SetLeds(3, 1, 255, 0, 150);
	mStripeLevel1.Show();
}

void Ufo::ShowLogoLeds(){
	mStripeLogo.Show();
	mStripeLevel1.Show();
}

void Ufo::SetId() {
	uint8_t sMac[8]{};
	int rc = esp_read_mac(sMac, ESP_MAC_WIFI_SOFTAP);
	if (rc != ESP_OK) {
		ESP_LOGE(LOGTAG, "esp_read_mac: %s", esp_err_to_name(rc));
	}
	mId.printf("ufo-%02x%02x%02x%02x%02x%02x", sMac[0], sMac[1], sMac[2], sMac[3], sMac[4], sMac[5]);
	ESP_LOGI(LOGTAG, "Setting Device ID to %s", mId.c_str());
}

//-----------------------------------------------------------------------------------------

Ufo ufo;

void app_main(){

	nvs_flash_init();
	tcpip_adapter_init();

	ufo.Start();
}

