/*
 * udpclient.h
 *
 *  Created on: Dec 21, 2025
 *      Author: Rama Syafrizal
 */

#ifndef INC_UDPCLIENT_H_
#define INC_UDPCLIENT_H_

#define CORNER_PORT_FL	5051
#define CORNER_PORT_FR	5052
#define CORNER_PORT_RL	5053
#define CORNER_PORT_RR	5054

#define CORNER_FRAME_ID_FL	0x01
#define CORNER_FRAME_ID_FR	0x02
#define CORNER_FRAME_ID_RL	0x03
#define CORNER_FRAME_ID_RR	0x04

#define CORNER_FRAME_HEADER	0xBA
#define VCU_FRAME_HEADER	0xCB

#define MATLAB_PORT	4095
#define LED_COMMAND_PORT 5055

#define CORNER_PORT			CORNER_PORT_RL
#define CORNER_FRAME_ID		CORNER_FRAME_ID_RL

#if (CORNER_PORT == CORNER_PORT_FL) || (CORNER_PORT == CORNER_PORT_FR)
	#define FRONT_WHEEL
#endif

/*Packet Receive*/
typedef struct __attribute__((packed)) {
	uint16_t Speed;
	uint16_t Angel;
}CobaData;

typedef struct __attribute__((packed)) {
	uint8_t Header;
	uint8_t id;
	uint16_t ADCvalue;
	uint16_t seq;
}Micro2Micro;

typedef struct __attribute__((packed)) {
	uint8_t Header;
	uint8_t id;
	uint16_t Trq;
	uint16_t Vx;
	uint16_t Vx_Wheel;
	uint16_t Vx_Thrl;
	uint16_t Steer_Wheel;
	uint32_t Mzd;
	uint16_t seq;
}VCUToCorner;

typedef struct __attribute__((packed)) {
	uint8_t Header;
	uint8_t id;
	uint16_t Vx_Wheel;
	uint16_t Steer_Wheel;
	uint16_t seq;
	uint32_t timestamp;
}CornerToSimulink_Latency;

typedef struct __attribute__((packed)) {
	uint8_t Header;
	uint8_t id;
	uint16_t seq;
	uint32_t timestamp;
}Latency_sendBack;

/*Packet Transmit*/
typedef struct __attribute__((packed)) {
	uint8_t Header;
	uint8_t id;
	uint16_t speedReff;
	uint16_t speedAct;
	uint16_t angleReff;
	uint16_t angleAct;
	uint16_t seq;
}dataLogger;

typedef struct __attribute__((packed)) {
	uint8_t Header;
	uint8_t id;
	uint16_t ADCdata;
	uint16_t seq;
}MicroP2P;

typedef struct __attribute__((packed)) {
	uint8_t Header;
	uint8_t id;
	uint16_t Vx_Wheel;
	uint16_t Steer_Wheel;
	uint16_t seq;
}CornerToSimulink;

typedef struct __attribute__((packed)) {
	uint8_t Header;
	uint8_t id;
	uint16_t Trq;
	uint16_t Vx;
	uint16_t Vx_Wheel;
	uint16_t Vx_Thrl;
	uint16_t Steer_Wheel;
	uint32_t Mzd;
	uint16_t seq;
	uint32_t timestamp;
}VCUToCorner_Latency;

typedef struct __attribute__((packed)) {
	uint8_t Header;
	uint8_t id;
	uint16_t Vx;
	uint16_t Vy;
	uint16_t deltaFL;
	uint16_t deltaFR;
	uint16_t deltaRL;
	uint16_t deltaRR;
	uint16_t beta;
	uint16_t yawrate;
	uint16_t Fy_FL;
	uint16_t Fy_FR;
	uint16_t Fy_RL;
	uint16_t Fy_RR;
	uint16_t Fz_FL;
	uint16_t Fz_FR;
	uint16_t Fz_RL;
	uint16_t Fz_RR;
	uint16_t omegaFL;
	uint16_t omegaFR;
	uint16_t omegaRL;
	uint16_t omegaRR;
	uint16_t seq;
	uint32_t timestamp;
}PCToVCU_Latency;

typedef struct __attribute__((packed)) {
	uint8_t Header;
	uint8_t id;
	uint16_t seq;
	uint32_t timestamp;
}Latency_send;

// Mode pengiriman Ethernet
typedef enum {
    SEND_TO_MATLAB = 0,
    SEND_TO_VCU    = 1
} EthMode_t;

void udpclient_init(void);
void ledCommand_init(void);
void HIL_ETH_ToggleMode(void);
void udpsend_hex(const void *data, uint16_t len);
void udpReceive(void);
void HIL_ETH_Init(void);

extern EthMode_t currEthMode;
extern uint16_t data1;
extern uint16_t data2;

extern float data_trq;
extern float data_Vx;
extern float data_Vx_Wheel;
extern float data_Vx_Thrl;
extern float data_Steer_Wheel;
extern int32_t data_Mzd;
extern uint16_t adcData;
extern uint32_t timestamp;

extern uint16_t sequence;
extern int Latency;
extern double Latency_s;

#endif /* INC_UDPCLIENTRAW_H_ */
