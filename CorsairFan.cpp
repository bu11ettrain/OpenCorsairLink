#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CorsairLinkProto.h"
#include "CorsairLink.h"
#include "CorsairFan.h"

extern CorsairLink *cl;

CorsairFan::CorsairFan()
{
}

void CorsairFan::PrintInfo(CorsairFanInfo fan)
{
	fprintf(stdout, "%s:\n", fan.Name );
	fprintf(stdout, "\tMode: %s\n", GetFanModeString(fan.Mode) );
	fprintf(stdout, "\tRPM: %i\n", fan.RPM );
}

char* CorsairFan::GetFanModeString(int mode)
{
	char *modeString = NULL;
	switch(mode){
		case FixedPWM:
			asprintf(&modeString,"Fixed PWM");
			break;
		case FixedRPM:
			asprintf(&modeString, "Fixed RPM");
			break;
		case Default:
			asprintf(&modeString, "Default");
			break;
		case Quiet:
			asprintf(&modeString, "Quiet");
			break;
		case Balanced:
			asprintf(&modeString, "Balanced");
			break;
		case Performance:
			asprintf(&modeString, "Performance");
			break;
		case Custom:
			asprintf(&modeString, "Custom");
			break;
		default:
			asprintf(&modeString, "N/A (%02X)", mode );
			break;
	}
	return modeString;
}

int CorsairFan::ConnectedFans() {
	int fans = 0, i = 0, fanMode = 0;

	for (i = 0; i < 5; i++) {
		memset(cl->buf,0x00,sizeof(cl->buf));
		// Read fan Mode
		cl->buf[0] = 0x07; // Length
		cl->buf[1] = cl->CommandId++; // Command ID
		cl->buf[2] = WriteOneByte; // Command Opcode
		cl->buf[3] = FAN_Select; // Command data...
		cl->buf[4] = i; // select fan
		cl->buf[5] = cl->CommandId++; // Command ID
		cl->buf[6] = ReadOneByte; // Command Opcode
		cl->buf[7] = FAN_Mode; // Command data...

		int res = hid_write(cl->handle, cl->buf, 11);
		if (res < 0) {
			fprintf(stderr, "Error: Unable to write() %s\n", (char*)hid_error(cl->handle) );
		}

		res = cl->hid_read_wrapper(cl->handle, cl->buf);
		if (res < 0) {
			fprintf(stderr, "Error: Unable to read() %s\n", (char*)hid_error(cl->handle) );
		}
		fanMode = cl->buf[4];

		if(fanMode != 0x03){
			fans++;
		}
	}

	return fans;
}

void CorsairFan::ReadFanInfo(int fanIndex, CorsairFanInfo *fan){
	int res = 0;

	if(fanIndex < 4){
		asprintf(&fan->Name, "Fan %i", fanIndex + 1);
	}
	else {
		asprintf(&fan->Name, "Pump");
	}

	memset(cl->buf,0x00,sizeof(cl->buf));
	// Read fan Mode
	cl->buf[0] = 0x07; // Length
	cl->buf[1] = cl->CommandId++; // Command ID
	cl->buf[2] = WriteOneByte; // Command Opcode
	cl->buf[3] = FAN_Select; // Command data...
	cl->buf[4] = fanIndex; // select fan
	cl->buf[5] = cl->CommandId++; // Command ID
	cl->buf[6] = ReadOneByte; // Command Opcode
	cl->buf[7] = FAN_Mode; // Command data...
	
	res = hid_write(cl->handle, cl->buf, 11);
	if (res < 0) {
		fprintf(stderr, "Error: Unable to write() %s\n", (char*)hid_error(cl->handle) );
	}

	res = cl->hid_read_wrapper(cl->handle, cl->buf);
	if (res < 0) {
		fprintf(stderr, "Error: Unable to read() %s\n", (char*)hid_error(cl->handle) );
	}
	fan->Mode = cl->buf[4] & 0x0E;

	memset(cl->buf,0x00,sizeof(cl->buf));
	// Read fan RPM
	cl->buf[0] = 0x07; // Length
	cl->buf[1] = cl->CommandId++; // Command ID
	cl->buf[2] = WriteOneByte; // Command Opcode
	cl->buf[3] = FAN_Select; // Command data...
	cl->buf[4] = fanIndex; // select fan
	cl->buf[5] = cl->CommandId++; // Command ID
	cl->buf[6] = ReadTwoBytes; // Command Opcode
	cl->buf[7] = FAN_ReadRPM; // Command data...

	res = hid_write(cl->handle, cl->buf, 11);
	if (res < 0) {
		fprintf(stderr, "Error: Unable to write() %s\n", (char*)hid_error(cl->handle) );
	}

	res = cl->hid_read_wrapper(cl->handle, cl->buf);
	//All data is little-endian.
	int rpm = cl->buf[5] << 8;
	rpm += cl->buf[4];

	fan->RPM = rpm;
}

