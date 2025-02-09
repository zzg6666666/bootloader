#include "uds.h"
#include "typedef_conf.h"
#include "uart_printf.h"
#include "string.h"  //memcpy  memset
#include "command.h" //flash_erase
#include "crc.h"
// static const Dcm_dids_conf dcmCfgDids[] = {
//     {
//         0xF190,   // id
//         dcm_NULL, // write
//         dcm_NULL, // read
//         dcm_NULL  // control
//     },
//     {
//         0xF180,   // id
//         dcm_NULL, // write
//         dcm_NULL, // read
//         dcm_NULL  // control
//     },
// };

// 0x31 01FF00开始擦写 0x3103FF00u擦写结果， 010202 crc  010203 检查前提条件  01ff05 开始安装

// ota 流程 10 02 重启到bootloader  27解锁  擦除flash  38(34)请求升级  78正反馈，36请求传输 76正反馈 0x37传输完成 31 crc  31 安装

// 31服务Trigger

// 开始擦写
static uint8_t service0x31SubFunctionStartErase(uint8_t *data, uint16_t dataLen);
// 擦写结果
static uint8_t service0x31SubFunctionEraseResult(uint8_t *data, uint16_t dataLen);
// 进行CRC校验
static uint8_t service0x31SubFunctionCRCValidate(uint8_t *data, uint16_t dataLen);
// 开始刷写flash
static uint8_t service0x31SubFunctionUpdateProgram(uint8_t *data, uint16_t dataLen);
// 升级完成CRC验证
static uint8_t service0x31SubFunctionDownFinishCRCValidate(uint8_t *data, uint16_t dataLen);

static uint8_t service0x31Trigger(uint8_t *data, uint16_t dataLen);
static uint8_t service0x34Trigger(uint8_t *data, uint16_t dataLen);
static uint8_t service0x36Trigger(uint8_t *data, uint16_t dataLen);
static uint8_t service0x37Trigger(uint8_t *data, uint16_t dataLen);

// 支持的服务表长度 bootloader 必须的session 必须是1002 需要解锁
Dcm_services_conf dcmServicesTable[] =
    {
        {
            0x10U,    // id
            dcm_NULL, // service trigger
            dcm_NULL, // session
            dcm_NULL, // security //bootloader
            dcm_NULL  // subService
        },
        {
            0x11U,    // id
            dcm_NULL, // service trigger
            dcm_NULL, // session
            dcm_NULL, // security
            dcm_NULL  // subService
        },
        {
            0x22U,    // id
            dcm_NULL, // service trigger
            dcm_NULL, // session
            dcm_NULL, // security
            dcm_NULL  // subService
        },
        {
            0x2EU,    // id
            dcm_NULL, // service trigger
            dcm_NULL, // session
            dcm_NULL, // security
            dcm_NULL  // subService
        },
        {
            0x27U,    // id
            dcm_NULL, // service trigger
            dcm_NULL, // session
            dcm_NULL, // security
            dcm_NULL  // subService
        },
        {
            0x28U,    // id
            dcm_NULL, // service trigger
            dcm_NULL, // session
            dcm_NULL, // security
            dcm_NULL  // subService
        },
        {
            0x31U,              // id
            service0x31Trigger, // service trigger
            dcm_NULL,           // session
            dcm_NULL,           // security
            dcm_NULL            // subService
        },
        {
            0x34U,              // id
            service0x34Trigger, // service trigger
            dcm_NULL,           // session
            dcm_NULL,           // security
            dcm_NULL            // subService
        },
        {
            0x36U,              // id
            service0x36Trigger, // service trigger
            dcm_NULL,           // session
            dcm_NULL,           // security
            dcm_NULL            // subService
        },
        {
            0x37U,              // id
            service0x37Trigger, // service trigger
            dcm_NULL,           // session
            dcm_NULL,           // security
            dcm_NULL            // subService
        },
        {
            0x38U,    // id
            dcm_NULL, // service trigger
            dcm_NULL, // session
            dcm_NULL, // security
            dcm_NULL  // subService
        },
        {
            0x3EU,    // id
            dcm_NULL, // service trigger
            dcm_NULL, // session
            dcm_NULL, // security
            dcm_NULL  // subService
        },
        {
            0x85U,    // id
            dcm_NULL, // service trigger
            dcm_NULL, // session
            dcm_NULL, // security
            dcm_NULL  // subService
        },
};
static const Dcm_control_dids_conf dcmControlDids[] = // service 0x31
    {
        {0x010202, {dcm_NULL, dcm_NULL, service0x31SubFunctionCRCValidate}},           // crc验证
        {0x01FF00, {dcm_NULL, dcm_NULL, service0x31SubFunctionStartErase}},            // erase flash
        {0x03FF00, {dcm_NULL, dcm_NULL, service0x31SubFunctionEraseResult}},           // check flash result
        {0x010203, {dcm_NULL, dcm_NULL, service0x31SubFunctionUpdateProgram}},         // 开始升级
        {0x010205, {dcm_NULL, dcm_NULL, service0x31SubFunctionDownFinishCRCValidate}}, // 下载完成crc验证
        {0x01ff05, {dcm_NULL}}};

