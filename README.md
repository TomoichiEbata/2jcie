
# 前提

## Makefileの改造

gdbを使う為に、  

CFLAGS  = -Wall -Wextra -g

としているが、もともとは、  

CFLAGS  = -Wall -Wextra

となっている

## 起動条件

■デバイスロックされていたら、これを実行する

$ sudo rm /var/lock/LCK..ttyUSB5

■プロセスが止まらなかったら、これを実行する  

$ ps -ef | grep 2jcie-bu01  
        6454  5272 21 18:40 pts/3    00:00:02 ./2jcie-bu01 /dev/ttyUSB5 1 

$ sudo kill -9 6454


## 起動

// これは動く  
$ ./2jcie-bu01 /dev/ttyUSB5 0

// これは動かない(バグみたい)  
$ ./2jcie-bu01 /dev/ttyUSB5 1


## その他
in sensor_data.c  

#define LATEST_ADDR             (0x5022)  // Table71


# 2JCIE-BUをLinux等で動かす
([オムロンのセンサーをラズパイで使う](https://eetimes.jp/ee/articles/1912/26/news034_6.html) に詳しい記載があります)

![2JCIE-BU](https://image.itmedia.co.jp/ee/articles/1912/26/mm191226diy26.jpg)


2JCIE-BUをLinux等で動かすために、Contributorの方がpythonのサンプルプログラムを開示されていますが、私はserialのライブラリのバージョン問題（serial, pyserial, python3-serial）に直面して、四苦八苦していました。ソースコードにもいろいろな変更を試みたのですが、結局動かすことができませんでした。

「もうダメかなぁ」と諦めかけていたころ、たまたま、このページを見つけました（アットマークテクノ様、助かりました。ありがとうございました）。

取りあえず、本日のところは、このセンサーからラズパイでデータ1行のみ取り出すところまでの、私の手順を開示します（次回以降、処理の自動化と、センサーデータをWebでグラフ化するところまで持っていきます）。

それでは、以下に、私が試した手順を説明します。

まず、"2JCIE-BU01"をUSBポートに差し込んで、ラズパイをリブートします。リブート後に、以下を行ってください。

```$ lsusb
Bus 001 Device 004: ID 0590:00d4 Omron Corp. ←これがあることを確認
Bus 001 Device 006: ID 1e0e:9001 Qualcomm / Option
Bus 001 Device 005: ID 0424:7800 Standard Microsystems Corp.
Bus 001 Device 003: ID 0424:2514 Standard Microsystems Corp. USB 2.0 Hub
Bus 001 Device 002: ID 0424:2514 Standard Microsystems Corp. USB 2.0 Hub
Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
```
環境センサーが正常に接続されているならば、「Omron Corp.」という情報のデバイスが表示され、これが環境センサーのデバイス情報になります。このデバイス情報から「USB ID」（上記例では[0590:00d4]）が確認できます。

確認したUSB IDを「/sys/bus/usb-serial/drivers/ftdi_sio/new_id」に記入します。

```
$ sudo modprobe ftdi_sio
$ sudo sh -c "echo 0590 00d4 > /sys/bus/usb-serial/drivers/ftdi_sio/new_id"
```

このデバイスが、ttyUSB*のどれかに割り当たるのは分かっているのですが、その番号を知るためには、メッセージの内容を読み取る必要があります。

```
$ dmesg | grep ttyUSB
[13.191062] usb 1-1.1.3: GSM modem （1-port） converter now attached to ttyUSB0
[13.194454] usb 1-1.1.3: GSM modem （1-port） converter now attached to ttyUSB1
[13.201466] usb 1-1.1.3: GSM modem （1-port） converter now attached to ttyUSB2
[13.203155] usb 1-1.1.3: GSM modem （1-port） converter now attached to ttyUSB3
[13.204087] usb 1-1.1.3: GSM modem （1-port） converter now attached to ttyUSB4
[154.579585] usb 1-1.3: FTDI USB Serial Device converter now attached to ttyUSB5
```

このように、私のラズパイでは、ttyUSB5に割り当たっています（これは環境によって変わります）

しかし、こんなこと毎回やっていたら面倒です。自動化する方法は、前述の参照ページに記載があるので、それを試してください（私の環境ではうまく動作させることができなかったので、現在別手段を検討中です）。

次に、こちらのサンプルプログラム（C言語）をダウンロードして、適当なディレクトリに展開してください。
```
$ tar xzvf 2jcie-bu01-usb.tar.gz
$ cd 2jcie-bu01-usb-master
$ ls
data_output.c  data_output.o  Makefile sensor_data.h common.h data_output.h main.c sensor_data.c  sensor_data.o sensor_data.h
```
とした後、
```
$ make
```
とすると、私のラズパイでは、
```
main.c: In function ‘main’:
main.c:185:11: error: ‘exit_restore’ undeclared （first use in this function）
    return exit_restore;
（以下省略）
```

というエラーが出てきたので、main.cの一部を、

```
//goto exit_restore;
restore_serial（fd, &tio）;
exit（-1）;
```
のように書き直して（3箇所くらいあった）、無理やりビルドまで通して、実行ファイル"2jcie-bu01"を作りました。

実行結果は以下の通りです。

```
$ ./2jcie-bu01 /dev/ttyUSB5 0
Mode : Get Latest Data.
Temperature, Relative humidity, Ambient light, Barometric pressure, Sound noise, eTVOC, eCO2, Discomfort index, Heat stroke
24.52,53.03,75,1012.602,67.11,108,1113,71.45,21.23
Program all success.
```
コマンドを入力する度に、値が微妙に変化しているので、正常に動作しているものと思われます。センサー値さえ取得できれば、後はなんとかなると思いますので、今回はここまでにしたいと思います。

# 「ラズパイ」で実家を見える化する
([「ラズパイ」で実家を見える化する](https://eetimes.jp/ee/articles/2001/30/news036_5.html) に詳しい記載があります)


では、ここから後半になります。

前回は、ラズパイとその専用の通信デバイスからなる、DIYの見守りシステムの最小構成の作り方を説明し、オムロンにご協力頂き、同社の「環境センサ 2JCIE-BU」を、私のラズパイでも、動かせるようにしました。

今回は、後半では、前回に引き続きAmazonで5000円程度で購入できるラズパイを使った、実家の老親を見守る最小システムで、実家の親の状態を数値とグラフで「見える化」する方法の一つをご紹介致します。

前回、C言語を使って、2JCIE-BUからセンサーデータを取り出すところまで成功しましたので、今回は、これをグラフで表示する機能を作ってみたいと思います。

まず、2JCIE-BUからセンサーデータを、一分に一回程度取得できれば十分だと考えましたので、前回のプログラムにほとんど変更を加えずに、最小の負荷で「見える化」を実現してみたいと思います。

最初に、こちらの圧縮ファイルをダウンロードして、ホームディレクトリに展開してください。

## [Step.1] 2JCIE-BUからセンサーデータをcsvファイルで保存する

思いっ切り手を抜くために、前回のプログラムからセンサーデータの部分だけを表示をするようにして、リダイレクトでcsvファイルとしてセーブをしてしまいます。

このプログラムでは、センサー情報以外のデータは出力されませんので、このようにして、csvファイルの作成を試みてください。

```
$ sudo modprobe ftdi_sio
$ sudo sh -c "echo 0590 00d4 > /sys/bus/usb-serial/drivers/ftdi_sio/new_id"
```
（上記は初期設定なので自動化するのが望ましいが、面倒なので後回し）

```
pi@raspberrypi:~/2jcie$ ./2jcie-bu01 /dev/ttyUSB5 0 > data_test.csv
```
これで、data_test.csvにセンサーデータが確保されていれば成功です。以下はサンプルです。

```
pi@raspberrypi:~/2jcie$ cat data_test.csv
25.39,51.92,22,1003.275,44.47,625,1926,72.49,21.74
```

## [Step.2] 定期的にセンサーデータファイル（data_test.csv）を更新する

これもいろいろやり方はあるのですが、やはり最もラクな方法として、crontabを使います。

```
/etc/crontab
```
に以下を追記

#改行なし1行で記載
```
*/1 *	* * *	pi    /home/pi/2jcie/2jcie-bu01 /dev/ttyUSB5 0 > /home/pi/2jcie/data_test.csv 
```
これで1分おきにデータが更新されるようになります。

さて、ここから、データの変化をグラフで表示する、htmlファイルを作成します。

今回は、グラフ表示で手を抜くために、折れ線グラフ、棒グラフ、円グラフ、レーダーチャートなど、6種類のグラフが簡単に描けてしまうJavascriptのライブラリであるChart.jsと、そのリアルタイムストリーミングデータ向けプラグインである、chartjs-plugin-streaming.jsを使いました。

## [Step.3] ラズパイにテスト用のWebサーバを立てる

意外に知られていませんが、テスト的にWebサーバを立ち上げる場合、apacheやらniginxをインストールしなくても、コマンド一発で簡単に立ち上げる方法がいくつかあります。

今回は、pythonを使った一発コマンドで"~/2jcie"の中のhtml用のwebサーバを立ち上げます。

```
pi@raspberrypi:~/2jcie$ python3 -m http.server
```

これで、http://211.158.177.150:8000/xxxx.html で、ターゲットのhtmlファイルにアクセスできるようになります（このIPアドレスはダミーです）。

## [Step.4] 温度表示用の"temp.html"ファイルを動かしてみる

実は今回のプログラムで一番苦労したのが、htmlファイルにcsvファイルを読む込ませることでした。セキュリティの問題だか何だか知りませんが、ローカルで立ち上げたhtmlファイルには、ローカルのファイルが読み込めないというルールが課せられているようです。

また、csvファイルを読む込むだけのサンプルプログラムが見つからなくて、随分苦労しました ―― という愚痴はさておき。

ブラウザから、http://211.158.177.150:8000/temp.html と入力してください。

![TEMP](https://image.itmedia.co.jp/ee/articles/2001/30/mm20200130kaigo21.jpg)


上記のような表示が出てくれば成功です。

さらに、http://211.158.177.150:8000/2jcie.html と入力すれば、


![2JCIE](https://image.itmedia.co.jp/ee/articles/2001/30/mm20200130kaigo22.jpg)

というような画面が登場するハズです。

改良の余地はありますが、取りあえず今回は、速攻で作りましたので、見栄えについては、今後の課題にさせてください（というか、好き勝手に改造してください）。

プログラムの内容についての説明は、割愛します。他の人のサンプルをかき集めて、どうにかこうにか動かしたものですので、うまく説明できません。

今回の成果は、chartjs-plugin-streaming.jsを使ったjavaScriptにcsvファイルからの読み込みを成功させた、ということになると考えています。

「そんなショボイことが成果？」と言われるかもしれませんが、こういうショボイことが突破できなくてシステム構築に失敗する、ということは結構あります。

これで何ができると問われれば、取りあえず、実家の室温、既に就寝したか（部屋は暗くなっているか）、テレビがついているか（騒音はあるか）、などは、すぐに分かると思います。

このようなグラフを使った監視では、カメラによる映像監視のようにプライバシーの問題を回避しつつ、本質的な実家の見守りが可能になります。



