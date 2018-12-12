#include "DynamicRequestHandler.h"
#include "Ufo.h"
#include "DisplayCharter.h"
#include "Config.h"
#include "DynatraceAction.h"
#include "esp_system.h"
#include <esp_log.h>
#include "Ota.h"
#include "String.h"
#include "WebClient.h"
#include <cJSON.h>

static char tag[] = "DynamicRequestHandler";


//#define LATEST_FIRMWARE_URL "https://surpro4:9999/getfirmware"  // testing with local go server
//#define OTA_LATEST_FIRMWARE_JSON_URL "https://github.com/Dynatrace/ufo-esp32/raw/master/firmware/version.json"
//#define OTA_LATEST_FIRMWARE_URL "https://github.com/Dynatrace/ufo-esp32/raw/master/firmware/ufo-esp32.bin"
//#define OTA_LATEST_FIRMWARE_JSON_URL "https://raw.githubusercontent.com/Dynatrace/ufo-esp32/master/firmware/version.json"
//#define OTA_LATEST_FIRMWARE_URL "https://raw.githubusercontent.com/Dynatrace/ufo-esp32/master/firmware/ufo-esp32.bin"

String DynamicRequestHandler::HandleApiRequest(std::list<TParam>& params) {
	String sBody;

    DynatraceAction* dtHandleRequest = mpUfo->dt.enterAction("Handle API Request");	

	mpUfo->IndicateApiCall();

	std::list<TParam>::iterator it = params.begin();
	while (it != params.end()){

		if ((*it).paramName == "top_init")
			mpUfo->DisplayCharterLevel1().Init();
		else if ((*it).paramName == "top"){
			__uint16_t i = 0;
			 while (i < (*it).paramValue.length())
				 i = mpUfo->DisplayCharterLevel1().ParseLedArg((*it).paramValue, i);
		}
		else if ((*it).paramName == "top_bg")
			mpUfo->DisplayCharterLevel1().ParseBgArg((*it).paramValue);
		else if ((*it).paramName == "top_whirl")
			mpUfo->DisplayCharterLevel1().ParseWhirlArg((*it).paramValue);
		else if ((*it).paramName == "top_morph")
			mpUfo->DisplayCharterLevel1().ParseMorphArg((*it).paramValue);

		if ((*it).paramName == "bottom_init")
			mpUfo->DisplayCharterLevel2().Init();
		else if ((*it).paramName == "bottom"){
			__uint16_t i = 0;
			 while (i < (*it).paramValue.length())
				 i = mpUfo->DisplayCharterLevel2().ParseLedArg((*it).paramValue, i);
		}
		else if ((*it).paramName == "bottom_bg")
			mpUfo->DisplayCharterLevel2().ParseBgArg((*it).paramValue);
		else if ((*it).paramName == "bottom_whirl")
			mpUfo->DisplayCharterLevel2().ParseWhirlArg((*it).paramValue);
		else if ((*it).paramName == "bottom_morph")
			mpUfo->DisplayCharterLevel2().ParseMorphArg((*it).paramValue);

		else if ((*it).paramName == "logo")
			mpUfo->GetLogoDisplay().ParseLogoLedArg((*it).paramValue);
		else if ((*it).paramName == "logo_reset")
			mpUfo->GetLogoDisplay().Init();


		it++;
	}

	mpUfo->dt.leaveAction(dtHandleRequest);

	return sBody;
}

