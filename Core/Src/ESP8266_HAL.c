/*
 * ESP8266_HAL.c
 *
 *  Created on: 2024-10-19
 *      Author: Greenhouse - Knowit
 *      (used code from Controllerstech)
 */

#include "UartRingbuffer_multi.h"
#include "ESP8266_HAL.h"
#include "stdio.h"
#include "string.h"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef  htim1;
extern TIM_HandleTypeDef  htim2;

#define wifi_uart &huart1
#define pc_uart &huart2

char buffer[20];

char *Basic_inclusion = "<!DOCTYPE html> <html>\n<head><meta name=\"viewport\"\
		content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n\
		<title>DRIVE CONTROL</title>\n<style>html { font-family: Helvetica; \
		display: inline-block; margin: 0px auto; text-align: center;}\n\
		body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\
		h3 {color: #444444;margin-bottom: 50px;}\n\
		.button1 {display: block; \
		width: 150px;background-color: #1abc9c;border: none;color: white; \
		padding: 13px 30px;text-decoration: none;font-size: 25px; \
		margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n\
		.button2 {display: block; \
		width: 150px;background-color: #1abc9c;border: none;color: white;\
		padding: 13px 30px;text-decoration: none;font-size: 25px;\
		margin: 10px auto 35px;cursor: pointer;border-radius: 4px;}\n\
		.button3 {display: block; \
		width: 150px;background-color: #1abc9c;border: none;color: white;\
		padding: 13px 30px;text-decoration: none;font-size: 25px;\
		margin: 10px auto 35px;cursor: pointer;border-radius: 4px;}\n\
		.button4 {display: block;\
		width: 150px;background-color: #1abc9c;border: none;color: white;\
		padding: 13px 30px;text-decoration: none;font-size: 25px;\
		margin: 10px auto 35px;cursor: pointer;border-radius: 4px;}\n\
		.button-on {background-color: #1abc9c;}\n.button-on:active \
		{background-color: #16a085;}\n.button-off {background-color: #34495e;}\n\
        .button-off:active {background-color: #2c3e50;}\np {font-size: 14px;color: #888; margin-bottom: 10px;}\n\
        </style>\n</head>\n<body>\n<h1>KRAC CONTROL</h1>\n";

char *LEFT_OFF   = "<a class=\"button1 button-off\" href=\"/l\">LEFT</a>";
char *MIDDLE_OFF = "<a class=\"button2 button-off\" href=\"/m\">MIDDLE</a>";
char *RIGHT_OFF  = "<a class=\"button3 button-off\" href=\"/r\">RIGHT</a>";
char *STOP_OFF   = "<a class=\"button4 button-off\" href=\"/s\">STOP</a>";
char *DRIVE_OFF  = "<a class=\"button1 button-off\" href=\"/d\">DRIVE</a>";
char *BACK_OFF   = "<a class=\"button2 button-off\" href=\"/b\">BACK</a>";
//char *DRIVE      = "<a class=\"button1 button-off\" href=\"/s\">STOP</a>";
//char *BACK       = "<a class=\"button2 button-off\" href=\"/s\">STOP</a>";
char *STOP       = "<a class=\"button1 button-off\" href=\"/d\">DRIVE</a> \n\
			        <a class=\"button2 button-off\" href=\"/b\">BACK</a>";
char *LEFT       = "<a class=\"button1 button-on\" href=\"/l\">LEFT</a>";
char *MIDDLE     = "<a class=\"button2 button-on\" href=\"/m\">MIDDLE</a>";
char *RIGHT      = "<a class=\"button3 button-on\" href=\"/r\">RIGHT</a>";

//char *STOP_OFF  = "<p>Status: OFF</p><a class=\"button2 button-off\" href=\"/d\">STOP</a>";
//char *STOP  = "<p>Status: ON</p><a class=\"button2 button-on\" href=\"/d\">STOP</a>";

char *Terminate = "</body></html>";

/*****************************************************************************************************************************************/

void ESP_Init (char *SSID, char *PASSWD)
{
	char data[80];

	Ringbuf_init();

	Uart_sendstring("AT+RST\r\n", wifi_uart);
	Uart_sendstring("RESETTING.", pc_uart);
	for (int i=0; i<5; i++)
	{
		Uart_sendstring(".", pc_uart);
		HAL_Delay(1000);
	}

	/********* AT **********/
	Uart_flush(wifi_uart);
	Uart_sendstring("AT\r\n", wifi_uart);
	while(!(Wait_for("OK\r\n", wifi_uart)));
	Uart_sendstring("AT---->OK\n\n", pc_uart);

	/********* AT+CWMODE=1 **********/
	Uart_flush(wifi_uart);
	Uart_sendstring("AT+CWMODE=1\r\n", wifi_uart);
	while (!(Wait_for("OK\r\n", wifi_uart)));
	Uart_sendstring("CW MODE---->1\n\n", pc_uart);

	/********* AT+CWJAP="SSID","PASSWD" **********/
	Uart_flush(wifi_uart);
	Uart_sendstring("connecting... to the provided AP\n", pc_uart);
	sprintf (data, "AT+CWJAP=\"%s\",\"%s\"\r\n", SSID, PASSWD);
	Uart_sendstring(data, wifi_uart);
	while (!(Wait_for("OK\r\n", wifi_uart)));
	sprintf (data, "Connected to,\"%s\"\n\n", SSID);
	Uart_sendstring(data,pc_uart);

	/********* AT+CIFSR **********/
	Uart_flush(wifi_uart);
	Uart_sendstring("AT+CIFSR\r\n", wifi_uart);
	while (!(Wait_for("CIFSR:STAIP,\"", wifi_uart)));
	while (!(Copy_upto("\"",buffer, wifi_uart)));
	while (!(Wait_for("OK\r\n", wifi_uart)));
	int len = strlen (buffer);
	buffer[len-1] = '\0';
	sprintf (data, "IP ADDR: %s\n\n", buffer);
	Uart_sendstring(data, pc_uart);

	/********* AT+CIPMUX **********/
	Uart_flush(wifi_uart);
	Uart_sendstring("AT+CIPMUX=1\r\n", wifi_uart);
	while (!(Wait_for("OK\r\n", wifi_uart)));
	Uart_sendstring("CIPMUX---->OK\n\n", pc_uart);

	/********* AT+CIPSERVER **********/
	Uart_flush(wifi_uart);
	Uart_sendstring("AT+CIPSERVER=1,80\r\n", wifi_uart);
	while (!(Wait_for("OK\r\n", wifi_uart)));
	Uart_sendstring("CIPSERVER---->OK\n\n", pc_uart);

	Uart_sendstring("Now Connect to the IP ADRESS\n\n", pc_uart);
}

