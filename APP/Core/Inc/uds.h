#ifndef _UDS_H_
#define _UDS_H_

#include "typedef_conf.h"

typedef uint8_t (*dcm_function)(uint8_t *data, uint16_t dataLen);

// service
typedef struct
{
    uint8_t service_id;          // 服务的id
    dcm_function serviceTrigger; // 服务的触发器
    uint8_t *dcmSessionLevel;    // session支持
    uint8_t *dcmSecurityLevel;   // service支持
    dcm_function *subServices;   // 子功能支持
} Dcm_services_conf;

// read did
typedef struct
{
    uint8_t session_level; // 会话等级

    uint8_t security_level; // 安全等级

    dcm_function readDidFunc; // 读取DID的函数接口

} Dcm_readDids_conf;

// write did
typedef struct
{
    uint8_t session_level;     // 会话等级
    uint8_t security_level;    // 安全等级
    dcm_function writeDidFunc; // 写DID的函数接口
} Dcm_writeDids_conf;

// control dids 31服务
typedef struct
{
    uint8_t *session_level;      // 会话等级
    uint8_t *security_level;     // 安全等级
    dcm_function controlDidFunc; // control DID的函数接口
} Dcm_controlDid_conf;

// did 同一个did支持不同的功能。
typedef struct
{
    uint16_t identifier; // DID for 22 ,2e
    Dcm_writeDids_conf didWrite;
    Dcm_readDids_conf didRead;
} Dcm_dids_conf;

// did 同一个did支持不同的功能。
typedef struct
{
    uint32_t longIdentifier; // DID 31
    Dcm_controlDid_conf didControlConf;
} Dcm_control_dids_conf;

typedef struct
{
    uint8_t init;
    uint8_t reqData[255];
    uint16_t reqDataLength;
    uint16_t receiveDataLength;
    uint8_t resData[255];
    uint16_t resDataLength;
} uds_frame_tag;

typedef struct
{
    uint8_t data[255];        // 数据buff
    uint32_t CRC;             // 数据的crc校验
    uint32_t dataLength;      // 传入的数据长度
    uint32_t reqDataLen;      // 请求的数据长度
    uint32_t reqFlashAddress; // 请求写入的数据地址
    uint8_t reciveDataTimes;  // 接受到的服务次数
} OTA_data;

typedef enum
{
    PositiveResponse = 0x40,
    GeneralReject = 0x10,
    ControlPositiveResponse = 0x41,
    ServiceNotSupported = 0x11,
    ServiceNotSupportedInActiveSession = 0x7f,
    ResponsePending = 0x78,
    SubFunctionNotSupported = 0x12,
    IncorrectMessageLengthOrInvalidFormat = 0x13,
    RequestSequenceError = 0x24, // 请求顺序错误
    FirstFrameResponse = 0x25,   // 第一帧回复
    CustomResponse = 0x26        // 自定义回复
} uds_response_code;

uint8_t can_service_entry(uint8_t *data);
void can_service_response(uint8_t *data,uint16_t dataLength);
void uds_response(uint8_t nrc);
void uds_entry(uint8_t *data, uint16_t dataLen);
#endif