// 储存 uds接收到的信息
uds_frame_tag uds_frame = {.init = 0,
                           .receiveDataLength = 0,
                           .reqDataLength = 0,
                           .resDataLength = 0};

// memset(uds_frame.reqData, 0x00, 255);

// 储存ota收到的buff
OTA_data ota_data = {.dataLength = 0,
                     .reqDataLen = 0,
                     .dataLength = 0,
                     .CRC = 0,
                     .reciveDataTimes = 0,
                     .reqFlashAddress = 0};

// can 每条报文长度为8字节
uint8_t can_service_entry(uint8_t *data)
{

    // 检查服务是单帧(0)  连续帧(2) ，首帧(1)

    switch (((data[0]) & 0xf0) >> 4)
    {
    case 0: // 单帧
    {
        memset(uds_frame.reqData, 0x00, 255);
        uds_frame.reqDataLength = data[0] & 0x0f;
        memcpy(uds_frame.reqData, &data[1], uds_frame.reqDataLength);
        uds_entry(uds_frame.reqData, uds_frame.reqDataLength);
    }

    break;

    case 1: // 首帧
    {
        uds_frame.init = 1;
        memset(uds_frame.reqData, 0x00, 255);
        uds_frame.reqDataLength = ((data[0] & 0x0f) << 8) + data[1];
        uds_frame.receiveDataLength = 6;
        memcpy(&uds_frame.reqData[0], &data[2], 6);
        {
            // error catch 空间不足，跳帧 等等
        }
        uds_response(FirstFrameResponse);
    }
    break;
    case 2: // 连续帧
    {
        if ((uds_frame.receiveDataLength + 7) >= uds_frame.reqDataLength)
        {
            // 接收完成
            memcpy(&uds_frame.reqData[uds_frame.receiveDataLength], &data[1], (uds_frame.reqDataLength - uds_frame.receiveDataLength));
            uds_entry(uds_frame.reqData, uds_frame.reqDataLength);
        }
        else
        {
            // 持续接收
            memcpy(&uds_frame.reqData[uds_frame.receiveDataLength], &data[1], 7);
            uds_frame.receiveDataLength += 7;
        }
    }
    }

    // can respoense

    return 0;
}

// uds 入口，支持can和其他协议
void uds_entry(uint8_t *data, uint16_t dataLen)
{
    for (int i = 0; i < 720000; i++)
    { // 延时，
    }
    // uint8_t ret_value ;
    Dcm_services_conf *service = dcm_NULL;

    // 检查服务是否被支持
    for (uint8_t i = 0; i < sizeof(dcmServicesTable) / sizeof(Dcm_services_conf); i++)
    {
        if (data[0] == dcmServicesTable[i].service_id)
        {
            uart_printf(system_log, "find services :", &dcmServicesTable[i].service_id, 1);
            uart_printf(system_log, "recive data is ", data, (uint8_t)dataLen);
            service = &dcmServicesTable[i];
            break;
        }
    }

    // 未找到sid
    if (service == dcm_NULL)
    {
        uart_printf(system_log, "the sid dont been supported\r\n", NULL, 0);

        uds_response(ServiceNotSupported);
    }
    // 未找到 subid
    else if (service->serviceTrigger == NULL)
    {
        uart_printf(system_log, "the sid dont have a trriger function\r\n", NULL, 0);
        uds_response(SubFunctionNotSupported);
    }
    else if (service->serviceTrigger != dcm_NULL)
    {
        uds_response(ResponsePending);
        service->serviceTrigger(&data[1], dataLen - 1);
    }
}