bool DynamicRequestHandler::HandleApiListRequest(std::list<TParam>& params, HttpResponse& rResponse){
    DynatraceAction* dtHandleRequest = mpUfo->dt.enterAction("Handle API List Request");	
	String sBody;
	mpUfo->GetApiStore().GetApisJson(sBody);
	rResponse.AddHeader(HttpResponse::HeaderContentTypeJson);
	rResponse.AddHeader(HttpResponse::HeaderNoCache);
	rResponse.SetRetCode(200);
	mpUfo->dt.leaveAction(dtHandleRequest);
	return rResponse.Send(sBody.c_str(), sBody.length());
}
bool DynamicRequestHandler::HandleApiEditRequest(std::list<TParam>& params, HttpResponse& rResponse){

    DynatraceAction* dtHandleRequest = mpUfo->dt.enterAction("Handle API Edit Request");	
	__uint8_t uId = 0xff;
	const char* sNewApi = NULL;
	bool bDelete = false;

	rResponse.SetRetCode(302);
	String sBody = "Updated";
	for (auto &it : params) {
		if (it.paramName == "apiid")
			uId = it.paramValue.toInt() - 1;
		else if (it.paramName == "apiedit")
			sNewApi = it.paramValue.c_str();
		else if (it.paramName == "delete")
			bDelete = true;
	}
	if (bDelete) {
		if (!mpUfo->GetApiStore().DeleteApi(uId)) {
			rResponse.SetRetCode(500);
			sBody = "DeleteApi failed";
		}
	}
	else {
		if (!mpUfo->GetApiStore().SetApi(uId, sNewApi)) {
			rResponse.SetRetCode(500);
			sBody = "SetApi failed";
		}
	}
	rResponse.AddHeader(HttpResponse::HeaderNoCache);
	rResponse.AddHeader("Location: /");
	mpUfo->dt.leaveAction(dtHandleRequest);
	return rResponse.Send(sBody);
}


bool DynamicRequestHandler::HandleInfoRequest(std::list<TParam>& params, HttpResponse& rResponse){

    DynatraceAction* dtHandleRequest = mpUfo->dt.enterAction("Handle Info Request");	
	char sHelp[64];
	String tmp;
	cJSON *json = cJSON_CreateObject();
	cJSON_AddStringToObject(json, "apmode", mpUfo->GetConfig().mbAPMode ? "1" : "0");
	cJSON_AddNumberToObject(json, "heap", esp_get_free_heap_size());
	cJSON_AddStringToObject(json, "ssid", mpUfo->GetConfig().msSTASsid.c_str());
	cJSON_AddStringToObject(json, "hostname", mpUfo->GetConfig().msHostname.c_str());
	cJSON_AddStringToObject(json, "enterpriseuser", mpUfo->GetConfig().msSTAENTUser.c_str());
	cJSON_AddStringToObject(json, "sslenabled", mpUfo->GetConfig().mbWebServerUseSsl ? "1" : "0");
	cJSON_AddNumberToObject(json, "listenport", mpUfo->GetConfig().muWebServerPort);
	if (mpUfo->GetConfig().mbAPMode) {
		tmp.printf(IPSTR, IP2STR((ip4_addr*)&(mpUfo->GetConfig().muLastSTAIpAddress)));
		cJSON_AddStringToObject(json, "lastiptoap", tmp.c_str());
	} else {
		cJSON_AddStringToObject(json, "ipaddress", mpUfo->GetWifi().GetLocalAddress().c_str());
		sHelp[0] = 0;
		mpUfo->GetWifi().GetGWAddress(sHelp);
		cJSON_AddStringToObject(json, "ipgateway", sHelp);
		sHelp[0] = 0;
		mpUfo->GetWifi().GetNetmask(sHelp);
		cJSON_AddStringToObject(json, "ipsubnetmask", sHelp);
		uint8_t uChannel = 0;
		int8_t iRssi = 0;
		mpUfo->GetWifi().GetApInfo(iRssi, uChannel);
		cJSON_AddNumberToObject(json, "rssi", iRssi);
		cJSON_AddNumberToObject(json, "channel", uChannel);
	}
	memset(sHelp, 0, 6);
	mpUfo->GetWifi().GetMac((uint8_t*)sHelp);
	tmp.printf("%02x:%02x:%02x:%02x:%02x:%02x", sHelp[0], sHelp[1], sHelp[2], sHelp[3], sHelp[4], sHelp[5]);
	cJSON_AddStringToObject(json, "macaddress", tmp.c_str());
	cJSON_AddStringToObject(json, "firmwareversion", FIRMWARE_VERSION);
	cJSON_AddStringToObject(json, "ufoid", mpUfo->GetConfig().msUfoId.c_str());
	cJSON_AddStringToObject(json, "ufoname", mpUfo->GetConfig().msUfoName.c_str());
	cJSON_AddStringToObject(json, "organization", mpUfo->GetConfig().msOrganization.c_str());
	cJSON_AddStringToObject(json, "department", mpUfo->GetConfig().msDepartment.c_str());
	cJSON_AddStringToObject(json, "location", mpUfo->GetConfig().msLocation.c_str());
	cJSON_AddNumberToObject(json, "dtenabled", mpUfo->GetConfig().mbDTEnabled);
	cJSON_AddStringToObject(json, "dtenvid", mpUfo->GetConfig().msDTEnvIdOrUrl.c_str());
	cJSON_AddNumberToObject(json, "dtinterval", mpUfo->GetConfig().miDTInterval);
	cJSON_AddNumberToObject(json, "dtmonitoring", mpUfo->GetConfig().mbDTMonitoring);
	cJSON_AddStringToObject(json, "adminpw", mpUfo->GetConfig().msAdminPw.empty() ? "" : ".....");
	cJSON_AddStringToObject(json, "mqtttopic", mpUfo->GetConfig().msMqttTopic.c_str());
	cJSON_AddStringToObject(json, "mqttstatustopic", mpUfo->GetConfig().msMqttStatusTopic.c_str());
	cJSON_AddNumberToObject(json, "mqttstatusperiodseconds", mpUfo->GetConfig().muMqttStatusPeriodSeconds);
	cJSON_AddNumberToObject(json, "mqttstatusqos", mpUfo->GetConfig().muMqttStatusQos);
	cJSON_AddStringToObject(json, "mqtturi", mpUfo->GetConfig().msMqttUri.c_str());
	cJSON_AddStringToObject(json, "mqttpw", mpUfo->GetConfig().msMqttPw.empty() ? "" : ".....");
	cJSON_AddStringToObject(json, "mqttservercert", mpUfo->GetConfig().msMqttServerCert.c_str());
	cJSON_AddStringToObject(json, "mqttclientkey", mpUfo->GetConfig().msMqttClientKey.empty() ? "" : ".....");
	cJSON_AddStringToObject(json, "mqttclientcert", mpUfo->GetConfig().msMqttClientCert.c_str());
	cJSON_AddNumberToObject(json, "mqttqos", mpUfo->GetConfig().muMqttQos);
	cJSON_AddNumberToObject(json, "mqttkeepalive", mpUfo->GetConfig().muMqttKeepalive);
	cJSON_AddStringToObject(json, "mqtterrorstate", mpUfo->GetConfig().msMqttErrorState.c_str());
	cJSON_AddNumberToObject(json, "topringledcount", mpUfo->DisplayCharterLevel1().mRingLedCount);
	cJSON_AddNumberToObject(json, "bottomringledcount", mpUfo->DisplayCharterLevel2().mRingLedCount);
	char* sBody = cJSON_Print(json);
	cJSON_Delete(json);
	rResponse.SetRetCode(sBody ? 200 : 500);
	rResponse.AddHeader(HttpResponse::HeaderContentTypeJson);
	rResponse.AddHeader(HttpResponse::HeaderNoCache);
	bool rc = false;
	if (sBody) {
		rc = rResponse.Send(sBody, strlen(sBody));
		free(sBody);
	}
	mpUfo->dt.leaveAction(dtHandleRequest);
	return rc;
}