int Server_Send (char *str, int Link_ID)
{
	int len = strlen (str);
	char data[80];
	sprintf (data, "AT+CIPSEND=%d,%d\r\n", Link_ID, len);
	Uart_sendstring(data, wifi_uart);
	while (!(Wait_for(">", wifi_uart)));
	Uart_sendstring (str, wifi_uart);
	while (!(Wait_for("SEND OK", wifi_uart)));
	sprintf (data, "AT+CIPCLOSE=5\r\n");
	Uart_sendstring(data, wifi_uart);
	while (!(Wait_for("OK\r\n", wifi_uart)));
	return 1;
}

void Server_Handle (char *str, int Link_ID)
{
	char datatosend[2048] = {0};

	sprintf (datatosend, Basic_inclusion);

	if (!(strcmp (str, "/l")))
	{
		strcat(datatosend, LEFT);
		strcat(datatosend, MIDDLE_OFF);
		strcat(datatosend, RIGHT_OFF);
		strcat(datatosend, STOP_OFF);
		strcat(datatosend, Terminate);
		//Server_Send(datatosend, Link_ID);
	}
	else if (!(strcmp (str, "/r")))
	{
		strcat(datatosend, LEFT_OFF);
		strcat(datatosend, MIDDLE_OFF);
		strcat(datatosend, RIGHT);
		strcat(datatosend, STOP_OFF);
		strcat(datatosend, Terminate);
		//Server_Send(datatosend, Link_ID);
	}
	else if (!(strcmp (str, "/m")))
	{
		strcat(datatosend, LEFT_OFF);
		strcat(datatosend, MIDDLE);
		strcat(datatosend, RIGHT_OFF);
		strcat(datatosend, STOP_OFF);
		strcat(datatosend, Terminate);
		//Server_Send(datatosend, Link_ID);
	}
	else if (!(strcmp (str, "/d")) ||
			 !(strcmp (str, "/b")))
	{
		strcat(datatosend, LEFT_OFF);
		strcat(datatosend, MIDDLE);
		strcat(datatosend, RIGHT_OFF);
		strcat(datatosend, STOP_OFF);
		strcat(datatosend, Terminate);
		//Server_Send(datatosend, Link_ID);

	} else {
		strcat(datatosend, STOP);
		strcat(datatosend, Terminate);
		//Server_Send(datatosend, Link_ID);
	}
	Server_Send(datatosend, Link_ID);
}

void Server_Start (void)
{
	//PWM FOR MOTOR DRIVE
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	//PWM FOR SERVO
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);

	char buftocopyinto[64] = {0};
	char Link_ID;

	while (!(Get_after("+IPD,", 1, &Link_ID, wifi_uart)));
	Link_ID -= 48;

	while (!(Copy_upto(" HTTP/1.1", buftocopyinto, wifi_uart)));

	if (Look_for("/l", buftocopyinto) == 1) //TURN LEFT
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, 1); //LD2
		__HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_2, 90);
		Server_Handle("/l",Link_ID);
	}
	else if (Look_for("/m", buftocopyinto) == 1) //TURN MIDDLE
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, 0); //LD2
		__HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_2, 75);
		Server_Handle("/m",Link_ID);
	}
	else if (Look_for("/r", buftocopyinto) == 1) //TURN RIGHT
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, 1); //LD2
		__HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_2, 60);
		Server_Handle("/r",Link_ID);
	}
	else if (Look_for("/b", buftocopyinto) == 1) //BACKWARD DRIVE
	{
	    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, 0);
		__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_1, 30);
	    Server_Handle("/b",Link_ID);
	}
	else if (Look_for("/d", buftocopyinto) == 1) //FORWARD DRIVE
	{
	    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, 1);
		__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_1, 30);
		Server_Handle("/d",Link_ID);

	}
	else if (Look_for("/favicon.ico", buftocopyinto) == 1);
	else // STOP VEHICLE
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, 0); //LD2
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, 0);
		__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_1, 0);
		// RESTORE STEERING
		__HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_2, 75);
		Server_Handle("/s",Link_ID);
	}
}