// 开始擦写
static uint8_t service0x31SubFunctionStartErase(uint8_t *data, uint16_t dataLen)
{
    uint8_t ret_value = 0;
    uart_printf(system_log, "Start Erase at ", data, 4);
    uint32_t EraseAddress = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];

    // 解锁 flash
    flash_lock(1);
    ret_value = flash_erase(EraseAddress);

    if (ret_value == true)
    {
        uds_response(ControlPositiveResponse);
    }
    else
    {
        uds_response(GeneralReject);
    }
    return ret_value;
}

// 擦写结果 传入开始地址和结束地址
static uint8_t service0x31SubFunctionEraseResult(uint8_t *data, uint16_t dataLen)
{
    uint8_t ret_value;
    uint8_t nrc = ControlPositiveResponse;
    uint32_t readAddressStart;
    uint32_t readAddressEnd;
    uart_printf(system_log, "check Erase Result\r\n", NULL, 0);

    convert_uint8_t_to_uint32_t(data, &readAddressStart, 1);
    convert_uint8_t_to_uint32_t(&data[4], &readAddressEnd, 1);
    // uint32_t readAddressStart = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
    // uint32_t readAddressEnd = (data[4] << 24) + (data[5] << 16) + (data[6] << 8) + data[7];

    // 检查flash地址合法性 擦写时，flash_erase会做判断
    ret_value = check_flash_address(readAddressStart);
    ret_value = check_flash_address(readAddressEnd);

    // flash的真实地址
    readAddressStart &= 0x0000FFFF;

    // 页地址
    readAddressStart = readAddressStart - (readAddressStart % 1024);
    readAddressStart |= 0x08000000;

    readAddressEnd |= 0x08000000;

    // 读取flash
    uint32_t *temp = (uint32_t *)readAddressStart;

    if (ret_value == true)
    {
        // 检查区域是不是都是0xFF
        uint16_t i;
        for (i = 0; i < (readAddressEnd - readAddressStart) / 4; i++)
        {
            if (*temp != 0xFFFFFFFF)
            {
                break;
            }
            temp++;
        }

        if (i != (readAddressEnd - readAddressStart) / 4)
        {
            ret_value = false;
            uint8_t data_to_uint8_t[4];
            readAddressStart += i * 4;
            convert_uint32_t_to_uint8_t(&readAddressStart, data_to_uint8_t, 1);
            uart_printf(error_log | system_log, "flash not 0xff :", data_to_uint8_t, 4);
            nrc = GeneralReject;
        }
    }
    uds_response(nrc);
    return ret_value;
}
// 进行CRC校验 data 0 -3 传入的CRC值， data 4 = 0x1，验证ota一包的数据 0x02 ，验证升级整个app的数据
static uint8_t service0x31SubFunctionCRCValidate(uint8_t *data, uint16_t dataLen)
{

    uart_printf(system_log, "CRC data\r\n", data, dataLen);
    uint32_t inputCRC = 0x00; // 子服务传进来的CRC值
    uint8_t nrc = ControlPositiveResponse;
    uint32_t crc_data[(ota_data.dataLength / 4)]; // 用于计算的cRC数据

    convert_uint8_t_to_uint32_t(data, &inputCRC, 1);
    convert_uint8_t_to_uint32_t(&ota_data.data[0], crc_data, ota_data.dataLength / 4);

    // 进行crc

    crc_init();
    crc_reset();

    uint32_t crc_result;
    crc_result = crc_calculate(crc_data, sizeof(crc_data) / sizeof(uint32_t));

    uint8_t temp[4];
    convert_uint32_t_to_uint8_t(&crc_result, temp, 1);
    if (crc_result != inputCRC)
    {
        uart_printf(error_log | system_log, "crc fail", temp, 4);
        nrc = GeneralReject;
    }
    uds_response(nrc);
    return 0;
}

