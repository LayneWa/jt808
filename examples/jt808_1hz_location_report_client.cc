// MIT License
//
// Copyright (c) 2020 Yuming Meng
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// @File    :  jt808_1hz_location_report_client.cc
// @Version :  1.0
// @Time    :  2020/09/25 11:18:19
// @Author  :  Meng Yuming
// @Contact :  mengyuming@hotmail.com
// @Desc    :  None

#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

#include "jt808/client.h"


using LocationExtensions = std::map<uint8_t, std::vector<uint8_t>>;

namespace {

constexpr uint8_t kPositioningFixStatus = 0xEE;

int UpdateGNSSSatelliteNumber( uint8_t const& num, LocationExtensions* items) {
  if (items == nullptr) return -1;
  auto const& it = items->find(libjt808::kGnssSatellites);
  if (it != items->end()) {
    it->second.clear();
    it->second.push_back(num);
  } else {
   items->insert(
      std::make_pair(libjt808::kGnssSatellites, std::vector<uint8_t>{num}));
  }
  return 0;
}

void UpdateGNSSPositioningSolutionStatus(
    uint8_t const& fix, LocationExtensions* items) {
  auto const& it = items->find(kPositioningFixStatus);
  if (it != items->end()) {
    it->second.clear();
    it->second.push_back(fix);
  } else {
    items->insert(
        std::make_pair(kPositioningFixStatus, std::vector<uint8_t>{fix}));
  }
  // 检查后续自定义信息长度项是否存在.
  auto const& iter =
      items->find(libjt808::kCustomInformationLength);
  if (iter == items->end()) {
    items->insert(
      std::make_pair(libjt808::kCustomInformationLength,
                     std::vector<uint8_t>{0}));
  }
}
#if defined (_WIN32)
struct tm* localtime_r(const time_t* timep, struct tm* result)
{
    // 调用 localtime() 函数获取本地时间
    struct tm* tmp = localtime(timep);
    // 将 localtime() 函数返回的结果复制到 result 中
    if (tmp != nullptr)
    {
        memcpy(result, tmp, sizeof(struct tm));
    }
    return tmp;
}
#endif
std::string TimestampToString(int64_t const& timestamp) {
  struct tm tm_now;
  auto tt = static_cast<time_t>(timestamp);
  localtime_r(&tt, &tm_now);
  char date[16] = {0};
  snprintf(date, sizeof(date)-1, "%02d%02d%02d%02d%02d%02d",
		       (tm_now.tm_year+1900)%100, tm_now.tm_mon + 1, tm_now.tm_mday,
		       tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
  return std::string(date);
}

std::string GetTime(void) {
  return TimestampToString(
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch()).count());
}

}  // namespace



// string转wstring
std::wstring StringToWstring(const std::string str)
{
    unsigned len = str.size() * 2; // 预留字节数
    setlocale(LC_CTYPE, "");       //必须调用此函数
    wchar_t* p = new wchar_t[len]; // 申请一段内存存放转换后的字符串
    mbstowcs(p, str.c_str(), len); // 转换
    std::wstring str1(p);
    delete[] p;// 释放申请的内存
    return str1;
}

// wstring转string
std::string WstringToString(const std::wstring str)
{
    unsigned len = str.size() * 4;
    setlocale(LC_CTYPE, "");
    char* p = new char[len];
    wcstombs(p, str.c_str(), len);
    std::string str1(p);
    delete[] p;
    return str1;
}


