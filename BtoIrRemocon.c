/**
 *
 * BtoIrRemocon.c
 *
 * 2013 KLab Inc.
 *
 * ビット・トレード・ワン社製「USB 接続赤外線リモコンキット」を操作する
 * Windows コンソールプログラム
 *
 * 同社製品ページ:
 * http://bit-trade-one.co.jp/BTOpicture/Products/005-RS/index.html
 *
 * This software is provided "as is" without any express and implied warranty
 * of any kind. The entire risk of the quality and performance of this software
 * with you, and you shall use this software your own sole judgment and
 * responsibility. KLab shall not undertake responsibility or liability for
 * any and all damages resulting from your use of this software.
 * KLab does not warrant this software to be free from bug or error in
 * programming and other defect or fit for a particular purpose, and KLab does
 * not warrant the completeness, accuracy and reliability and other warranty
 * of any kind with respect to result of your use of this software.
 * KLab shall not be obligated to support, update or upgrade this software.
 *
 */

 /*
  使い方：
  
  0. PC に USB 接続赤外線リモコンキット（以下"USB IR REMOCON"）を接続
  
  1.リモコンデータ受信モード
    BtoIrRemocon.exe を引数なしで実行するとリモコンデータ受信モードで起動する。
    手持ちの赤外線リモコンから USB IR REMOCON の赤外線 LED に向けて赤外線
    データを送るとコンソールに 14 文字のリモコンデータコードが表示される。
    例：C10220B0008030
 
  2.リモコンデータ送信モード
    BtoIrRemocon.exe を上記 1. のコードを引数として起動すると USB IR REMOCON
    の赤外線 LED からそのリモコンデータが照射される。コードは複数指定可能、
    途中に p<秒数> の要領でポーズ指定を記述できる。
    例：BtoIrRemocon C10220B0008030 p3 C10220B0008034 p3 C10220B0008036

*/

#include <windows.h>
#include <stdio.h>
#include <setupapi.h>

#pragma comment(lib, "setupapi")

#define RECEIVE_WAIT_MODE_NONE  0
#define RECEIVE_WAIT_MODE_WAIT  1

#define DEVICE_BUFSIZE       65
#define REMOCON_DATA_LENGTH   7

typedef void (__stdcall *DEF_HidD_GetHidGuid)(LPGUID HidGuid);

