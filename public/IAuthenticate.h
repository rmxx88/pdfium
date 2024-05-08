#ifndef IAUTHENTICATE_H
#define IAUTHENTICATE_H

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

typedef struct OfflineCertificateInfo
{
    char* pstrOfflineCertificate = nullptr;//脱机码
    char* pstrPublicKey = nullptr;//公钥
    char* pstrAppID = nullptr; //应用id
}OFFLINECERTIFICATEINFO_T;

/**
* @brief 鉴权 (优先级:在线->离线->豁免期)
* @param[in] offlineCertificateInfo 脱机证书鉴权相关信息结构体
* @param[in] pstrEncFilePath 授权文件路径(可读可写),例如/xxx/xxx/enc.txt 或者 为授权文件内容(isString 为true时)
* @param[in,out] sucType 返回校验成功的类型 1:服务端返回的授权码 2:离线授权码 3:豁免期 4:脱机离线码 -1:失败
* @param[in,out] pstrmodules 返回模块权限,逗号隔开 例如(HPV,HBE)
* @param[in] isString： false则证书从授权文件中读取， true则证书直接从接口中传入
* @return 0表示成功，返回详细信息使用GetAuthErrorInfo接口获取
*/
VERIFY_EXPORT int Authenticate(OfflineCertificateInfo* offlineCertificateInfo,const char* pstrEncFilePath,int& sucType,char pstrmodules[500],bool isString = false);

#if defined (__cplusplus)
}
#endif


#endif

