/*
 * udpclient.c
 *
 *  Created on: Dec 21, 2025
 *      Author: Rama Syafrizal
 *
 *      modify from controllertech
 */


/* ===================== Includes ===================== */
#include "cmsis_os.h"

#include "lwip/opt.h"
#include "lwip/api.h"
#include "lwip/sys.h"
#include "lwip.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"
#include "functionMath.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "main.h"
#include "udpclient.h"
#include "DCMotorControl.h"
#include "stepper_drv8825.h"

/* ===================== Static Objects ===================== */
static struct netconn *conn;
static struct netconn *led_conn;
static struct netbuf  *rxbuf, *buf;
static struct udp_pcb *upcb;
struct pbuf *prx;
u16_t len;

//static struct udp_pcb *upcb_rx;
//static struct udp_pcb *upcb_tx;
static ip_addr_t destIP;

/* ===================== Global Variables ===================== */
//int indx = 0;

//char smsg[200];
//uint32_t buffer = 0;

uint16_t data1,data2;
uint16_t sequence = 0;
float data_trq = 0.0f;
float data_Vx = 0.0f;
float data_Vx_Wheel = 0.0f;
float data_Vx_Thrl = 0.0f;
float data_Steer_Wheel = 0.0f;
uint32_t timestamp = 0;
int32_t data_Mzd = 0;
uint16_t adcData = 0;
volatile uint8_t motorQtControl = 0;
volatile float motorQtReff = 0.0f;

/* ============ Qt STEPPER CONTROL VARIABLES ============ */
volatile uint8_t stepperQtControl = 0;
volatile float stepperQtAngle = 0.0f;

//Latency
int Latency = 0;
double Latency_s = 0.0f;

/* ===================== Application State ===================== */
EthMode_t      currEthMode = SEND_TO_VCU;
CobaData	   cd;
VCUToCorner    v2c;
VCUToCorner_Latency v2c_l;
Latency_sendBack lsb;
Latency_send ls;
Micro2Micro    m2m;
PCToVCU_Latency p2v;

/* ===================== Function Prototypes ===================== */
void udpsend(char *data);
void udpsend_hex(const void *data, uint16_t len);


void HIL_ETH_ToggleMode(void) {
    if (currEthMode == SEND_TO_MATLAB)
        currEthMode = SEND_TO_VCU;
    else
        currEthMode = SEND_TO_MATLAB;
}

static void udpinit_thread(void *arg)
{
    err_t err, recv_err;
    upcb = udp_new();

    /* Create new UDP netconn */
    conn = netconn_new(NETCONN_UDP);

    if (conn == NULL)
    {
        netconn_delete(conn);
        vTaskDelete(NULL);
        return;
    }

    /* Bind to local port */
    err = netconn_bind(conn, IP_ADDR_ANY, CORNER_PORT);
    if (err != ERR_OK)
    {
        netconn_delete(conn);
        vTaskDelete(NULL);
        return;
    }

//    netconn_set_recvtimeout(conn, 5);

    /* Destination IP address */
    /* specialy for udpsend */
    IP_ADDR4(&destIP, 10, 252, 62, 212);

    /* Connect to destination port */
//    err = netconn_connect(conn, &destIP, destPort);
//    if (err != ERR_OK)
//    {
//        netconn_delete(conn);
//        vTaskDelete(NULL);
//        return;
//    }

    /* Infinite receive loop */
    for (;;)
    {
        recv_err = netconn_recv(conn, &rxbuf);

        if (recv_err == ERR_OK && rxbuf != NULL)
        {
			memcpy(&v2c, rxbuf->p->payload,sizeof(VCUToCorner));
			if(v2c.Header == VCU_FRAME_HEADER && v2c.id == CORNER_FRAME_ID)
			{
				//Extract data
				data_trq         = v2c.Trq;
				data_Vx          = v2c.Vx;
				data_Vx_Wheel    = v2c.Vx_Wheel;
				data_Vx_Thrl     = v2c.Vx_Thrl;
				data_Steer_Wheel = v2c.Steer_Wheel;
				data_Mzd         = v2c.Mzd;
				sequence	     = v2c.seq;

				data_trq         = (data_trq / 100) - 120;
				data_Vx          = data_Vx / 100;
				data_Vx_Wheel    = data_Vx_Wheel / 100;
				data_Vx_Thrl     = data_Vx_Thrl / 100;
				data_Steer_Wheel = (data_Steer_Wheel / 100) - 40;
				data_Mzd	     =  data_Mzd - 40000;
			}

			//Release buffer
			netbuf_delete(rxbuf);
        }
    }
}