bool DynamicRequestHandler::HandleDynatraceIntegrationRequest(std::list<TParam>& params, HttpResponse& rResponse){

    DynatraceAction* dtHandleRequest = mpUfo->dt.enterAction("Handle Dynatrace Integration Request");	
	String sEnvId;
	String sApiToken;
	bool bEnabled = false;
	int iInterval = 0;

	String sBody;

	std::list<TParam>::iterator it = params.begin();
	while (it != params.end()){
		if ((*it).paramName == "dtenabled")
			bEnabled = (*it).paramValue;
		else if ((*it).paramName == "dtenvid")
			sEnvId = (*it).paramValue;
		else if ((*it).paramName == "dtapitoken")
			sApiToken = (*it).paramValue;
		else if ((*it).paramName == "dtinterval")
			iInterval = (*it).paramValue.toInt();
		it++;
	}

	mpUfo->GetConfig().mbDTEnabled = bEnabled;
	mpUfo->GetConfig().msDTEnvIdOrUrl = sEnvId;
	if (sApiToken.length())
		mpUfo->GetConfig().msDTApiToken = sApiToken;
	mpUfo->GetConfig().miDTInterval = iInterval;

	if (mpUfo->GetConfig().Write())
		mpUfo->GetDtIntegration().ProcessConfigChange();

	ESP_LOGI(tag, "Dynatrace Integration Saved");

	rResponse.AddHeader(HttpResponse::HeaderNoCache);
	rResponse.AddHeader("Location: /#!pagedynatraceintegrationsettings");
	rResponse.SetRetCode(302);
	mpUfo->dt.leaveAction(dtHandleRequest);
	return rResponse.Send();
}


