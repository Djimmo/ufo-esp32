#include "UfoWebServer.h"
#include "Ufo.h"
#include "Config.h"
#include "DynamicRequestHandler.h"
#include "HttpRequestParser.h"
#include "HttpResponse.h"
#include <lwip/sockets.h>
#include <esp_log.h>
#include <esp_system.h>
#include <mbedtls/base64.h>

#include "sdkconfig.h"
#include "fontwoff.h"
#include "fontttf.h"
#include "fontsvg.h"
#include "fonteot.h"
#include "indexhtml.h"

static char tag[] = "UfoWebServer";

//------------------------------------------------------------------

UfoWebServer::UfoWebServer() {
	mbRestart = false;
	SetUploadHandler(&mOta);
}

UfoWebServer::~UfoWebServer() {
}

bool UfoWebServer::StartUfoServer(){

	if (!mpUfo->GetConfig().msAdminPw.empty()) {
		mAuthHeader.printf("WWW-Authenticate: Basic realm=\"%s\"", mpUfo->GetConfig().msHostname.c_str());
		mAdminAuth.printf("admin:%s", mpUfo->GetConfig().msAdminPw.c_str());
	}

	if (mpUfo->GetConfig().mbAPMode)
		return Start(80, false, NULL);	
	
	__uint16_t port; 
	if (mpUfo->GetConfig().muWebServerPort)
		port = mpUfo->GetConfig().muWebServerPort;
	else
		port = mpUfo->GetConfig().mbWebServerUseSsl ? 443 : 80;
	return Start(port, mpUfo->GetConfig().mbWebServerUseSsl, &(mpUfo->GetConfig().msWebServerCert));		
}


