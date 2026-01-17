# はじめに

このプロジェクトは Boot Cable USBと呼ばれるUSBでPCとGame Boy Advanceを接続する機器のドライバと読み書きをするソフトウェアです。
かつては専用ドライバを導入していましたが、64bit環境で動作しなくなったため、WinUSBに移行しWindows10では動作確認していましたが、Windows11では動作しなくなったため、このプロジェクトで動作を復活させようとしています。


# グランドデザイン

- VS2026(v145)を使ってビルドすることを想定する
- 動作をさせることを優先して可読性は後回しで良いが、新たに実装した関数には日本語でコメントを入れること
- 調査のためのprintfは適宜入れてよいが、終わったら消すこと

# ファイル構成

- usb_driver WinUSBドライバ定義
- btcons
  - device.cpp, h デバイス固有定義
  - piohost.c Boot Cable IO定義
  - main.cpp メイン
  - usb.cpp, h USB IO API

# ビルド方法

- btcable_controler.slnをVisual Studio Build Toolsでビルドする
- 作成先(x64/Debug or Release)のbtcons.exeを実行

*********** BOOT CABLE USB CONSOLE TOOL Ver 3.541 ************
Error can't found BootCable USB !!

が表示された場合、正しくBootCableを認識できていない。


*********** BOOT CABLE USB CONSOLE TOOL Ver 3.541 ************
Endpoint index: 0 Pipe type: Bulk Pipe ID: 0x1 (Timeout set, AutoClearStall).
 -> Mapped to cmd_h (0)
Endpoint index: 1 Pipe type: Bulk Pipe ID: 0x82 (Timeout set, AutoClearStall).
 -> Mapped to i_h (1)
Endpoint index: 2 Pipe type: Bulk Pipe ID: 0x2 (Timeout set, AutoClearStall).
 -> Mapped to o_h (2)
Sending PGM size (len=262144)...
Warning: PGM size 262144 exceeds protocol limit. Caps to 65535 words (262140 bytes).
Sending PGM body...
...............
PGM body sent: 262140 / 262140 bytes.

といった表示がされれば成功
