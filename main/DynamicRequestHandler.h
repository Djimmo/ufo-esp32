#ifndef MAIN_DYNAMICREQUESTHANDLER_H_
#define MAIN_DYNAMICREQUESTHANDLER_H_

#include "UrlParser.h"
#include "HttpResponse.h"
#include <list>

class Ufo;
class DisplayCharter;

class DynamicRequestHandler {
public:
	DynamicRequestHandler(Ufo* pUfo) : mpUfo(pUfo) {}
	virtual ~DynamicRequestHandler() {}

	String HandleApiRequest(std::list<TParam>& params);
	bool HandleApiListRequest(std::list<TParam>& params, HttpResponse& rResponse);
	bool HandleApiEditRequest(std::list<TParam>& params, HttpResponse& rResponse);
	bool HandleInfoRequest(std::list<TParam>& params, HttpResponse& rResponse);
	bool HandleConfigRequest(std::list<TParam>& params, HttpResponse& rResponse);
	bool HandleMqttConfigRequest(std::list<TParam>& params, HttpResponse& rResponse);
	bool HandleUfoConfigRequest(std::list<TParam>& params, HttpResponse& rResponse);
	bool HandleSrvConfigRequest(std::list<TParam>& params, HttpResponse& rResponse);
	bool HandleFirmwareRequest(std::list<TParam>& params, HttpResponse& response);
	bool HandleCheckFirmwareRequest(std::list<TParam>& params, HttpResponse& response);
	bool HandleDynatraceIntegrationRequest(std::list<TParam>& params, HttpResponse& response);
	bool HandleDynatraceMonitoringRequest(std::list<TParam>& params, HttpResponse& response);

	bool ShouldRestart() { return mbRestart; }

private:
	Ufo* mpUfo;

	bool mbRestart = false;
};

#endif /* MAIN_DYNAMICREQUESTHANDLER_H_ */