bool DynamicRequestHandler::HandleDynatraceMonitoringRequest(std::list<TParam>& params, HttpResponse& rResponse){

    DynatraceAction* dtHandleRequest = mpUfo->dt.enterAction("Handle Dynatrace Monitoring Request");	
	bool bEnabled = false;

	String sBody;

	std::list<TParam>::iterator it = params.begin();
	while (it != params.end()){
		if ((*it).paramName == "dtmonitoring")
			bEnabled = (*it).paramValue;
		else if ((*it).paramName == "ufoname")
			mpUfo->GetConfig().msUfoName = (*it).paramValue;
		else if ((*it).paramName == "organization")
			mpUfo->GetConfig().msOrganization = (*it).paramValue;
		else if ((*it).paramName == "department")
			mpUfo->GetConfig().msDepartment = (*it).paramValue;
		else if ((*it).paramName == "location")
			mpUfo->GetConfig().msLocation = (*it).paramValue;
			
		it++;
	}

	mpUfo->GetConfig().mbDTMonitoring = bEnabled;

	if (mpUfo->GetConfig().Write()) {
		mpUfo->GetAWSIntegration().ProcessConfigChange();
		mpUfo->dt.ProcessConfigChange();
	}

	ESP_LOGI(tag, "Dynatrace Monitoring Saved");

	rResponse.AddHeader(HttpResponse::HeaderNoCache);
	rResponse.AddHeader("Location: /#!pagedynatracemonitoringsettings");
	rResponse.SetRetCode(302);
	mpUfo->dt.leaveAction(dtHandleRequest);
	return rResponse.Send();
}

bool DynamicRequestHandler::HandleConfigRequest(std::list<TParam>& params, HttpResponse& rResponse){

    DynatraceAction* dtHandleRequest = mpUfo->dt.enterAction("Handle Config Request");	
	const char* sWifiMode = NULL;
	const char* sWifiSsid = NULL;
	const char* sWifiPass = NULL;
	const char* sWifiEntPass = NULL;
	const char* sWifiEntUser = NULL;
	const char* sWifiEntCA = NULL;
	const char* sWifiHostName = NULL;

	String sBody;

	std::list<TParam>::iterator it = params.begin();
	while (it != params.end()){

		if ((*it).paramName == "wifimode")
			sWifiMode = (*it).paramValue.c_str();
		else if ((*it).paramName == "wifissid")
			sWifiSsid = (*it).paramValue.c_str();
		else if ((*it).paramName == "wifipwd")
			sWifiPass = (*it).paramValue.c_str();
		else if ((*it).paramName == "wifientpwd")
			sWifiEntPass = (*it).paramValue.c_str();
		else if ((*it).paramName == "wifientuser")
			sWifiEntUser = (*it).paramValue.c_str();
		else if ((*it).paramName == "wifientca")
			sWifiEntCA = (*it).paramValue.c_str();
		else if ((*it).paramName == "wifihostname")
			sWifiHostName = (*it).paramValue.c_str();
		it++;
	}

	bool bOk = false;
	if (sWifiSsid){
		mpUfo->GetConfig().msSTASsid = sWifiSsid;

		if (sWifiMode && (sWifiMode[0] == '2')){ //enterprise wap2
			if (sWifiEntUser && (sWifiEntUser[0] != 0x00)){
					mpUfo->GetConfig().msSTAENTUser = sWifiEntUser;
				if (sWifiEntCA)
					mpUfo->GetConfig().msSTAENTCA = sWifiEntCA;
				else
					mpUfo->GetConfig().msSTAENTCA.clear();
				if (sWifiEntPass)
					mpUfo->GetConfig().msSTAPass = sWifiEntPass;
				else
					mpUfo->GetConfig().msSTAPass.clear();
				bOk = true;
			}
		}
		else{
			if (sWifiPass)
				mpUfo->GetConfig().msSTAPass = sWifiPass;
			else
				mpUfo->GetConfig().msSTAPass.clear();
			mpUfo->GetConfig().msSTAENTUser.clear();
			mpUfo->GetConfig().msSTAENTCA.clear();
			bOk = true;
		}
	}
	if (sWifiHostName && !mpUfo->GetConfig().msHostname.equals(sWifiHostName)) {
		mpUfo->GetConfig().msHostname = sWifiHostName;
		bOk = true;
	}
	if (bOk){
		mpUfo->GetConfig().mbAPMode = false;
		mpUfo->GetConfig().Write();
		mbRestart = true;
		sBody = "<html><head><title>SUCCESS - firmware update succeded, rebooting shortly.</title>"
				"<meta http-equiv=\"refresh\" content=\"10; url=/\"></head><body>"
				"<h2>New settings stored, rebooting shortly.</h2></body></html>";
		rResponse.SetRetCode(200);
	}
	else{
		rResponse.AddHeader("Location: /#!pagewifisettings");
		rResponse.SetRetCode(302);
	}

	rResponse.AddHeader(HttpResponse::HeaderNoCache);
	mpUfo->dt.leaveAction(dtHandleRequest);
	return rResponse.Send(sBody.c_str(), sBody.length());
}

