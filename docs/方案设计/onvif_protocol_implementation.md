ONVIF 协议栈手写实现：从零实现 PTZ 控制与认证

一、问题背景
在 EdgeXGuard 项目中，需要控制 ONVIF 摄像头的云台（PTZ）转动、获取设备信息和 RTSP 地址。常见的方案有两种：

方案	缺点
使用 gSOAP 生成代码	生成的代码极其臃肿（数万行），MinGW 编译困难，交叉编译复杂
使用现成 ONVIF 库	依赖重，需要额外链接，且很多库不支持 Windows/MinGW
目标：不依赖任何 ONVIF 专用库，手写一套轻量级 ONVIF 协议栈，只实现需要的功能（设备发现、设备信息、RTSP 地址、PTZ 控制）。

二、ONVIF 协议概述

ONVIF 的核心是基于 SOAP + HTTP 的 Web 服务协议。

┌─────────────────────────────────────────────────────────┐
│                      HTTP POST                          │
│  Content-Type: application/soap+xml; charset=utf-8     │
│                                                         │
│  ┌─────────────────────────────────────────────────┐    │
│  │                 SOAP Envelope                    │    │
│  │  ┌───────────────────────────────────────────┐  │    │
│  │  │           SOAP Header                      │  │    │
│  │  │  ┌─────────────────────────────────────┐  │  │    │
│  │  │  │     WS-UsernameToken 认证信息        │  │  │    │
│  │  │  └─────────────────────────────────────┘  │  │    │
│  │  └───────────────────────────────────────────┘  │    │
│  │  ┌───────────────────────────────────────────┐  │    │
│  │  │            SOAP Body                       │  │    │
│  │  │  ┌─────────────────────────────────────┐  │  │    │
│  │  │  │      ONVIF 操作请求                  │  │  │    │
│  │  │  │  (GetDeviceInfo / ContinuousMove)   │  │  │    │
│  │  │  └─────────────────────────────────────┘  │  │    │
│  │  └───────────────────────────────────────────┘  │    │
│  └─────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────┘

三、核心功能实现

3.1 设备发现（WS-Discovery）
ONVIF 设备通过 UDP 多播发送 Probe 消息。

void OnvifClient::discoverDevices() {
    QByteArray probe =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<Envelope xmlns=\"http://www.w3.org/2003/05/soap-envelope\" "
        "xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\">"
        "<Body><Probe><Types>dn:NetworkVideoTransmitter</Types></Probe></Body>"
        "</Envelope>";
    
    // 发送 UDP 多播到 239.255.255.250:3702
    m_udpSocket->writeDatagram(probe, QHostAddress("239.255.255.250"), 3702);
}
技术要点：

多播地址 239.255.255.250:3702 是 WS-Discovery 标准

只需实现 Probe 消息，不需处理 ProbeMatch 的全部字段，只提取服务地址

3.2 设备信息获取（GetDeviceInformation）
    void OnvifClient::requestDeviceInfo(const QString& serviceAddress) {
    QByteArray msg =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\" "
        "xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\">"
        + getWsSecurityHeader().toUtf8() +
        "<soap:Body>"
        "<tds:GetDeviceInformation/>"
        "</soap:Body>"
        "</soap:Envelope>";
    
    // 发送 HTTP POST 请求，解析响应的 XML 提取信息
}

3.3 RTSP 地址获取（GetStreamUri）

    void OnvifClient::requestStreamUri(const QString& serviceAddress) {
    // 1. 先调用 GetProfiles 获取 ProfileToken
    // 2. 再用 ProfileToken 调用 GetStreamUri 获取 RTSP 地址
    
    QByteArray getUriMsg = QString(
        "<soap:Envelope ...>"
        "%1"
        "<soap:Body>"
        "<trt:GetStreamUri>"
        "<trt:StreamSetup>"
        "<trt:Stream>RTP-Unicast</trt:Stream>"
        "<trt:Transport><trt:Protocol>RTSP</trt:Protocol></trt:Transport>"
        "</trt:StreamSetup>"
        "<trt:ProfileToken>%2</trt:ProfileToken>"
        "</trt:GetStreamUri>"
        "</soap:Body>"
        "</soap:Envelope>")
        .arg(getWsSecurityHeader())
        .arg(profileToken)
        .toUtf8();
}

3.4 WS-UsernameToken 认证（核心难点）
ONVIF 要求使用 WS-Security 规范，在 SOAP Header 中携带认证信息。

认证流程：
┌─────────────┐     ┌─────────────┐
│   客户端     │     │  摄像头      │
└──────┬──────┘     └──────┬──────┘
       │                   │
       │  1. 生成 Nonce     │
       │  2. 获取当前 UTC   │
       │  3. 计算 Digest    │
       │  4. 发送请求       │
       │ ─────────────────→ │
       │                   │ 5. 验证时间戳
       │                   │ 6. 验证 Digest
       │                   │ 7. 返回响应
       │ ←───────────────── │

代码实现：

