Computer Networks pRj2
===

> Author: b05902127 劉俊緯, b05902117 陳柏文

## Program & Protocol Design

* All command's format

```json
{
   "opcode" : "char",
   "length" : "unsgined long long"
}
```

* Types of Opcode

| opcode | details                                                      |
| ------ | ------------------------------------------------------------ |
| s      | **Sing up**: send usr/pwd to register an account.            |
| l      | **Login** : send usr/pwd to login.                           |
| m      | **Messaging** : send a message.                              |
| r      | **Refresh** : update message history.                        |
| u      | **Upload File** : Upload a file.                             |
| d      | **Download file** : Download a file                          |
| a      | **All user**: Show all user/ friend list/ black list of login account. |
| x      | **Edit Friend / Black List** |
| !      | **Server message**                                           |

#### Sign up

##### Request

```json
{
    "username" : "string",
    "password" : "string"
}
```

##### Response

```json
{
    "status_code" : int,
    "state" : "string",
    "data" : {}
}
```

#### Login

##### Request

```json
{
    "user_name" : "string",
    "password" : "string"
}
```

##### Response


```json
{
    "status_code" : int,
    "state" : "string",
    "data" : {}
}
```

#### Message
##### Request

```json
{
    "target" : "int",
    "type" : "message/data",	
    "message" : "string"
}
```

##### Response


```json
{
    "status_code" : int,
    "state" : "string",
    "data" : []
}
```

#### Refresh

##### Request

```json
{
    "target" : "int",
    "start_from" : int,
    "end_to" : int,
}
```

##### Response


```json
{
    "status_code" : int,
    "state" : "string",
    "data" : {
        line_number: {
            "who" : int
        	"type" : ""
        	"message" : ""
        }
    }
}
```

#### Upload File

#####  Request

```json
{
    "target" : "int",
    "filename" : "string",
    "data" : base64(file),
}
```
##### Response

```json
{
    "status_code" : int,
    "state" : "string",
    "data" : {}
}
```

#### Download File

#####  Request

```json
{
    "target" : "int",
    "filename" : "string",
    "FILEID" : "string",
}
```
##### Response

```json
{
    "status_code" : int,
    "state" : "string",
    "data" : [base64(file)]
}
```

#### Show All User

#####  Request

```json
{
    "target" : "string",
    // b: blak_list, f: friend_list, a: user_list.
}
```
##### Response


```json
{
    "status_code" : int,
    "state" : "string",
    "data" :
    [
        ["id1","name1"],
        ["id2","name2"]
    ]
}
```

#### Edit Black / Friend List

#####  Request

```json
{
    "op" : "string",
    // add / delete
    "target" : "int"
}
```

##### Response


```json
{
    "status_code" : int,
    "state" : "string",
    "data" : []
}
```


## User & Operator Guide

## How2Run?

## Bonus