bool DynamicRequestHandler::HandleMqttConfigRequest(std::list<TParam>& params, HttpResponse& rResponse){

    DynatraceAction* dtHandleRequest = mpUfo->dt.enterAction("Handle MQTT Config Request");
	Config& config = mpUfo->GetConfig();
	String sBody;
	for (auto &it : params) {
		if (it.paramName == "mqtttopic")
			config.msMqttTopic = it.paramValue;
		else if (it.paramName == "mqttstatustopic")
			config.msMqttStatusTopic = it.paramValue;
		else if (it.paramName == "mqttstatusperiodseconds")
			config.muMqttStatusPeriodSeconds = it.paramValue.toInt();
		else if (it.paramName == "mqttstatusqos")
			config.muMqttStatusQos = it.paramValue.toInt();
		else if (it.paramName == "mqttqos")
			config.muMqttQos = it.paramValue.toInt();
		else if (it.paramName == "mqtturi")
			config.msMqttUri = it.paramValue;
		else if (it.paramName == "mqttpw" && it.paramValue != ".....")
			config.msMqttPw = it.paramValue;
		else if (it.paramName == "mqttservercert")
			config.msMqttServerCert = it.paramValue;
		else if (it.paramName == "mqttclientkey" && it.paramValue != ".....")
			config.msMqttClientKey = it.paramValue;
		else if (it.paramName == "mqttclientcert")
			config.msMqttClientCert = it.paramValue;
		else if (it.paramName == "mqttkeepalive")
			config.muMqttKeepalive = it.paramValue.toInt();
		else if (it.paramName == "mqtterrorstate")
			config.msMqttErrorState = it.paramValue;
	}
	config.Write();
	mbRestart = true;
	sBody = "<html><head><title>SUCCESS - firmware update succeded, rebooting shortly.</title>"
			"<meta http-equiv=\"refresh\" content=\"10; url=/\"></head><body>"
			"<h2>New settings stored, rebooting shortly.</h2></body></html>";
	rResponse.SetRetCode(200);
	rResponse.AddHeader(HttpResponse::HeaderNoCache);
	mpUfo->dt.leaveAction(dtHandleRequest);
	return rResponse.Send(sBody.c_str(), sBody.length());
}

bool DynamicRequestHandler::HandleUfoConfigRequest(std::list<TParam>& params, HttpResponse& rResponse){
	DynatraceAction* dtHandleRequest = mpUfo->dt.enterAction("Handle UFO Config Request");
	DisplayCharter&	topRing = mpUfo->DisplayCharterLevel1();
	DisplayCharter&	bottomRing = mpUfo->DisplayCharterLevel2();
	String sBody;
	sBody = "<html><head><title>SUCCESS - UFO Config update succeeded.</title>"
			"<meta http-equiv=\"refresh\" content=\"3; url=/\"></head><body>";
	for (auto &it : params) {
		if (it.paramName == "topringledcount") {
			topRing.mRingLedCount = it.paramValue.toInt();
			sBody += " top: " + it.paramValue;
		}
		else if(it.paramName == "bottomringledcount") {
			bottomRing.mRingLedCount = it.paramValue.toInt();
			sBody += " bottom: " + it.paramValue;
		}
	}

	bottomRing.Write(false);
	topRing.Write(true);
	
	mbRestart = false;
	sBody += "<h2>New settings stored, returning to main page shortly.</h2></body></html>";
	rResponse.SetRetCode(200);
	rResponse.AddHeader(HttpResponse::HeaderNoCache);
	mpUfo->dt.leaveAction(dtHandleRequest);	
	return rResponse.Send(sBody.c_str(), sBody.length());
}

