#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Flight.h"
#include "fileHelper.h"

void	initFlight(Flight* pFlight, const AirportManager* pManager)
{
	Airport* pPortOr = setAiportToFlight(pManager, "Enter name of origin airport:");
	pFlight->nameSource = _strdup(pPortOr->name);
	int same;
	Airport* pPortDes;
	do {
		pPortDes = setAiportToFlight(pManager, "Enter name of destination airport:");
		same = isSameAirport(pPortOr, pPortDes);
		if (same)
			printf("Same origin and destination airport\n");
	} while (same);
	pFlight->nameDest = _strdup(pPortDes->name);
	initPlane(&pFlight->thePlane);
	getCorrectDate(&pFlight->date);
}

int		isFlightFromSourceName(const Flight* pFlight, const char* nameSource)
{
	if (strcmp(pFlight->nameSource, nameSource) == 0)
		return 1;
		
	return 0;
}


int		isFlightToDestName(const Flight* pFlight, const char* nameDest)
{
	if (strcmp(pFlight->nameDest, nameDest) == 0)
		return 1;

	return 0;


}

int		isPlaneCodeInFlight(const Flight* pFlight, const char*  code)
{
	if (strcmp(pFlight->thePlane.code, code) == 0)
		return 1;
	return 0;
}

int		isPlaneTypeInFlight(const Flight* pFlight, ePlaneType type)
{
	if (pFlight->thePlane.type == type)
		return 1;
	return 0;
}


void	printFlight(const Flight* pFlight)
{
	printf("Flight From %s To %s\t",pFlight->nameSource, pFlight->nameDest);
	printDate(&pFlight->date);
	printPlane(&pFlight->thePlane);
}

void	printFlightV(const void* val)
{
	const Flight* pFlight = *(const Flight**)val;
	printFlight(pFlight);
}


Airport* setAiportToFlight(const AirportManager* pManager, const char* msg)
{
	char name[MAX_STR_LEN];
	Airport* port;
	do
	{
		printf("%s\t", msg);
		myGets(name, MAX_STR_LEN,stdin);
		port = findAirportByName(pManager, name);
		if (port == NULL)
			printf("No airport with this name - try again\n");
	} while(port == NULL);

	return port;
}

void	freeFlight(Flight* pFlight)
{
	free(pFlight->nameSource);
	free(pFlight->nameDest);
	free(pFlight);
}


int saveFlightToFile(const Flight* pF, FILE* fp)
{
	if (!writeStringToFile(pF->nameSource, fp, "Error write flight source name\n"))
		return 0;

	if (!writeStringToFile(pF->nameDest, fp, "Error write flight destination name\n"))
		return 0;

	if (!savePlaneToFile(&pF->thePlane,fp))
		return 0;

	if (!saveDateToFile(&pF->date,fp))
		return 0;

	return 1;
}

int freadFlightFromCompressFile(Flight * flight, FILE* f, unsigned char * data)
{
	int len1 = (data[0] >> 3);
	flight->nameSource = (char*)calloc(len1 + 1, sizeof(char));
	if (!flight->nameSource)
		return 0;

	int len2 = ((data[0] & 0x7) << 2) | (data[1] >> 6);
	flight->nameDest = (char*)calloc(len2 + 1, sizeof(char));
	if (!flight->nameDest)
		return 0;

	flight->thePlane.type = ((data[1] & 0x30) >> 4);
	flight->date.month = (data[1] & 0xF);

	char* code = flight->thePlane.code;
	code = (char*)malloc(4 * sizeof(char) + 1);
	BYTE code1 = ((data[2] & 0xF8) >> 3) + 65;
	BYTE code2 = ((data[2] & 0x7) << 2) | ((data[3] & 0xC0) >> 6) + 65;
	BYTE code3 = ((data[3] & 0x3E) >> 1) + 65;
	BYTE code4 = ((data[3] & 0x01) << 4) | ((data[4] & 0xF0) >> 4) + 65;
	code[0] = code1;
	code[1] = code2;
	code[2] = code3;
	code[3] = code4;
	code[4] = '\0';
	strcpy(flight->thePlane.code, code);
	flight->date.year = (data[4] & 0x0F) + 2021;
	flight->date.day = (data[5] & 0x1F);
	if (!fread(flight->nameSource, sizeof(BYTE), len1, f))
	{
		return 0;
	}

	if (!fread(flight->nameDest, sizeof(BYTE), len2, f))
	{
		return 0;
	}
	return 1;
}

int fwriteFlightFromCompressFile(Flight * flight, FILE* f, unsigned char * data)
{
	data[0] = (BYTE)((strlen(flight->nameSource) << 3) | (strlen(flight->nameDest) >> 2));
	data[1] = (BYTE)(((strlen(flight->nameDest) & 0x3) << 6) | ((flight->thePlane.type) << 4) | (flight->date.month));
	data[2] = (BYTE)(((flight->thePlane.code[0] - 65) << 3) |
		((flight->thePlane.code[1] - 65) >> 2));
	data[3] = (BYTE)((((flight->thePlane.code[1] - 65) & 0x3) << 6) |
		((flight->thePlane.code[2] - 65) << 1) |
		((flight->thePlane.code[3] - 65) >> 4));
	data[4] = (BYTE)(((flight->thePlane.code[3] - 65) << 4) | (flight->date.year - 2021));
	data[5] = (BYTE)((flight->date.day));

	if (!fwrite(data, sizeof(BYTE), 6, f))
	{
		return 0;
	}
	if (!fwrite(flight->nameSource, sizeof(BYTE), strlen(flight->nameSource), f))
	{
		return 0;
	}

	if (!fwrite(flight->nameDest, sizeof(BYTE), strlen(flight->nameDest), f))
	{
		return 0;
	}
	return 1;
}


int loadFlightFromFile(Flight* pF, const AirportManager* pManager, FILE* fp)
{

	pF->nameSource = readStringFromFile(fp, "Error reading source name\n");
	if (!pF->nameSource)
		return 0;

	pF->nameDest = readStringFromFile(fp, "Error reading destination name\n");
	if (!pF->nameDest)
		return 0;

	if (!loadPlaneFromFile(&pF->thePlane, fp))
		return 0;

	if (!loadDateFromFile(&pF->date, fp))
		return 0;

	return 1;
}

int	compareFlightBySourceName(const void* flight1, const void* flight2)
{
	const Flight* pFlight1 = *(const Flight**)flight1;
	const Flight* pFlight2 = *(const Flight**)flight2;
	return strcmp(pFlight1->nameSource, pFlight2->nameSource);
}

int	compareFlightByDestName(const void* flight1, const void* flight2)
{
	const Flight* pFlight1 = *(const Flight**)flight1;
	const Flight* pFlight2 = *(const Flight**)flight2;
	return strcmp(pFlight1->nameDest, pFlight2->nameDest);
}

int	compareFlightByPlaneCode(const void* flight1, const void* flight2)
{
	const Flight* pFlight1 = *(const Flight**)flight1;
	const Flight* pFlight2 = *(const Flight**)flight2;
	return strcmp(pFlight1->thePlane.code, pFlight2->thePlane.code);
}

int		compareFlightByDate(const void* flight1, const void* flight2)
{
	const Flight* pFlight1 = *(const Flight**)flight1;
	const Flight* pFlight2 = *(const Flight**)flight2;


	return compareDate(&pFlight1->date, &pFlight2->date);
	

	return 0;
}