QString OnvifClient::getWsSecurityHeader() {
    // 1. 生成 16 字节随机 Nonce
    QByteArray nonce;
    for (int i = 0; i < 16; i++) {
        nonce.append(QRandomGenerator::global()->generate() & 0xFF);
    }
    
    // 2. 获取 UTC 时间（ISO 8601 格式）
    QString created = QDateTime::currentDateTimeUtc()
                      .toString("yyyy-MM-ddThh:mm:ssZ");
    
    // 3. 计算 PasswordDigest = SHA1(Nonce + Created + Password)
    QByteArray combined = nonce + created.toUtf8() + m_password.toUtf8();
    QByteArray digest = QCryptographicHash::hash(combined, QCryptographicHash::Sha1);
    QString passwordDigest = digest.toBase64();
    
    // 4. 拼装 SOAP Header
    return QString(
        "<soap:Header>"
        "<wsse:Security xmlns:wsse=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd\" "
        "xmlns:wsu=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd\">"
        "<wsse:UsernameToken>"
        "<wsse:Username>%1</wsse:Username>"
        "<wsse:Password Type=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0#PasswordDigest\">%2</wsse:Password>"
        "<wsse:Nonce>%3</wsse:Nonce>"
        "<wsu:Created>%4</wsu:Created>"
        "</wsse:UsernameToken>"
        "</wsse:Security>"
        "</soap:Header>")
        .arg(m_username, passwordDigest, nonce.toBase64(), created);
}

技术要点：

Nonce 必须每次都不同，防止重放攻击

Created 必须是 UTC 时间，格式严格

PasswordDigest 算法是 SHA1(Nonce + Created + Password)

三个字段都要 Base64 编码后放入 XML

3.5 PTZ 连续移动控制

    bool OnvifClient::continuousMove(const QString& serviceAddress, 
                                  double x, double y, double zoom) {
    QString ptzUrl = serviceAddress;
    ptzUrl.replace("device_service", "ptz_service");
    
    // 1. 获取 ProfileToken（关键！不能硬编码）
    QString profileToken = getFirstProfileToken(serviceAddress);
    
    // 2. 构造 SOAP 请求
    QByteArray msg = QString(
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\" "
        "xmlns:tptz=\"http://www.onvif.org/ver20/ptz/wsdl\" "
        "xmlns:tt=\"http://www.onvif.org/ver10/schema\">"
        "%1"
        "<soap:Body>"
        "<tptz:ContinuousMove>"
        "<tptz:ProfileToken>%2</tptz:ProfileToken>"
        "<tptz:Velocity>"
        "<tt:PanTilt x=\"%3\" y=\"%4\" space=\"http://www.onvif.org/ver10/schema/PTZSpaces/ContinuousGenericSpace\"/>"
        "<tt:Zoom x=\"%5\" space=\"http://www.onvif.org/ver10/schema/PTZSpaces/ContinuousGenericSpace\"/>"
        "</tptz:Velocity>"
        "</tptz:ContinuousMove>"
        "</soap:Body>"
        "</soap:Envelope>")
        .arg(getWsSecurityHeader())
        .arg(profileToken)
        .arg(x).arg(y).arg(zoom)
        .toUtf8();
    
    // 3. 发送 HTTP POST 请求
    QNetworkReply* reply = m_networkManager->post(request, msg);
    // 处理响应...
}

3.6 获取 ProfileToken（动态解析）

    QString OnvifClient::getFirstProfileToken(const QString& serviceAddress) {
    // 发送 GetProfiles 请求
    QByteArray msg = QString(
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<soap:Envelope ...>"
        "%1"
        "<soap:Body><trt:GetProfiles/></soap:Body>"
        "</soap:Envelope>")
        .arg(getWsSecurityHeader())
        .toUtf8();
    
    // 解析响应，提取 token 属性
    QRegularExpression re("token=\"([^\"]+)\"");
    if (match.hasMatch()) {
        return match.captured(1);
    }
    return "Profile_1";  // fallback
}

四、踩坑与解决方案

4.1 认证方式选择
方式	代码量	兼容性	本项目选择
Basic Auth	简单	部分摄像头不支持	❌
WS-UsernameToken	复杂	标准 ONVIF 设备都支持	✅

4.2 ProfileToken 获取
错误做法：直接写 "Profile_1"，部分摄像头这个 token 不存在。

正确做法：动态调用 GetProfiles 接口，从响应中解析 token。

4.3 XML 命名空间
每个 SOAP 消息必须包含正确的命名空间，否则摄像头无法解析：

xmlns:soap="http://www.w3.org/2003/05/soap-envelope"
xmlns:tptz="http://www.onvif.org/ver20/ptz/wsdl"
xmlns:tt="http://www.onvif.org/ver10/schema"

4.4 Zoom 支持检测
项目中使用的 TP-Link TL-IPC45CL-V2 是 PT 云台（仅水平+垂直），不支持光学变焦。需要在 UI 中根据设备能力禁用 Zoom 控件。

4.5 SOAP 请求中的 Header 位置
WS-UsernameToken 必须放在 <soap:Header> 中，而不是 <soap:Body> 中。

六、验证方法
6.1 用 ONVIF Device Test Tool 对比
用 Test Tool 连接摄像头，抓取成功的请求

对比自己发出的请求，逐字段检查差异

6.2 用 Wireshark 抓包分析
    # 过滤 HTTP/XML 流量
    http or xml

6.3 用 Postman 调试
构造 HTTP POST 请求

设置 Content-Type: application/soap+xml

填入 SOAP 请求体

查看响应

七、经验总结
.不要依赖代码生成器：手写协议虽然麻烦，但可控、可移植、体积小。

.认证是最大坑：WS-UsernameToken 的 Nonce、Created、Digest 每个细节都要正确。

.动态获取 Token：永远不要硬编码设备返回的 token，它们可能变化。

.先验证硬件能力：不是所有摄像头都支持 Zoom，提前适配。

.抓包对比是调试利器：当请求不生效时，用 Wireshark 对比官方工具的成功请求。