static uint8_t service0x31SubFunctionDownFinishCRCValidate(uint8_t *data, uint16_t dataLen)
{
    uint8_t nrc = ControlPositiveResponse;
    uint32_t inputCRC = 0x00; // 子服务传进来的CRC值
    uint32_t APP_start_address = 0x00;
    uint32_t APP_end_address = 0x00;
    convert_uint8_t_to_uint32_t(data, &inputCRC, 1);
    convert_uint8_t_to_uint32_t(data + 4, &APP_start_address, 1);
    convert_uint8_t_to_uint32_t(data + 8, &APP_end_address, 1);

    if (check_flash_address(APP_start_address) != true || check_flash_address(APP_end_address) != true)
    {
        nrc = IncorrectMessageLengthOrInvalidFormat;
    }
    else
    {
        uint32_t *flash_data = (uint32_t *)APP_start_address;
        uint16_t flash_data_length = (APP_end_address - APP_start_address) / 4;
        crc_reset();
        if (inputCRC != crc_calculate(flash_data, flash_data_length))
        {
            nrc = GeneralReject;
        }
    }
    uds_response(nrc);
    return 0;
}

static uint8_t service0x31SubFunctionUpdateProgram(uint8_t *data, uint16_t dataLen)
{
    uart_printf(system_log, "31 service insatll app\r\n", NULL, 0);

    uint16_t flash_data_length = ota_data.dataLength / 4;
    uint32_t flash_data[flash_data_length];

    memset(flash_data, 0, flash_data_length);

    convert_uint8_t_to_uint32_t(ota_data.data, flash_data, flash_data_length);

    flash_write(ota_data.reqFlashAddress, flash_data, flash_data_length);
    uds_response(ControlPositiveResponse);
    return 0;
}

static uint8_t service0x31Trigger(uint8_t *data, uint16_t dataLen)
{
    uint8_t ret_value = false;
    // 检查是否支持此功能
    uart_printf(system_log, "31 service data ", data, dataLen);

    // 解析sid
    uint32_t identifier = (data[0] << 16) + (data[1] << 8) + data[2];
    Dcm_controlDid_conf Dcm_controlDid_conf = {dcm_NULL};
    for (uint8_t i = 0; i < sizeof(dcmControlDids) / sizeof(Dcm_control_dids_conf); i++)
    {
        if (dcmControlDids[i].longIdentifier == identifier)
        {
            uart_printf(system_log, "31 service find subId\r\n", NULL, 0);
            Dcm_controlDid_conf = dcmControlDids[i].didControlConf;
            break;
        }
    }
    // 找到sid 对应的 function
    if (Dcm_controlDid_conf.controlDidFunc == dcm_NULL)
    {
        uart_printf(system_log, "service 31 dont support this did or this did dont have a sub function \r\n", NULL, 0);
    }
    else
    {
        // 执行对应的control did
        ret_value = Dcm_controlDid_conf.controlDidFunc(&data[3], dataLen - 3);
    }

    return ret_value;
}