int main(int argc, char **argv) {
  libjt808::JT808Client client;
  client.Init();
  client.SetRemoteAccessPoint("82.157.104.53", 7000);
  client.SetTerminalPhoneNumber("007755121422");
  client.set_location_report_inteval(1, true);
  // 重写JT808终端注册应答消息体解析函数.
  // auto& parser = client.parser();
  // if (libjt808::JT808FrameParserOverride(
  //     &parser, libjt808::kTerminalRegisterResponse,
  //     [] (std::vector<uint8_t> const& in,
  //         libjt808::ProtocolParameter* para) -> int {
  //       if (para == nullptr) return -1;
  //       uint16_t pos = libjt808::MSGBODY_NOPACKET_POS;
  //       auto const& msg_len = para->parse.msg_head.msgbody_attr.bit.msglen;
  //       // 应答流水号.
  //       para->parse.respone_flow_num = in[pos]*256 + in[pos+1];
  //       // 应答结果.
  //       para->parse.respone_result = in[pos+2];
  //       // 应答结果为0(成功)时解析出附加的鉴权码.
  //       if (para->parse.respone_result == libjt808::kRegisterSuccess) {
  //         auto begin = in.begin()+pos+3;
  //         auto end = begin + msg_len-3;
  //         para->parse.authentication_code.assign(begin, end);
  //       // 对于部分后台, 如果返回终端已注册, 还可能附带鉴权码.
  //       // 若附带鉴权码, 则可以使用该鉴权码正确连接到后台.
  //       } else if (para->parse.respone_result ==
  //                  libjt808::kTerminalHaveBeenRegistered) {
  //         if (msg_len > 3) {
  //           auto begin = in.begin()+pos+3;
  //           auto end = begin + msg_len-3;
  //           para->parse.authentication_code.assign(begin, end);
  //           para->parse.respone_result = libjt808::kRegisterSuccess;
  //         }
  //       }
  //       return 0;
  //     })) ;


  // 设置终端注册信息.
  // Args:
  //     p_id:  省域ID.
  //     c_id:  市县域ID.
  //     m_id:  制造商ID, 最长5字节.
  //     t_model:  终端型号, 最长20字节.
  //     t_id:  终端ID, 最长7字节.
  //     c_color:  车牌颜色.
  //     c_num:  车牌号码.
  // Returns:
  //     None.

  libjt808::RegisterInfo register_info;

  constexpr uint8_t kManufacturerId[] = {
    'S', 'K', 'O', 'E', 'M'
  };

  constexpr uint8_t kTerminalModel[] = {
    'S', 'K', '9', '1', '5', '1'
  };

  constexpr uint8_t kTerminalId[] = {
    '0', '0', '0', '0', '0', '1'
  };

  register_info.province_id = 0x002c;
  register_info.city_id = 0x012c;
  register_info.manufacturer_id.assign(
      kManufacturerId, kManufacturerId+sizeof(kManufacturerId));
  register_info.terminal_model.assign(
      kTerminalModel, kTerminalModel+sizeof(kTerminalModel));
  register_info.terminal_id.assign(
      kTerminalId, kTerminalId+sizeof(kTerminalId));
  register_info.car_plate_color = libjt808::VehiclePlateColor::kBlue;
  register_info.car_plate_num = WstringToString(L"粤") + "B12345";
//u8"粤B12345";

  client.SetTerminalRegisterInfo(register_info);
  std::string str;
  str.clear();

  if (register_info.car_plate_color != libjt808::VehiclePlateColor::kVin) {
    str.clear();
    str.assign(register_info.car_plate_num.begin(),
               register_info.car_plate_num.end());
    printf("  car plate number: %s\n", str.c_str());
  }


  if ((client.ConnectRemote() == 0) &&
      (client.JT808ConnectionAuthentication() == 0)) {
    //client.UpdateLocation(22.570336, 113.937577, 54.0f, 60, 0, GetTime());

    client.UpdateLocation(0, 0, 0.0f, 0, 0, GetTime());
    libjt808::StatusBit status_bit {};
    //status_bit.bit.positioning = 1;  // 已成功定位.
    client.SetStatusBit(status_bit.value);
    auto& location_extensions = client.GetLocationExtension();
    //UpdateGNSSSatelliteNumber(11, &location_extensions);
    //UpdateGNSSPositioningSolutionStatus(2, &location_extensions);
    client.Run();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    uint32_t pos_flag = 0;
    auto tp_beg = std::chrono::steady_clock::now();
    auto tp_end = tp_beg;
    while (client.service_is_running()) {
      // 模拟定位模块1s更新一次定位数据.
      tp_end = std::chrono::steady_clock::now();
      if (std::chrono::duration_cast<
            std::chrono::milliseconds>(tp_end-tp_beg).count() >= 1000) {
        tp_beg = tp_end;
        if ((++pos_flag / 10) % 2 == 0) {  // 每10秒切换一次位置.
          //client.UpdateLocation(22.570336, 113.937577, 54.0f, 60, 0, GetTime());
	  client.UpdateLocation(0, 0, 0.0f, 0, 0, GetTime());
        } else {
          client.UpdateLocation(22.570336, 113.938577, 54.0f, 50, 90, GetTime());
        }
        client.GenerateLocationReportMsgNow();
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    }
    client.Stop();
  }
  return 0;
}