bool DynamicRequestHandler::HandleSrvConfigRequest(std::list<TParam>& params, HttpResponse& rResponse){
    DynatraceAction* dtHandleRequest = mpUfo->dt.enterAction("Handle Server Config Request");	
	const char* sSslEnabled = NULL;
	const char* sListenPort = NULL;
	const char* sServerCert = NULL;
	const char* sCurrentHost = NULL;

	String sBody;

	for (auto &it : params) {
		if (it.paramName == "sslenabled")
			sSslEnabled = it.paramValue.c_str();
		else if (it.paramName == "listenport")
			sListenPort = it.paramValue.c_str();
		else if (it.paramName == "servercert" && (it.paramValue.empty() || it.paramValue.length() > 256))
			sServerCert = it.paramValue.c_str();
		else if (it.paramName == "currenthost")
			sCurrentHost = it.paramValue.c_str();
		else if (it.paramName == "adminpw")
			mpUfo->GetConfig().msAdminPw = it.paramValue;
	}
	mpUfo->GetConfig().mbWebServerUseSsl = (sSslEnabled != NULL);
	mpUfo->GetConfig().muWebServerPort = atoi(sListenPort);
	mpUfo->GetConfig().msWebServerCert = sServerCert;
	ESP_LOGD(tag, "HandleSrvConfigRequest %d, %d", mpUfo->GetConfig().mbWebServerUseSsl, mpUfo->GetConfig().muWebServerPort);
	mpUfo->GetConfig().Write();
	mbRestart = true;
	
	String newUrl = "/";
	if (sCurrentHost){
		String sHost = sCurrentHost;
		int i = sHost.indexOf(':');
		if (i >= 0)
			sHost = sHost.substring(0, i);
		if (sHost.length()){
			if (sSslEnabled != NULL){
				newUrl = "https://" + sHost;
				if (mpUfo->GetConfig().muWebServerPort && (mpUfo->GetConfig().muWebServerPort != 443)){
					newUrl += ':';
					newUrl += sListenPort;
				}
			}
			else{
				newUrl = "http://" + sHost;
				if (mpUfo->GetConfig().muWebServerPort && (mpUfo->GetConfig().muWebServerPort != 80)){
					newUrl += ':';
					newUrl += sListenPort;
				}
			}
			newUrl += '/';
		}
	}

	sBody = "<html><head><title>SUCCESS - firmware update succeded, rebooting shortly.</title>"
			"<meta http-equiv=\"refresh\" content=\"10; url=";
	sBody += newUrl;
	sBody += "\"></head><body><h2>New settings stored, rebooting shortly.</h2></body></html>";
	rResponse.SetRetCode(200);
	rResponse.AddHeader(HttpResponse::HeaderNoCache);
	mpUfo->dt.leaveAction(dtHandleRequest);
	return rResponse.Send(sBody);
}

/*
GET: /firmware?update
GET: /firmware?progress
Response:
{ "session": "9724987887789", 
"progress": "22",
"status": "inprogress" }
Session: 32bit unsigned int ID that changes when UFO reboots
Progress: 0..100%
Status: notyetstarted | inprogress | connectionerror | flasherror | finishedsuccess
notyetstarted: Firmware update process has not started.
inprogress: Firmware update is in progress.
connectionerror: Firmware could not be downloaded. 
flasherror: Firmware could not be flashed.
finishedsuccess: Firmware successfully updated. Rebooting now.
*/

