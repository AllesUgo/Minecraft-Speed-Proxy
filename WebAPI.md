---
title: Minecraft-Speed-Proxy
language_tabs:
  - shell: Shell
  - http: HTTP
  - javascript: JavaScript
  - ruby: Ruby
  - python: Python
  - php: PHP
  - java: Java
  - go: Go
toc_footers: []
includes: []
search: true
code_clipboard: true
highlight_theme: darkula
headingLevel: 2
generator: "@tarslib/widdershins v4.0.30"

---

# Minecraft-Speed-Proxy WebAPI接口文档

WebAPI当前版本采用单用户登录方式，无需用户名，密码在配置文件设置。

你可以自行编写程序或网页，利用该接口文档实现对Minecraft-Speed-Proxy的远程控制。
# Interface


## POST 登录

POST /api/login

登录
登录成功后将会设置Cookie，包含一个关键的token字段作为后续鉴权令牌
后续所有需要身份验证的操作均需要该Cookie

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
  "message": "success"
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

### 返回头部 Header

|Status|Header|Type|Format|Description|
|---|---|---|---|---|
|200|Cookie|string||none|

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

