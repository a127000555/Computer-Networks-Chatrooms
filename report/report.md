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
| x      | **Edit Friend / Black List**                                 |
| !      | **Server message**                                           |
| t      | **Tell the username if id is given.**                        |

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
#### Id -> username

#####  Request

```json
{
    "target" : "int"
}
```

##### Response


```json
{
    "status_code" : int,
    "state" : "string",
    "data" : "string"
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

假設現在有很多人，那麼我們可以用friend list來快速查找你熟悉的人。

此外，我們的friend list是可刪除的。

##### Black List

假設你討厭某個人，你可以把它加入黑名單。
此後如果你要拿到你們的聊天紀錄或傳訊息，server都會請你原諒它。

此外，我們的black list是可刪除的。

### Client Protection & Usability

##### Client Auto Reconnect

##### Message Encryption

### Server Protection & Security

##### Server Failover

為了避免一些神奇的外力因素，或者client搞鬼，所以server除了SIGINT或一些uncatchable signal以外，一切忽略。

或是當client傳了一些server無法處理而crash的時候，server也會自動回復。

##### No Plaintext Password

由於Server存明文password會有很大的資安漏洞(只要server被pwn那麼所有人密碼都會外流)，因此我們的password是有SHA256過得。

##### File Hashing

為了避免file有問題，server也提供一個很貼心的服務。
在upload file的時候，除了後面會有magic number以外，也會附上sha256sum。

而這個sha256sum是base64過後的，所以user要確認是否檔案下載無誤，可以使用以下這個指令。

```bash
base64 -w 0 XD.pdf | sha256sum
```