static void led_command_thread(void *arg)
{
    struct netbuf *command_buf = NULL;
    char command[32];
    void *payload;
    u16_t payload_len;
    err_t err;

    (void)arg;
    led_conn = netconn_new(NETCONN_UDP);
    if (led_conn == NULL)
    {
        vTaskDelete(NULL);
        return;
    }

    err = netconn_bind(led_conn, IP_ADDR_ANY, LED_COMMAND_PORT);
    if (err != ERR_OK)
    {
        netconn_delete(led_conn);
        vTaskDelete(NULL);
        return;
    }

    for (;;)
    {
        if (netconn_recv(led_conn, &command_buf) != ERR_OK || command_buf == NULL)
            continue;

        char responseBuffer[48];
        const char *response = "ERR UNKNOWN COMMAND";
        
        if (netbuf_data(command_buf, &payload, &payload_len) == ERR_OK &&
            payload_len < sizeof(command))
        {
            memcpy(command, payload, payload_len);
            command[payload_len] = '\0';

            // LED PB0 commands
            if (strcmp(command, "PB0 ON") == 0)
            {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
                response = "OK";
            }
            else if (strcmp(command, "PB0 OFF") == 0)
            {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
                response = "OK";
            }
            else if (strcmp(command, "PB0 TOGGLE") == 0)
            {
                HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
                response = "OK";
            }
            else if (strcmp(command, "PB0 STATUS") == 0)
            {
                /* Report the output state commanded by the STM32. */
                response = (GPIOB->ODR & GPIO_PIN_0) ? "PB0 ON" : "PB0 OFF";
            }
            // LED PB7 commands
            else if (strcmp(command, "LED ON") == 0)
            {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
                response = "OK";
            }
            else if (strcmp(command, "LED OFF") == 0)
            {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
                response = "OK";
            }
            else if (strcmp(command, "LED TOGGLE") == 0)
            {
                HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_7);
                response = "OK";
            }
            // Motor commands
            else if (strcmp(command, "MOTOR STATUS") == 0)
            {
                snprintf(responseBuffer, sizeof(responseBuffer),
                         "MOTOR STATUS %ld %ld",
                         (long)motorQtReff,
                         (long)motorQtGetActualRpm());
                response = responseBuffer;
            }
            else if (strcmp(command, "MOTOR AUTO") == 0)
            {
                motorQtControl = 0;
                response = "OK";
            }
            else if (strncmp(command, "MOTOR ", 6) == 0)
            {
                if (strcmp(command, "MOTOR STOP") == 0)
                {
                    motorQtReff = 0.0f;
                    motorQtControl = 1;
                    response = "OK";
                }
                else
                {
                    char *endPtr = NULL;
                    float requestedRpm = strtof(command + 6, &endPtr);
                    if (endPtr != command + 6 && *endPtr == '\0' &&
                        requestedRpm >= -120.0f && requestedRpm <= 120.0f)
                    {
                        motorQtReff = requestedRpm;
                        motorQtControl = 1;
                        response = "OK";
                    }
                }
            }
            // Stepper/Steering commands
            else if (strcmp(command, "STEPPER STATUS") == 0)
            {
                snprintf(responseBuffer, sizeof(responseBuffer),
                         "STEPPER STATUS %ld %ld",
                         (long)stepperQtAngle,
                         (long)stepperQtGetActualAngle());
                response = responseBuffer;
            }
            else if (strcmp(command, "STEPPER AUTO") == 0)
            {
                stepperQtControl = 0;
                response = "OK";
            }
            else if (strncmp(command, "STEPPER ", 8) == 0)
            {
                if (strcmp(command, "STEPPER STOP") == 0)
                {
                    stepperQtAngle = 0.0f;
                    stepperQtControl = 1;
                    response = "OK";
                }
                else
                {
                    char *stepperEndPtr = NULL;
                    float requestedAngle = strtof(command + 8, &stepperEndPtr);
                    if (stepperEndPtr != command + 8 && *stepperEndPtr == '\0' &&
                        requestedAngle >= -40.0f && requestedAngle <= 40.0f)
                    {
                        stepperQtAngle = requestedAngle;
                        stepperQtControl = 1;
                        response = "OK";
                    }
                }
            }
        }

        struct netbuf *response_buf = netbuf_new();
        if (response_buf != NULL)
        {
            netbuf_ref(response_buf, response, strlen(response));
            netconn_sendto(led_conn, response_buf, netbuf_fromaddr(command_buf),
                           netbuf_fromport(command_buf));
            netbuf_delete(response_buf);
        }

        netbuf_delete(command_buf);
        command_buf = NULL;
    }
}

/*-------UDP Send function to send the data to the server-------------*/
void udpsend (char *data)
{
	buf = netbuf_new();   // Create a new netbuf
	netbuf_ref(buf, data, strlen(data));  // refer the netbuf to the data to be sent 
	netconn_send(conn,buf);  // send the netbuf to the client
	netbuf_delete(buf);  // delete the netbuf
}

void udpsend_hex(const void *data, uint16_t len)
{
	//udp_pcb
//    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
//    if (p != NULL) {
//        memcpy(p->payload, data, len);
//        udp_sendto(upcb, p, &destIP, destPort);
//        pbuf_free(p);
//    }

    //netconn
    struct netbuf *nbuf = netbuf_new();
    if (nbuf == NULL) return;

    void *payload = netbuf_alloc(nbuf, len);
    memcpy(payload, data, len);

//    netconn_send(conn, nbuf);
    netconn_sendto(conn, nbuf, &destIP, MATLAB_PORT);
    netbuf_delete(nbuf);
}

/* UDP send Thread will send data every 500ms */
//static void udpsend_thread(void *arg)
//{
//	for (;;)
//	{
//		sprintf (smsg, "index value = %d\n", indx++);
//		udpsend(smsg);
//		osDelay(500);
//	}
//}


void udpclient_init(void)
{
//	sys_thread_new("udpsend_thread", udpsend_thread, NULL, DEFAULT_THREAD_STACKSIZE,osPriorityNormal);
	sys_thread_new("udpinit_thread", udpinit_thread, NULL, 256,osPriorityNormal);
}

void ledCommand_init(void)
{
    sys_thread_new("led_command", led_command_thread, NULL, 128, osPriorityNormal);
}