int CorsairFan::SetFansInfo(int fanIndex, CorsairFanInfo fanInfo){
	memset(cl->buf,0x00,sizeof(cl->buf));

	if(fanInfo.Mode == FixedPWM || fanInfo.Mode == FixedRPM
		|| fanInfo.Mode == Default || fanInfo.Mode == Quiet
		|| fanInfo.Mode == Balanced	|| fanInfo.Mode == Performance
		|| fanInfo.Mode == Custom) {

		cl->buf[0] = 0x0b; // Length
		cl->buf[1] = cl->CommandId++; // Command ID
		cl->buf[2] = WriteOneByte; // Command Opcode
		cl->buf[3] = FAN_Select; // Command data...
		cl->buf[4] = fanIndex; // select fan
		cl->buf[5] = cl->CommandId++; // Command ID
		cl->buf[6] = WriteOneByte; // Command Opcode
		cl->buf[7] = FAN_Mode; // Command data...
		cl->buf[8] = fanInfo.Mode;
		cl->buf[9] = cl->CommandId++; // Command ID
		cl->buf[10] = ReadOneByte; // Command Opcode
		cl->buf[11] = FAN_Mode; // Command data...

		int res = hid_write(cl->handle, cl->buf, 17);
		if (res < 0) {
			fprintf(stderr, "%s", (char*)hid_error(cl->handle) );
			return 1;
		}

		res = cl->hid_read_wrapper(cl->handle, cl->buf);
		if (res < 0) {
			fprintf(stderr, "%s", (char*)hid_error(cl->handle) );
			return 1;
		}
		if(fanInfo.Mode != cl->buf[6]){
			fprintf(stderr, "Cannot set fan mode.");
			return 1;
		}
	} else {
		fprintf(stderr, "Invalid fan mode." );
		return 1;
	}
	if(fanInfo.RPM != 0) {
		memset(cl->buf,0x00,sizeof(cl->buf));

		cl->buf[0] = 0x0b; // Length
		cl->buf[1] = cl->CommandId++; // Command ID
		cl->buf[2] = WriteOneByte; // Command Opcode
		cl->buf[3] = FAN_Select; // Command data...
		cl->buf[4] = fanIndex; // select fan
		cl->buf[5] = cl->CommandId++; // Command ID
		cl->buf[6] = WriteTwoBytes; // Command Opcode
		cl->buf[7] = FAN_FixedRPM; // Command data...
		cl->buf[8] = fanInfo.RPM & 0x00FF;
		cl->buf[9] = fanInfo.RPM >> 8;
		cl->buf[10] = cl->CommandId++; // Command ID
		cl->buf[11] = ReadTwoBytes; // Command Opcode
		cl->buf[12] = FAN_ReadRPM; // Command data...

		int res = hid_write(cl->handle, cl->buf, 18);
		if (res < 0) {
			fprintf(stderr, "%s", (char*)hid_error(cl->handle) );
			return 1;
		}

		res = cl->hid_read_wrapper(cl->handle, cl->buf);
		if (res < 0) {
			fprintf(stderr, "%s", (char*)hid_error(cl->handle) );
			return 1;
		}
		//All data is little-endian.
		int rpm = cl->buf[5] << 8;
		rpm += cl->buf[4];
		if(fanInfo.RPM != rpm){
			fprintf(stderr, "Cannot set fan RPM.");
			return 1;
		}
	}

	return 0;
}

CorsairFan::~CorsairFan(){
	
}