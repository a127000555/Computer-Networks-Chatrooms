Computer Networks pRj2
===

> Author: b05902127 劉俊緯, b05902117 陳柏文

## Protocol Design

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
    "filename" : "string"
}
```
##### Response

```json
{
    "status_code" : int,
    "state" : "string",
    "data" : base64(file) (its string.)
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
    "target_list" : "string",
    // black / friend
    "target_id" : "int"
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

## System & Program Design

基於以上的protocol，我們可以分成server方和client方。以下我們分別講述server和client的成設計方法。

### Server

| Operator               | Description & Details                                        |
| ---------------------- | ------------------------------------------------------------ |
| sign up/ friend list   | 每個user都會有自己的user file。這個file會紀錄帳號，密碼，他的friend list/black list。<br />如果有更新request，就會直接重寫檔案。 |
| login                  | 每次重開server，它會抓取所有人的檔案資料。此時如果有socket登入，它就會檢查帳號與密碼後，將這個socket的狀態設為Login。 所有動作除了sign up以外都會檢查帳戶是否有login。 |
| messaging              | 如果a要傳訊息給b，那麼在history裡面會有a_b.txt。這個檔案會紀錄歷史以來的聊天紀錄，誰講的，是檔案提醒還是一般訊息，還有這是第幾個訊息。<br />client送訊息過來後會直接append在檔案尾端。 |
| refresh                | 我們可以將refresh做成要求整段紀錄的某幾個連續部位。在server這邊就會讀取對應的history，並抓出特定的連續區段文，傳回給client。 |
| upload / download file | client會將base64(file)傳給server，server為了避免衝突，所以後面會加一個random number，並利用history紀錄誰上傳了一個文件，以及文件名稱是什麼。至於要download file的話，client會告訴server他要什麼檔案，server會再傳回去。<br />client此時debase64(str)就會拿到原本的file。 |

### Client



## User & Operator Guide

## How2Run?

#### Compile

```bash
make
```

#### Run server

```bash
./piepie-server port_number
```

## Bonus

### Extra Operation 
##### Friend List

##### Black List

### Client Protection & Usability

##### Client Auto Reconnect

##### Encryption

### Server Protection & Security

##### Server Failover

##### No Plaintext Password

##### File Hashing