// PC に接続ずみの「USB 接続赤外線リモコンキット」のデバイスパスを取得
DWORD GetDevicePath(OUT char *pszDevicePath, IN DWORD cchBuf)
{
    // USB IR REMOCON 固有の ID 文字列
    char *szIdStr1 = "vid_22ea&pid_001e";
    char *szIdStr2 = "mi_03";
    int i;
    char *pszProp;
    HDEVINFO DeviceInfoTable = NULL;
    SP_DEVICE_INTERFACE_DATA DeviceIfData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceIfDetailData;
    SP_DEVINFO_DATA DevInfoData;
    DWORD InterfaceIndex, dwSize, dwError = ERROR_SUCCESS;
    HGLOBAL hMem = NULL;
    BOOL bFound;
    GUID InterfaceClassGuid;
    HMODULE hHidDLL;
    DEF_HidD_GetHidGuid pHidD_GetHidGuid;
    size_t len;

    //GUID InterfaceClassGuid = {0x4d1e55b2, 0xf16f, 0x11cf, 0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30};
    // HIDClass の GUID を取得
    hHidDLL = LoadLibrary("hid.dll");
    pHidD_GetHidGuid = (DEF_HidD_GetHidGuid)GetProcAddress(hHidDLL, "HidD_GetHidGuid");
    pHidD_GetHidGuid(&InterfaceClassGuid);
    FreeLibrary(hHidDLL);

    // HIDClass に属するデバイス群の含まれるデバイス情報セットを取得
    DeviceInfoTable = SetupDiGetClassDevs(&InterfaceClassGuid,
                            NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if(!DeviceInfoTable) {
        dwError = GetLastError();
        goto DONE;
    }
    // デバイス情報セットを走査し IR REMOCON デバイスを探す
    for (InterfaceIndex = 0; InterfaceIndex < 10000000; InterfaceIndex++) {
        DeviceIfData.cbSize = sizeof(DeviceIfData);
        if(SetupDiEnumDeviceInterfaces(DeviceInfoTable, NULL,
                            &InterfaceClassGuid, InterfaceIndex, &DeviceIfData)) {
            dwError = GetLastError();
            if (dwError == ERROR_NO_MORE_ITEMS) {
                goto DONE;
            }
        }
        else {
            dwError = GetLastError();
            goto DONE;
        }
        // 現在見ているデバイスの VID, PID の含まれるハードウェア ID 文字列を取得
        DevInfoData.cbSize = sizeof(DevInfoData);
        SetupDiEnumDeviceInfo(DeviceInfoTable, InterfaceIndex, &DevInfoData);
        SetupDiGetDeviceRegistryProperty(DeviceInfoTable, &DevInfoData,
                                    SPDRP_HARDWAREID, NULL, NULL, 0, &dwSize);
        hMem = GlobalAlloc(0, dwSize);
        if (!hMem) {
            dwError = GetLastError();
            goto DONE;
        }
        SetupDiGetDeviceRegistryProperty(DeviceInfoTable, &DevInfoData,
                            SPDRP_HARDWAREID, NULL, (PBYTE)hMem, dwSize, NULL);
        pszProp = strdup((char*)hMem);
        GlobalFree(hMem);
        hMem = NULL;
        
        // 取得したハードウェア ID 文字列に USB IR REMOCON 固有の VID, PID が含まれるか
        len = strlen(pszProp);
        for (i = 0; i < (int)len; i++) {
            pszProp[i] = tolower(pszProp[i]);
        }
        bFound = FALSE;
        if (strstr(pszProp, szIdStr1) != NULL && strstr(pszProp, szIdStr2) != NULL) {
            bFound = TRUE;
        }
        free(pszProp);

        // USB IR REMOCON 発見
        if (bFound) {
            // USB IR REMOCON のデバイスパスを得る
            SetupDiGetDeviceInterfaceDetail(DeviceInfoTable, &DeviceIfData,
                                                    NULL, 0, &dwSize, NULL);
            hMem = GlobalAlloc(0, dwSize);
            if (!hMem) {
                dwError = GetLastError();
                goto DONE;
            }
            pDeviceIfDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)hMem;
            pDeviceIfDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            if (SetupDiGetDeviceInterfaceDetail(DeviceInfoTable, &DeviceIfData,
                                        pDeviceIfDetailData, dwSize, NULL, NULL)) {
                if (cchBuf > dwSize - sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA)) {
                    // 成功
                    strcpy(pszDevicePath, pDeviceIfDetailData->DevicePath);
                    dwError = ERROR_SUCCESS;
                } else {
                    dwError = ERROR_NOT_ENOUGH_MEMORY;
                    goto DONE;
                }
                goto DONE;
            }
            else {
                dwError = GetLastError();
                goto DONE;
            }
        }
    }
DONE:
    if (hMem) {
        GlobalFree(hMem);
    }
    if (DeviceInfoTable) {
        SetupDiDestroyDeviceInfoList(DeviceInfoTable);
    }
    return dwError;
}

// リモコンデータ送信モード
int Transfer(HANDLE hDevice, int ac, char **av)
{
    int i, j, sts = -1;
    DWORD len;
    BYTE buf[DEVICE_BUFSIZE];
    char *pt, *e, s[3] = "\0\0";

    memset(buf, 0xFF, sizeof(buf));
    buf[0] = 0;
    buf[1] = 0x60; // 送信指定    

    // パラメータを順に走査して処理
    for (i = 1; i < ac; i++) {
        pt = av[i];
        fprintf(stderr, "[%s]\n", pt);
        // "pN" => N 秒間ポーズ
        if (tolower(*pt) == 'p') {
            if (pt[1] != '\0' && isdigit(pt[1])) {
                Sleep(1000 * atoi(&pt[1]));
                continue;
            } else {
                fprintf(stderr, "[%s] is skipped..\n", pt);
                continue;
            }
        }
        else if (strlen(pt) != REMOCON_DATA_LENGTH * 2) {
            continue;
        }
        // 16 進文字列をバイト配列へ
        for (j = 0; j < REMOCON_DATA_LENGTH; j++) {
            s[0] = pt[j*2];
            s[1] = pt[j*2+1];
            buf[j+2] = (BYTE)strtoul(s, &e, 16);
            if (*e != '\0') { // 変換エラー
                break;
            }
        }
        if (j != REMOCON_DATA_LENGTH) {
            fprintf(stderr, "[%s] is skipped..\n", pt);
            continue;
        }
        // 1件のリモコンデータを送信
        if (!WriteFile(hDevice, buf, DEVICE_BUFSIZE, &len, NULL)) {
            fprintf(stderr, "WriteFile: err=%u\n", GetLastError());
            goto DONE;
        }
    }
    memset(buf, 0xFF, sizeof(buf));
    buf[0] = 0;
    buf[1] = 0x40; // デバイスの送信バッファをクリア
    WriteFile(hDevice, buf, DEVICE_BUFSIZE, &len, NULL);
    memset(buf, 0x00, sizeof(buf));
    ReadFile(hDevice, buf, DEVICE_BUFSIZE, &len, NULL);
    sts = 0;
DONE:
    return sts;
}

