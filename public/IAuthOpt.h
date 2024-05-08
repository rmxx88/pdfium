#ifndef IAUTHOPT_H
#define IAUTHOPT_H

#ifdef __ANDROID__
#define VERIFY_EXPORT  __attribute__((visibility("default")))
#elif __linux__
#define VERIFY_EXPORT  __attribute__((visibility("default")))
#else
#if defined(_USRDLL)
#define VERIFY_EXPORT  __declspec(dllexport)
#else
#define VERIFY_EXPORT
#endif
#endif

#if defined( __cplusplus)
extern "C" {
#endif

enum DeviceIDType
{
	//Adnroid
	WLAN_MAC = 0, //无限网卡
	ETH_MAC,//有线网卡
	ADNROID_ID,//安卓ID

	//Win
	MAC = 7,//mac 地址
	CPUID,//cpu id
	SMBIOSUUID,//SmBIOS UUID
	HardDiskAndMotherboardSn//磁盘序列号与主板序列号
};

/**
* @brief 设置JavaVM 安卓平台下调用接口设置
* @param[in] vm JavaVM变量
*/
VERIFY_EXPORT void SetJavaVM(void* vm);

/**
* @brief 安卓平台下设置上下文
* @param[in] pContext 上下文变量
*/
VERIFY_EXPORT void SetContext(void* pContext);

/**
* @brief 设置设备信息
* @param[in] deviceIDType 设备id类型
* @param[in] strDeviceID 设备唯一ID
* @param[in] strModel 型号
* @param[in] strManufacturer 厂商
*/
//VERIFY_EXPORT void SetDeviceInfo(DeviceIDType deviceIDType,const char* pstrDeviceID,const char* pstrModel,const char* pstrManufacturer);

/**
* @brief 获取接口返回码详细信息
* @param[in] iErrCode 接口返回码
* @param[in out] pErrInfo 详细信息
*/
VERIFY_EXPORT void GetAuthErrorInfo(const int iErrCode,char pErrInfo[100]);

/**
* @brief 输出日志
* @param[in] bEnabledLog 是否开启日志
* @param[in] pstrLogPath 日志输出文件路径 例如:/xxx/xxx/log.txt
*/
VERIFY_EXPORT void SetAuthLog(const bool bEnabledLog, char* pstrLogPath);

/**
* @brief 返回SDK版本号
* @param[in out] pstrSdkVer SDK版本号
*/
VERIFY_EXPORT void GetAuthSdkVer(char pstrSdkVer[50]);

VERIFY_EXPORT void LicDecrypt(char* pstrKey, char*pstrLic, char strDecrypt[2048]);

#if defined (__cplusplus)
}
#endif


#endif