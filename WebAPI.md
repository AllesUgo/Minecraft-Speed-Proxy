
# Minecraft-Speed-Proxy WebAPI接口文档

WebAPI当前版本采用单用户登录方式，无需用户名，密码在配置文件设置。

你可以自行编写程序或网页，利用该接口文档实现对Minecraft-Speed-Proxy的远程控制。

可以在GitHub中查找由其他开发者开发的第三方管理面板，但请注意安全性。

# Authentication 鉴权

* API Key (apikey-header-Authorize)
    - Parameter Name: **Authorize**, in: header. 

通过login接口获取token后，在需要身份验证的请求中添加请求头部Authorize字段，值为token。

# Interfaces 接口

## POST 登录

POST /api/login

登录成功后将会返回token和生存时间，需要身份验证的请求在请求头部添加Authorize字段，值为token

> Body 请求参数

```json
{
  "password": "admin"
}
```

### 请求参数

|名称|位置|类型|必选|中文名|说明|
|---|---|---|---|---|---|
|body|body|object| 否 ||none|
|» password|body|string| 是 | 密码|none|

> 返回示例

> 200 Response

```json
{
  "status": 200,
  "message": "Login successful",
  "token": "17555886558072626",
  "token_expiry_time": 1755592255
}
```

### 返回结果

|状态码|状态码含义|说明|数据模型|
|---|---|---|---|
|200|[OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)|none|Inline|

### 返回数据结构

状态码 **200**

|名称|类型|必选|约束|中文名|说明|
|---|---|---|---|---|---|
|» status|integer|true|none||none|
|» message|string|true|none||none|
|» token|string|true|none||none|
|» token_expiry_time|integer|true|none||none|


## GET 获取在线用户列表

GET /api/get_online_users

返回当前在线的用户的所有信息

> 返回示例

> 200 Response

```json
{
  "online_users": [
    {
      "username": "User",
      "uuid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx",
      "ip": "::1",
      "current_proxy_size": 4253,
      "total_proxy_size": 0,
      "online_time_stamp": 1755062616,
      "proxy_target": "mc.hypixel.net:25565"
    }
  ],
  "status": 200,
  "message": "OK"
}
```

### 返回结果

|状态码|状态码含义|说明|数据模型|
|---|---|---|---|
|200|[OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)|none|Inline|

### 返回数据结构

状态码 **200**

|名称|类型|必选|约束|中文名|说明|
|---|---|---|---|---|---|
|» online_users|[object]|true|none||none|
|»» username|string|true|none||none|
|»» uuid|string|true|none||none|
|»» ip|string|true|none||none|
|»» current_proxy_flow|integer|true|none|本次代理流量|本次连接已经代理的流量|
|»» total_proxy_flow|integer|true|none||暂未启用|
|»» online_time_stamp|integer|true|none|登录时间戳|none|
|»» proxy_target|string|true|none|代理目标服务器|none|
|» status|integer|true|none||状态|
|» message|string|true|none||状态说明|

## GET 获取白名单

GET /api/get_whitelist

> 返回示例

> 200 Response

```json
{
  "whitelist_status": true,
  "white_list": [
    "User"
  ],
  "status": 200,
  "message": "OK"
}
```

### 返回结果

|状态码|状态码含义|说明|数据模型|
|---|---|---|---|
|200|[OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)|none|Inline|

### 返回数据结构

状态码 **200**

|名称|类型|必选|约束|中文名|说明|
|---|---|---|---|---|---|
|» whitelist_status|boolean|true|none||白名单是否开启|
|» white_list|[string]|true|none||none|
|» status|integer|true|none||none|
|» message|string|true|none||none|

## GET 获取黑名单

GET /api/get_blacklist

> 返回示例

> 200 Response

```json
{
  "black_list": [
    "User"
  ],
  "status": 200,
  "message": "OK"
}
```

### 返回结果

|状态码|状态码含义|说明|数据模型|
|---|---|---|---|
|200|[OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)|none|Inline|

### 返回数据结构

状态码 **200**

|名称|类型|必选|约束|中文名|说明|
|---|---|---|---|---|---|
|» black_list|[string]|true|none||none|
|» status|integer|true|none||none|
|» message|string|true|none||none|