// リモコンデータ受信モード
int Display(HANDLE hDevice)
{
    int i, sts = -1;
    DWORD len;
    BYTE buf[DEVICE_BUFSIZE];

    // デバイスの送信バッファをクリア
    memset(buf, 0xFF, sizeof(buf));
    buf[0] = 0;
    buf[1] = 0x40;
    WriteFile(hDevice, buf, DEVICE_BUFSIZE, &len, NULL);
    memset(buf, 0x00, sizeof(buf));
    ReadFile(hDevice, buf, DEVICE_BUFSIZE, &len, NULL);

    fprintf(stderr, "waiting...\n");

    // デバイスをリモコンデータ受信待ちモードに
    memset(buf, 0xFF, sizeof(buf));
    buf[0] = 0;
    buf[1] = 0x51;
    buf[2] = RECEIVE_WAIT_MODE_WAIT;
    if (!WriteFile(hDevice, buf, DEVICE_BUFSIZE, &len, NULL)) {
        fprintf(stderr, "WriteFile: err=%u\n", GetLastError());
        goto DONE;
    }
    memset(buf, 0x00, sizeof(buf));
    if (!ReadFile(hDevice, buf, DEVICE_BUFSIZE, &len, NULL)) {
        fprintf(stderr, "ReadFile: err=%u\n", GetLastError());
        goto DONE;
    }
    if (buf[1] != 0x51) {
        fprintf(stderr, "invalid response");
        goto DONE;
    }
    // リモコンデータ受信待ち
    while (TRUE) {
        memset(buf, 0xFF, sizeof(buf));
        buf[0] = 0;
        buf[1] = 0x50;
        WriteFile(hDevice, buf, DEVICE_BUFSIZE, &len, NULL);
        memset(buf, 0x00, sizeof(buf));
        ReadFile(hDevice, buf, DEVICE_BUFSIZE, &len, NULL);
        if (buf[1] == 0x50 && buf[2] != 0) {
            // 受信データありなら 16 進表示
            for (i = 0; i < 7; i++) {
                printf("%02X", buf[i+2]);
            }
            putchar('\n');
            break;
        }
        Sleep(500);
    }
    // リモコンデータ受信待ちモードを解除
    memset(buf, 0xFF, sizeof(buf));
    buf[0] = 0;
    buf[1] = 0x51;
    buf[2] = RECEIVE_WAIT_MODE_NONE;
    WriteFile(hDevice, buf, DEVICE_BUFSIZE, &len, NULL);
    memset(buf, 0x00, sizeof(buf));
    ReadFile(hDevice, buf, DEVICE_BUFSIZE, &len, NULL);
    sts = 0;

DONE:
    return sts;
}

// エントリーポイント
int main(int ac, char **av)
{
    int sts = -1;
    char szDevicePath[256];
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    BOOL doDisplay = FALSE;
    DWORD dwError;

    // 引数無しなら受信モード
    if (ac < 2) {
        doDisplay = TRUE;
    }
    // USB IR REMOCON のデバイスパスを得る
    dwError = GetDevicePath(szDevicePath, sizeof(szDevicePath));
    if (dwError != ERROR_SUCCESS) {
        fprintf(stderr, "failed to get device path: err=%u", dwError);
        goto DONE;
    }
    // データ送受信用にデバイスをオープン
    hDevice = CreateFile(szDevicePath, GENERIC_WRITE|GENERIC_READ,
            FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        fprintf(stderr, "CreateFile: err=%u\n", err);
        goto DONE;
    }
    if (doDisplay) {
        sts = Display(hDevice); // 受信モードへ
    } else {
        sts = Transfer(hDevice, ac, av); // 送信モードへ
    }

DONE:
    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
    }
    return sts;
}
