# Boot Cable USB Console Tool v4.0

## 概要

Boot Cable USBを使用してPCとゲームボーイアドバンス(GBA)を接続し、自作プログラムの転送やデバッグをおこなうためのツールです。
かつては専用のドライバが必要でしたが、Windows 10/11 (x64) 環境で動作するように WinUSB ベースのドライバに変更し、ツール自体もそれに合わせて更新されています。

> [!NOTE]
> 本ツールは `DeviceInterfaceGUID` として `{5a3a0cee-8b81-4134-b1c4-9af9a8e59c21}` を使用するようにハードコードされています。
> ドライバのインストール時 (`btusb.inf`) および `btcons.exe` の実行時にはこの GUID が一致している必要があります。

## 動作環境

- Windows 10 / 11 (64bit)
- Visual Studio 2022 (または Build Tools 2022)

## ビルド方法

1. `btcable_controler.sln` を Visual Studio で開きます。
2. ソリューション構成を `Release`、プラットフォームを `x64` に設定します。
3. ソリューションのビルドを実行します。
4. `x64/Release` フォルダに `btcons.exe` が生成されます。

## ドライバのインストール手順

Windows 11などでドライバの署名強制によりインストールできない問題を回避しつつ、WinUSBドライバを適用する方法です。

### 1. デバイスの接続
Boot Cable USB を PC に接続します。
デバイスマネージャー上で「不明なデバイス」またはドライバが正しく当たっていないデバイスとして認識されていることを確認してください。

### 2. ドライバの更新
1. デバイスマネージャーで該当デバイスを右クリックし、「ドライバーの更新」を選択します。
2. 「コンピューターを参照してドライバーを検索」を選択します。
3. このリポジトリ内の `usb_driver` フォルダを指定します。
4. インストールが完了し、正常に認識されることを確認してください。

※ もし INF ファイルの署名問題でインストールできない場合は、Windowsを「ドライバー署名の強制を無効にする」モードで起動してインストールするか、Zadig などのツールを使用して WinUSB ドライバをインストールすることを検討してください。

## 実行・確認手順

コマンドプロンプトや PowerShell で `btcons.exe` を実行します。

```powershell
./x64/Release/btcons.exe
```

### 成功時
以下のようにエンドポイントが認識され、待機状態または転送などが始まれば成功です。

```text
*********** BOOT CABLE USB CONSOLE TOOL Ver 4.0 ************
Endpoint index: 0 Pipe type: Bulk Pipe ID: 0x1 (Timeout set, AutoClearStall).
 -> Mapped to cmd_h (0)
Endpoint index: 1 Pipe type: Bulk Pipe ID: 0x82 (Timeout set, AutoClearStall).
 -> Mapped to i_h (1)
...
```

### 失敗時
デバイスが見つからない場合は以下のように表示されます。ドライバが正しく認識されているか再度確認してください。

```text
*********** BOOT CABLE USB CONSOLE TOOL Ver 4.0 ************
Error can't found BootCable USB !!
```

## コマンド一覧

`btcons.exe` はコマンドライン引数で動作モードを指定します。

### ROM/RAM操作

| コマンド | 説明 | 使用法 |
| :--- | :--- | :--- |
| **FR** | Flash ROM 読み込み | `btcons fr <filename> [offset] [length]` |
| **FW** | Flash ROM 書き込み | `btcons fw <filename> [offset] [length]` |
| **FA** | Flash ROM 追記 | `btcons fa <filename>` |
| **FE** | Flash ROM 全消去 | `btcons fe` |
| **SR** | Save RAM (SRAM/Flash/EEP) 読み込み | `btcons sr <filename> [offset] [length]` |
| **SW** | Save RAM (SRAM/Flash/EEP) 書き込み | `btcons sw <filename> [offset] [length]` |

#### 引数について
- `<filename>`: 読み書きするファイルパス
- `[offset]`: 開始オフセット (10進数 または `0x`で始まる16進数)
- `[length]`: 長さ (10進数, 16進数, または `k`, `m` 接尾辞が使用可能 例: `0x1000`, `256k`, `1m`)
- 省略時はデバイス全体、またはファイルサイズ全体が対象となります。

### プログラム転送・実行

| コマンド | 説明 | 使用法 |
| :--- | :--- | :--- |
| **(なし)** | プログラムロード & 実行 | `btcons <pgm_file> [d]` |

- `<pgm_file>`: 転送して実行するGBAプログラム (.gba / .bin)
- `[d]`: デバッグモード (切断エミュレーションなどのデバッグ用機能有効化)