bool DynamicRequestHandler::HandleFirmwareRequest(std::list<TParam>& params, HttpResponse& response) {
    DynatraceAction* dtHandleRequest = mpUfo->dt.enterAction("Handle Firmware Request");	
	std::list<TParam>::iterator it = params.begin();
	String sBody;
	response.SetRetCode(400); // invalid request
	while (it != params.end()) {

		if ((*it).paramName == "progress") {
#ifdef OTA_LATEST_FIRMWARE_JSON_URL
			short progressPct = 0;
			const char* progressStatus = "notyetstarted";
			int   progress = Ota::GetProgress();
			if (progress >= 0) {
				progressPct = progress;
				progressStatus = "inprogress";
			} else {
				switch (progress) {
					case OTA_PROGRESS_NOTYETSTARTED: progressStatus = "notyetstarted"; 
							break;
					case OTA_PROGRESS_CONNECTIONERROR: progressStatus = "connectionerror"; 
							break;
					case OTA_PROGRESS_FLASHERROR: progressStatus = "flasherror"; 
							break;
					case OTA_PROGRESS_FINISHEDSUCCESS: progressStatus = "finishedsuccess"; 
							progressPct = 100;
							break;
				}
			}
			sBody = "{ \"session\": \"";
			sBody += Ota::GetTimestamp();
			sBody += "\", \"progress\": \"";
			sBody += progressPct;
			sBody += "\", \"status\": \"";
			sBody += progressStatus;
			sBody += "\"}";
			response.AddHeader(HttpResponse::HeaderContentTypeJson);
			response.SetRetCode(200);
		} else if ((*it).paramName == "update") {
			if (Ota::GetProgress() == OTA_PROGRESS_NOTYETSTARTED) {
				Ota::StartUpdateFirmwareTask(OTA_LATEST_FIRMWARE_URL);
				//TODO implement firmware version check;
			}
			// {"status":"firmware update initiated.", "url":"https://github.com/Dynatrace/ufo-esp32/raw/master/firmware/ufo-esp32.bin"}
			sBody = "{\"status\":\"firmware update initiated.\", \"url\":\"";
			sBody += OTA_LATEST_FIRMWARE_URL;
			sBody += "\"}";
			response.AddHeader(HttpResponse::HeaderContentTypeJson);
			response.SetRetCode(200);
		} else if ((*it).paramName == "check") {
			//TODO implement firmware version check;
			sBody = "not implemented";
			response.SetRetCode(501); // not implemented
		} else if ((*it).paramName == "restart") {
			//TODO implement firmware version check;
			sBody = "restarting...";
			mbRestart = true;
			response.SetRetCode(200);
		} else if ((*it).paramName == "switchbootpartition") {
			Ota ota;
			if(ota.SwitchBootPartition()) {
				mbRestart = true;
				sBody = "Switching boot partition successful.";
				response.SetRetCode(200);
			} else {
				//TODO add ota.GetErrorInfo() to inform end-user of problem
				sBody = "Switching boot partition failed.";
				response.SetRetCode(500);
			}
#else
			sBody += "{\"session\":\"1\",\"progress\":\"0\",\"status\":\"off\"}";
			response.AddHeader(HttpResponse::HeaderContentTypeJson);
			response.SetRetCode(200);
#endif
		} else {
				sBody = "Invalid request.";
				response.SetRetCode(400);
		}
		it++;
	}
	response.AddHeader(HttpResponse::HeaderNoCache);
	mpUfo->dt.leaveAction(dtHandleRequest);
	return response.Send(sBody.c_str(), sBody.length());
}

bool DynamicRequestHandler::HandleCheckFirmwareRequest(std::list<TParam>& params, HttpResponse& response) {

    DynatraceAction* dtHandleRequest = mpUfo->dt.enterAction("Handle Check Firmware Request");	
	String sBody;
	response.SetRetCode(404); // not found

#ifdef OTA_LATEST_FIRMWARE_JSON_URL
	Url url;
	url.Parse(OTA_LATEST_FIRMWARE_JSON_URL);

	ESP_LOGD(tag, "Retrieve json from: %s", url.GetUrl().c_str());
	WebClient webClient;
	webClient.Prepare(&url);

	unsigned short statuscode = webClient.HttpGet();
    if (statuscode != 200)
		return false;
	int i = webClient.GetResponseData().indexOf("\"version\":\"");
	if (i <= 0)
		return false;
	String version = webClient.GetResponseData().substring(i + 11);
	i = version.indexOf('"');
	if (i <= 0)
		return false;
	version = version.substring(0, i);

	if (!version.equalsIgnoreCase(FIRMWARE_VERSION)){
		sBody = "{\"newversion\":\"Firmware available: ";
		sBody += version;
		sBody += "\"}";
	}
	else
#endif
		sBody = "{}";
	response.SetRetCode(200);
	mpUfo->dt.leaveAction(dtHandleRequest);
	return response.Send(sBody);
	
}