bool UfoWebServer::HandleRequest(HttpRequestParser& httpParser, HttpResponse& httpResponse){

	DynamicRequestHandler requestHandler(mpUfo->CreateRequestHandler());

	if (httpParser.GetUrl().equals("/api")){
		String sBody{ requestHandler.HandleApiRequest(httpParser.GetParams()) };
		httpResponse.AddHeader(HttpResponse::HeaderNoCache);
		httpResponse.SetRetCode(200);
		return httpResponse.Send(sBody);
	}
	if (!mAuthHeader.empty()) {
		httpResponse.AddHeader(mAuthHeader.c_str());
		if (!checkAuthAdmin(httpParser)) {
			const char* sBody = "<!DOCTYPE html>\n<html><head>\n<title>401 Unauthorized</title>\n"
				"</head><body>\n<h1>Unauthorized</h1>\n<p>This server could not verify that you "
				"are authorized to access the document requested.  Either you supplied the wrong "
				"credentials (e.g., bad password), or your browser doesn't understand how to supply "
				"the credentials required.</p>\n</body></html>";
			httpResponse.AddHeader(HttpResponse::HeaderNoCache);
			httpResponse.SetRetCode(401);
			return httpResponse.Send(sBody, strlen(sBody));
		}
	}
	if (httpParser.GetUrl().equals("/") || httpParser.GetUrl().equals("/index.html")){
		httpResponse.AddHeader(HttpResponse::HeaderContentTypeHtml);
		httpResponse.AddHeader("Content-Encoding: gzip");
		if (!httpResponse.Send(indexhtml_h, sizeof(indexhtml_h)))
			return false;
	}
	else if (httpParser.GetUrl().equals("/fonts/material-design-icons.woff")){
		httpResponse.AddHeader(HttpResponse::HeaderContentTypeBinary);
		if (!httpResponse.Send(fontwoff_h, sizeof(fontwoff_h)))
			return false;
	}
	else if (httpParser.GetUrl().equals("/fonts/material-design-icons.ttf")){
		httpResponse.AddHeader(HttpResponse::HeaderContentTypeBinary);
		if (!httpResponse.Send(fontttf_h, sizeof(fontttf_h)))
			return false;
	}
	else if (httpParser.GetUrl().equals("/fonts/material-design-icons.eot")){
		httpResponse.AddHeader(HttpResponse::HeaderContentTypeBinary);
		if (!httpResponse.Send(fonteot_h, sizeof(fonteot_h)))
			return false;
	}
	else if (httpParser.GetUrl().equals("/fonts/material-design-icons.svg")){
		httpResponse.AddHeader(HttpResponse::HeaderContentTypeBinary);
		if (!httpResponse.Send(fontsvg_h, sizeof(fontsvg_h)))
			return false;
	}
	else if (httpParser.GetUrl().equals("/dynatraceintegration")){
		if (!requestHandler.HandleDynatraceIntegrationRequest(httpParser.GetParams(), httpResponse))
			return false;
	}
	else if (httpParser.GetUrl().equals("/dynatracemonitoring")){
		if (!requestHandler.HandleDynatraceMonitoringRequest(httpParser.GetParams(), httpResponse))
			return false;
	}
	else if (httpParser.GetUrl().equals("/apilist")){
		if (!requestHandler.HandleApiListRequest(httpParser.GetParams(), httpResponse))
			return false;
	}
	else if (httpParser.GetUrl().equals("/apiedit")){
		if (!requestHandler.HandleApiEditRequest(httpParser.GetParams(), httpResponse))
			return false;
	}
	else if (httpParser.GetUrl().equals("/info")){
		if (!requestHandler.HandleInfoRequest(httpParser.GetParams(), httpResponse))
			return false;
	}
	else if (httpParser.GetUrl().equals("/config")){
		if (!requestHandler.HandleConfigRequest(httpParser.GetParams(), httpResponse))
			return false;
	} 
	else if (httpParser.GetUrl().equals("/mqttconfig")){
		if (!requestHandler.HandleMqttConfigRequest(httpParser.GetParams(), httpResponse))
			return false;
	} 
	else if (httpParser.GetUrl().equals("/ufoconfig"))
	{
		if (!requestHandler.HandleUfoConfigRequest(httpParser.GetParams(), httpResponse))
			return false;
	}
	else if (httpParser.GetUrl().equals("/srvconfig")){
		if (!requestHandler.HandleSrvConfigRequest(httpParser.GetParams(), httpResponse))
			return false;
	} 
	else if (httpParser.GetUrl().equals("/firmware")) {
		if (!requestHandler.HandleFirmwareRequest(httpParser.GetParams(), httpResponse))
			return false;
	}
	else if (httpParser.GetUrl().equals("/checkfirmware")) {
		if (!requestHandler.HandleCheckFirmwareRequest(httpParser.GetParams(), httpResponse))
			return false;
	}
	else if (httpParser.GetUrl().equals("/update")) {
		String sBody = "<html><head><title>SUCCESS - firmware update succeeded, rebooting shortly.</title>"
						"<meta http-equiv=\"refresh\" content=\"10; url=/\"></head><body>"
						"<h2>SUCCESS - firmware update succeeded, rebooting shortly.</h2></body></html>";
		if (!httpResponse.Send(sBody))
			return false;
	}

	else if (httpParser.GetUrl().equals("/test")){
		String sBody;
		sBody = httpParser.IsGet() ? "GET " : "POST ";
		sBody += httpParser.GetUrl();
		sBody += httpParser.IsHttp11() ? " HTTP/1.1" : "HTTP/1.0";
		sBody += "\r\n";
		std::list<TParam> params = httpParser.GetParams();
		std::list<TParam>::iterator it = params.begin();
		while (it != params.end()){
			sBody += (*it).paramName;
			sBody += " = ";
			sBody += (*it).paramValue;
			sBody += "\r\n";
			it++;
		}
		if (!httpParser.IsGet()){
			sBody += "Boundary:<";
			sBody += httpParser.GetBoundary();
			sBody += ">\r\n";
			sBody += "Body:\r\n";
			sBody += httpParser.GetBody();
		}
		if (!httpResponse.Send(sBody))
			return false;
	}
	else{
		httpResponse.SetRetCode(404);
		if (!httpResponse.Send(NULL, 0))
			return false;
	}

	if (mbRestart || requestHandler.ShouldRestart() || (mOta.GetProgress() == OTA_PROGRESS_FINISHEDSUCCESS)){
		ESP_LOGI(tag, "RESTARTING!");
		vTaskDelay(100);
		esp_restart();
	}

	return true;
}

bool UfoWebServer::checkAuthAdmin(HttpRequestParser& httpParser) {
	if (mpUfo->GetConfig().msAdminPw.empty())
		return true;
	ESP_LOGD(tag, "authorization: '%s'", httpParser.Authorization().c_str());
	size_t authlen = httpParser.Authorization().length(), decodedlen = authlen;
	if (authlen < 7)
		return false;
	String decoded;
	decoded.resize(authlen);
	int rc;
	if ((rc = mbedtls_base64_decode((unsigned char*)decoded.data(), authlen, &decodedlen,
			(const unsigned char*)httpParser.Authorization().c_str() + 6, authlen - 6)) != 0) {
		ESP_LOGD(tag, "base64_decode failed: %d", rc);
		return false;
	}
	decoded.resize(decodedlen);
	ESP_LOGD(tag, "base64 decoded: '%s', expected: '%s', match: %d", decoded.c_str(), mAdminAuth.c_str(), mAdminAuth == decoded);
	return mAdminAuth == decoded;
}
