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

基於以上的protocol，我們可以分成server方和client方。以下我們分別講述server和client的設計方法。

### Server

| Operator               | Description & Details                                        |
| ---------------------- | ------------------------------------------------------------ |
| sign up/ friend list   | 每個user都會有自己的user file。這個file會紀錄帳號，密碼，他的friend list/black list。<br />如果有更新request，就會直接重寫檔案。 |
| login                  | 每次重開server，它會抓取所有人的檔案資料。此時如果有socket登入，它就會檢查帳號與密碼後，將這個socket的狀態設為Login。 所有動作除了sign up以外都會檢查帳戶是否有login。 |
| messaging              | 如果a要傳訊息給b，那麼在history裡面會有a_b.txt。這個檔案會紀錄歷史以來的聊天紀錄，誰講的，是檔案提醒還是一般訊息，還有這是第幾個訊息。<br />client送訊息過來後會直接append在檔案尾端。 |
| refresh                | 我們可以將refresh做成要求整段紀錄的某幾個連續部位。在server這邊就會讀取對應的history，並抓出特定的連續區段文，傳回給client。 |
| upload / download file | client會將base64(file)傳給server，server為了避免衝突，所以後面會加一個random number，並利用history紀錄誰上傳了一個文件，以及文件名稱是什麼。至於要download file的話，client會告訴server他要什麼檔案，server會再傳回去。<br />client此時debase64(str)就會拿到原本的file。 |

### Client

| Operator               | Description & Details                                        |
| ---------------------- | ------------------------------------------------------------ |
| signup/ login           | 當使用者輸入帳號密碼，就將它包按上述的protocol做成json package中並傳給server，並接收server回傳的資訊來判斷成功。如果註冊成功ㄧ樣回回到初始頁面，進一步登入成功的話就會進入程式的主要頁面。 |
| Messaging         | 在主畫面輸入'm'即可進到選擇對象的頁面，接著輸入id之後便會進入聊天室中。 |
| Lists   | 在主畫面輸入't'會進到顯示lists的頁面，在其中可以選擇要查看all user/fiend list 或是 black list，接著把使用者要求包成json後傳送給server，成功的話就能得到該list，client接著會parse好並顯示。查看完後也可輸入m來進入Messaging頁開始選人聊天。 |
| Edit list       | 在主畫面輸入'x'會進到編輯lists的頁面，首先選擇想要對friend/black list做操作，接著選擇要對該list做什麼操作(add/delete)，最後輸入id，就會把上述行為包成json，並傳給server等回應。一樣編輯也可輸入m來進入Messaging頁。 |
| Chat | 當選好聊天對象之後便可開始聊天，此時如果輸入的並非指令(u/d/r/q)，就會被當成訊息傳送，會把訊息內容做base64 encode後包進json傳給server，並等待寫入成功的訊息。最後成功後便會執行Refresh把最新10筆聊天記錄抓回來並parse好。 |
| Upload/ Download | 在聊天過程中如果輸入的為u/d，即分別代表上傳/下載檔案，均須先輸入欲處理的檔案名稱，下載時還要額外輸入想存放的位置。上傳時會把檔案內容打開讀進buffer後，base64 encode完再放入json。下載時會則會把得到的json寫進file中。此系統最大支援上傳檔案大小為500KB。 |
| Refresh | 取得聊天紀錄，預設是取最新的十行，當進到chat room時可以輸入r後進到瀏覽歷史紀錄的頁面，選擇u/d可以選擇要看往上1行or往下1行。 |

## How2Run?

#### Compile

```bash
make
```

#### Run server

```bash
./piepie-server port_number
```
#### Run client

```bash
./piepie-client hostname:port_number
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

為了避免server臨時斷線而造成使用者的不便，我們額外開一個socket與server連接，並且偵測server是否存活，如果server斷線了，client會自動重新要求連線直到server回復正常，並會自動幫使用者連上。

##### Message Encryption

由於傳輸過程中有可能遭到竊聽，我們加密訊息的方式是把原本的base64的訊息，以古典加密的方式，位移64格，

即(ascii_number+64)%128，如此一來，雖不如現今的加密演算法理論上無法破解，但仍大大的增加了攻擊者攻擊的cost。而且解密時也可以用相同的函式解出。

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