## POST 向白名单添加用户

POST /api/add_whitelist_user

> Body 请求参数

```json
{
  "username": "User"
}
```

### 请求参数

|名称|位置|类型|必选|中文名|说明|
|---|---|---|---|---|---|
|body|body|object| 否 ||none|
|» username|body|string| 是 ||要添加用户名|

> 返回示例

> 200 Response

```json
{
  "status": 0,
  "message": "string"
}
```

### 返回结果

|状态码|状态码含义|说明|数据模型|
|---|---|---|---|
|200|[OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)|none|Inline|

### 返回数据结构

状态码 **200**

|名称|类型|必选|约束|中文名|说明|
|---|---|---|---|---|---|
|» status|integer|true|none||状态|
|» message|string|true|none||状态说明|

## POST 从白名单中删除用户

POST /api/remove_whitelist_user

> Body 请求参数

```json
{
  "username": "User"
}
```

### 请求参数

|名称|位置|类型|必选|中文名|说明|
|---|---|---|---|---|---|
|body|body|object| 否 ||none|
|» username|body|string| 是 ||要移除的用户名|

> 返回示例

> 200 Response

```json
{
  "status": 0,
  "message": "string"
}
```

### 返回结果

|状态码|状态码含义|说明|数据模型|
|---|---|---|---|
|200|[OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)|none|[基本响应数据结构](#schema基本响应数据结构)|

## POST 向黑名单添加用户

POST /api/add_blacklist_user

> Body 请求参数

```json
{
  "username": "string"
}
```

### 请求参数

|名称|位置|类型|必选|中文名|说明|
|---|---|---|---|---|---|
|body|body|object| 否 ||none|
|» username|body|string| 是 ||none|

> 返回示例

> 200 Response

```json
{
  "status": 0,
  "message": "string"
}
```

### 返回结果

|状态码|状态码含义|说明|数据模型|
|---|---|---|---|
|200|[OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)|none|Inline|

### 返回数据结构

状态码 **200**

|名称|类型|必选|约束|中文名|说明|
|---|---|---|---|---|---|
|» status|integer|true|none||状态|
|» message|string|true|none||状态说明|

## POST 从黑名单中删除用户

POST /api/remove_blacklist_user

> Body 请求参数

```json
{
  "username": "string"
}
```

### 请求参数

|名称|位置|类型|必选|中文名|说明|
|---|---|---|---|---|---|
|body|body|object| 否 ||none|
|» username|body|string| 是 ||none|

> 返回示例

> 200 Response

```json
{
  "status": 0,
  "message": "string"
}
```

### 返回结果

|状态码|状态码含义|说明|数据模型|
|---|---|---|---|
|200|[OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)|none|Inline|

### 返回数据结构

状态码 **200**

|名称|类型|必选|约束|中文名|说明|
|---|---|---|---|---|---|
|» status|integer|true|none||状态|
|» message|string|true|none||状态说明|

## GET 启用白名单

GET /api/enable_whitelist

> 返回示例

> 200 Response

```json
{
  "status": 0,
  "message": "string"
}
```

### 返回结果

|状态码|状态码含义|说明|数据模型|
|---|---|---|---|
|200|[OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)|none|Inline|

### 返回数据结构

状态码 **200**

|名称|类型|必选|约束|中文名|说明|
|---|---|---|---|---|---|
|» status|integer|true|none||状态|
|» message|string|true|none||状态说明|

## GET 禁用白名单

GET /api/disable_whitelist

> 返回示例

> 200 Response

```json
{
  "status": 0,
  "message": "string"
}
```

### 返回结果

|状态码|状态码含义|说明|数据模型|
|---|---|---|---|
|200|[OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)|none|Inline|

### 返回数据结构

状态码 **200**

|名称|类型|必选|约束|中文名|说明|
|---|---|---|---|---|---|
|» status|integer|true|none||状态|
|» message|string|true|none||状态说明|

## GET 获取用户代理表

GET /api/get_user_proxies

> 返回示例

> 200 Response

```json
{
  "default_proxy": "mc.hypixel.net:25565",
  "user_proxies": [
    {
      "username": "User",
      "proxy_target_addr": "example.com",
      "proxy_target_port": 25565
    }
  ],
  "status": 200,
  "message": "OK"
}
```

### 返回结果

|状态码|状态码含义|说明|数据模型|
|---|---|---|---|
|200|[OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)|none|Inline|

### 返回数据结构

状态码 **200**

|名称|类型|必选|约束|中文名|说明|
|---|---|---|---|---|---|
|» default_proxy|string|true|none||none|
|» user_proxies|[object]|true|none||none|
|»» username|string|true|none||none|
|»» proxy_target_addr|string|true|none||none|
|»» proxy_target_port|integer|true|none||none|
|» status|integer|true|none||none|
|» message|string|true|none||none|

## POST 设置用户代理

POST /api/set_user_proxy

> Body 请求参数

```json
{
  "username": "User",
  "proxy_address": "example.com",
  "proxy_port": 25565
}
```

### 请求参数

|名称|位置|类型|必选|中文名|说明|
|---|---|---|---|---|---|
|body|body|object| 否 ||none|
|» username|body|string| 是 ||none|
|» proxy_address|body|string| 是 ||none|
|» proxy_port|body|integer| 是 ||none|

> 返回示例

> 200 Response

```json
{
  "status": 0,
  "message": "string"
}
```

### 返回结果

|状态码|状态码含义|说明|数据模型|
|---|---|---|---|
|200|[OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)|none|Inline|

### 返回数据结构

状态码 **200**

|名称|类型|必选|约束|中文名|说明|
|---|---|---|---|---|---|
|» status|integer|true|none||状态|
|» message|string|true|none||状态说明|

## POST 删除用户代理

POST /api/remove_user_proxy

> Body 请求参数

```json
{
  "username": "User"
}
```

### 请求参数

|名称|位置|类型|必选|中文名|说明|
|---|---|---|---|---|---|
|body|body|object| 否 ||none|
|» username|body|string| 是 ||none|

> 返回示例

> 200 Response

```json
{
  "status": 0,
  "message": "string"
}
```

### 返回结果

|状态码|状态码含义|说明|数据模型|
|---|---|---|---|
|200|[OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)|none|Inline|

### 返回数据结构

状态码 **200**

|名称|类型|必选|约束|中文名|说明|
|---|---|---|---|---|---|
|» status|integer|true|none||状态|
|» message|string|true|none||状态说明|

## POST 设置最大在线用户数

POST /api/set_max_users

> Body 请求参数

```json
{
  "max_users": 100
}
```

### 请求参数

|名称|位置|类型|必选|中文名|说明|
|---|---|---|---|---|---|
|body|body|object| 否 ||none|
|» max_users|body|integer| 是 ||最大玩家数，-1表示无限制|

> 返回示例

> 200 Response

```json
{
  "status": 0,
  "message": "string"
}
```

### 返回结果

|状态码|状态码含义|说明|数据模型|
|---|---|---|---|
|200|[OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)|none|Inline|

### 返回数据结构

状态码 **200**

|名称|类型|必选|约束|中文名|说明|
|---|---|---|---|---|---|
|» status|integer|true|none||状态|
|» message|string|true|none||状态说明|

## POST 踢出用户

POST /api/kick_player

> Body 请求参数

```json
{
  "username": "User"
}
```

### 请求参数

|名称|位置|类型|必选|中文名|说明|
|---|---|---|---|---|---|
|body|body|object| 否 ||none|
|» username|body|string| 是 ||none|

> 返回示例

> 200 Response

```json
{
  "status": 0,
  "message": "string"
}
```

### 返回结果

|状态码|状态码含义|说明|数据模型|
|---|---|---|---|
|200|[OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)|none|Inline|

### 返回数据结构

状态码 **200**

|名称|类型|必选|约束|中文名|说明|
|---|---|---|---|---|---|
|» status|integer|true|none||状态|
|» message|string|true|none||状态说明|

## GET 退出登录

GET /api/logout

> 返回示例

> 200 Response

```json
{
  "status": 0,
  "message": "string"
}
```

### 返回结果

|状态码|状态码含义|说明|数据模型|
|---|---|---|---|
|200|[OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)|none|Inline|

### 返回数据结构

状态码 **200**

|名称|类型|必选|约束|中文名|说明|
|---|---|---|---|---|---|
|» status|integer|true|none||状态|
|» message|string|true|none||状态说明|

## GET 获取服务器启动时间戳和服务器当前时间戳

GET /api/get_start_time

> 返回示例

> 200 Response

```json
{
  "start_time": 1755524498,
  "now_time": 1755524521,
  "status": 200,
  "message": "Start time retrieved successfully"
}
```

### 返回结果

|状态码|状态码含义|说明|数据模型|
|---|---|---|---|
|200|[OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)|none|Inline|

### 返回数据结构

状态码 **200**

|名称|类型|必选|约束|中文名|说明|
|---|---|---|---|---|---|
|» start_time|integer|true|none|服务器开始时间戳|none|
|» now_time|integer|true|none|当前服务器时间戳|none|
|» status|integer|true|none||none|
|» message|string|true|none||none|

## GET 获取日志记录

GET /api/get_logs

> 返回示例

> 200 Response

```json
{
  "logs": [
    {
      "timestamp": 1755591375,
      "message": "玩家User uuid:xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx 登录于 127.0.0.1"
    },
    {
      "timestamp": 1755591377,
      "message": "玩家User uuid:xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx 退出于 127.0.0.1，在线时长2.000000秒，使用流量190.000000bytes"
    }
  ],
  "status": 200,
  "message": "Logs retrieved successfully"
}
```

### 返回结果

|状态码|状态码含义|说明|数据模型|
|---|---|---|---|
|200|[OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)|none|Inline|

### 返回数据结构

状态码 **200**

|名称|类型|必选|约束|中文名|说明|
|---|---|---|---|---|---|
|» logs|[object]|true|none||none|
|»» timestamp|integer|true|none||none|
|»» message|string|true|none||none|
|» status|integer|true|none||none|
|» message|string|true|none||none|

## POST 获取历史在线用户数

POST /api/get_online_number_list

> Body 请求参数

```json
{
  "start_time": 0,
  "end_time": 1755612174,
  "granularity": "minute"
}
```

### 请求参数

|名称|位置|类型|必选|中文名|说明|
|---|---|---|---|---|---|
|body|body|object| 否 ||none|
|» start_time|body|integer| 是 | 起始时间戳|none|
|» end_time|body|integer| 是 | 终止时间戳|none|
|» granularity|body|string| 是 | 粒度|支持minute、hour、day、week、month|

> 返回示例

> 200 Response

```json
{
  "user_numbers": [
    {
      "timestamp": 1755611904,
      "online_users": 0
    },
    {
      "timestamp": 1755611964,
      "online_users": 0
    },
    {
      "timestamp": 1755612024,
      "online_users": 0
    },
    {
      "timestamp": 1755612084,
      "online_users": 0
    },
    {
      "timestamp": 1755612174,
      "online_users": 0
    }
  ],
  "status": 200,
  "message": "User number list retrieved successfully"
}
```

### 返回结果

|状态码|状态码含义|说明|数据模型|
|---|---|---|---|
|200|[OK](https://tools.ietf.org/html/rfc7231#section-6.3.1)|none|Inline|

### 返回数据结构

状态码 **200**

|名称|类型|必选|约束|中文名|说明|
|---|---|---|---|---|---|
|» user_numbers|[object]|true|none||none|
|»» timestamp|integer|false|none||none|
|»» online_users|integer|false|none||none|
|» status|integer|true|none||none|
|» message|string|true|none||none|

# 数据模型

<h2 id="tocS_基本响应数据结构">基本响应数据结构</h2>

<a id="schema基本响应数据结构"></a>
<a id="schema_基本响应数据结构"></a>
<a id="tocS基本响应数据结构"></a>
<a id="tocs基本响应数据结构"></a>

```json
{
  "status": 0,
  "message": "string"
}

```

### 属性

|名称|类型|必选|约束|中文名|说明|
|---|---|---|---|---|---|
|status|integer|true|none||状态|
|message|string|true|none||状态说明|