// 34 服务
static uint8_t service0x34Trigger(uint8_t *data, uint16_t dataLen)
{
    uint8_t ret_value = true;
    uart_printf(info_log, "34 service trigger data is ", data, dataLen);

    // 34服务没有对应的sid 主要是参数配置
    /*
    data[0] : 参数配置 比如是否压缩
    data[1] : 该参数是代表后续的两个部分memoryAddress和memorySize所占的字节长度
    data[2 - data[(data[1] & 0xf0)] memoryAddress
    剩下 memorySize
    */

    /*
    返回：74
    高4bit为maxNumberOfBlockLength有效字节长度，低4bit保留为0.
    maxNumberOfBlockLength :表示0x36服务一次传输一个block的最大的字节数。
     */
    //
    // 忽略 data[0]

    // 写入的数据size 高4位
    uint8_t memorySizeLength = (data[1] & 0xf0) >> 4;
    // 写入的数据地址size 低4位
    uint8_t memoryAddressLength = (data[1] & 0x0f);

    // 数据长度
    uint8_t memorySize[4] = {0x00};
    // 写入数据地址
    uint8_t memoryAddress[4] = {0x00};

    // 取出数据长度
    for (uint8_t i = 0; i < memorySizeLength; i++)
    {
        memorySize[(4 - memorySizeLength) + i] |= data[2 + i];
    }
    // 取出数据地址
    for (uint8_t i = 0; i < memoryAddressLength; i++)
    {
        memoryAddress[(4 - memoryAddressLength) + i] |= data[2 + memorySizeLength + i];
    }

    uart_printf(info_log, "memorySize :", memorySize, 4);
    uart_printf(info_log, "memoryAddress :", memoryAddress, 4);

    convert_uint8_t_to_uint32_t(memorySize, &ota_data.reqDataLen, 1);
    convert_uint8_t_to_uint32_t(memoryAddress, &ota_data.reqFlashAddress, 1);

    // 清零写入的数据长度
    ota_data.dataLength = 0x00;

    // 清零写入的次数
    ota_data.reciveDataTimes = 0x00;
    {
        uds_frame.resData[0] = 0x74;
        uds_frame.resData[1] = 0x10;
        uds_frame.resData[2] = 0x1C; // 36 服务一帧的最大字节
        uds_frame.resDataLength = 0x03;
        uds_response(CustomResponse);
    }
    return ret_value;
}

// 36 服务 格式 36 count xxxx ，count 从 01 到 ff，然后从00开始
static uint8_t service0x36Trigger(uint8_t *data, uint16_t dataLen)
{
    uart_printf(info_log, "36 data ", data, dataLen);

    // 检查传包的顺序
    if ((ota_data.reciveDataTimes + 1) != data[0])
    {
        uds_response(GeneralReject);
        return false;
    }

    // 检查数据是不是超了
    if ((dataLen + ota_data.dataLength - 1) > sizeof(ota_data.data))
    {
        uds_response(GeneralReject);
        return false;
    }

    // 复制到暂存数据 跳过 counter
    memcpy(&ota_data.data[ota_data.dataLength], &data[1], dataLen - 1);

    // 更新请求数据长度
    ota_data.dataLength += dataLen - 1;
    // 更新请求的数据次数
    ota_data.reciveDataTimes++;
    uds_frame.resDataLength = 0x02;
    uds_frame.resData[0] = uds_frame.reqData[0] + 0x40;
    uds_frame.resData[1] = uds_frame.reqData[1];
    uds_response(CustomResponse);
    return true;
}

// 37服务
static uint8_t service0x37Trigger(uint8_t *data, uint16_t dataLen)
{
    // 传入的数据没有规定
    uart_printf(info_log, "37 data ", data, dataLen);

    // 数据传输完成 检查数据是否完成 并返回正反馈的nrc

    // 检查传入的数据长度和请求的数据长度 检查传入的数据长度能不能整除4
    if ((ota_data.dataLength != ota_data.reqDataLen) || (ota_data.dataLength % 4 != 0))
    {
        uds_response(IncorrectMessageLengthOrInvalidFormat);
        return false;
    }

    uds_frame.resDataLength = 0x02;
    uds_frame.resData[0] = 0x77;
    uds_frame.resData[1] = 0x00;
    uds_response(CustomResponse);
    return true;
}

void uds_response(uint8_t nrc)
{
    switch (nrc)
    {
    case PositiveResponse:
    {
        uds_frame.resData[0] = uds_frame.reqData[0] + 0x40;
        uds_frame.resData[1] = uds_frame.reqData[1];
        uds_frame.resData[2] = uds_frame.reqData[2];
        uds_frame.resDataLength = 0x03;
    }
    break;
    case ControlPositiveResponse:
    {
        uds_frame.resData[0] = uds_frame.reqData[0] + 0x40;
        uds_frame.resData[1] = uds_frame.reqData[1];
        uds_frame.resData[2] = uds_frame.reqData[2];
        uds_frame.resData[3] = uds_frame.reqData[3];
        uds_frame.resDataLength = 0x04;
    }

    break;

    case FirstFrameResponse:
    {
        uds_frame.resData[0] = 0x30;
        uds_frame.resDataLength = 0x01;
        break;
    }
    case ServiceNotSupported:
    case ServiceNotSupportedInActiveSession:
    case ResponsePending:
    case SubFunctionNotSupported:
    case IncorrectMessageLengthOrInvalidFormat:
    case RequestSequenceError:
    case GeneralReject:
    {
        uds_frame.resData[0] = 0x7F;
        uds_frame.resData[1] = uds_frame.reqData[0];
        uds_frame.resData[2] = nrc;
        uds_frame.resDataLength = 0x03;
    }
    break;
    case CustomResponse:
    default:
        break;
    }
    uart_printf(info_log, "response :", uds_frame.resData, uds_frame.resDataLength);
    can_service_response(uds_frame.resData, uds_frame.resDataLength);
    // 其他方式
}

void can_service_response(uint8_t *data, uint16_t dataLength)
{
    // 一帧can消息为8字节 单帧和连续帧

    uart_printf(info_log, "can_service_response ", data, dataLength);

    uint8_t can_data[8] = {0x00};
    if (dataLength <= 7) // 单帧
    {
        can_data[0] = dataLength & 0xff;
        memcpy(&can_data[1], data, dataLength);
        uart2_communicate_tx(can_data, 8);
    }
    else
    {
        // 连续帧的首帧
        can_data[0] = 0x10;
        if (dataLength > 255)
        {
            can_data[0] |= (dataLength & 0xf00) >> 8;
        }

        can_data[1] = (dataLength & 0xff); // 消息长度

        memcpy(&can_data[2], data, 6); // 首帧写入6个字节
        // 写入到fifo中
        uart2_communicate_tx(can_data, 8);

        uart_printf(info_log, "can frame", can_data, 8);

        // 连续帧的数量
        uint8_t frame_count = ((dataLength - 6) / 7);
        // 连续帧的低位
        uint8_t frame_low_4bits = 0x01;

        // 连续帧中的一帧数据为7字节，从data[6]开始
        for (uint8_t i = 0; i < frame_count; i++)
        {
            // 连续帧的第几帧
            can_data[0] = (0x20 | frame_low_4bits);
            // 复制这一帧的数据
            memcpy(&can_data[1], &data[6 + 7 * i], 7);
            uart2_communicate_tx(can_data, 8);
            uart_printf(info_log, "can frame", can_data, 8);
            frame_count++;
            if (frame_count > 0x0f)
            {
                frame_count = 0x00;
            }
        }
        // 是否还有不满7字节的一帧需要发送
        if ((frame_count * 7 + 6) != dataLength)
        {
            can_data[0] = (0x20 | frame_low_4bits);
            // 设置can_data数据为0
            memset(&can_data[1], 0, 7);
            // 复制剩下的数据
            memcpy(&can_data[1], &data[frame_count * 7 + 6], dataLength - frame_count * 7 - 6);
            uart_printf(info_log, "can frame", can_data, 8);
            uart2_communicate_tx(can_data, 8);
        }
    }
}
/*
测试步骤：

34 写入数据信息 34 00 44 00 00 00 0b 08 00 fc 00 {34 00 44 00 00 00 0b 08 00 fc 00}
46 请求下载
37 结束下载
31 01 02 02 //进行crc 计算

//34

uds 10 0d 34 00 44 00 00 00
uds 21 0b 08 00 fc 00 00 00

//36

uds 10 0d 36 01 12 34 56 78
uds 21 9a bc de f0 12 34 56

//37
uds 02 37 00 00 00 00 00 00

//31 crc 计算
uds 10 08 31 01 02 02 c0 47
uds 21 91 30 00 00 00 00 00

//请求下载
*/