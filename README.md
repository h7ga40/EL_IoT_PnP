# JSON形式のECHONET Liteの機器定義をIoT Plug and Playの機器定義へ変換

家電の通信規格[ECHONET Lite](https://echonet.jp/spec_g)は、機器の種類ごとの通信内容がAPPENDIXに詳細に定義されています。
その機器の定義をJSON形式で、HEMS(ECHONET Lite)認証支援センターから[Appendix Data](http://sh-center.org/sdk)として公開されています。

このソフトは、ECHONET Lite機器定義を[IoT Plug and Play](https://github.com/Azure/IoTPlugandPlay)で使われるDTDL(
Digital Twin Definition Language)のJSON形式に変換するものです。
ECHONET Lite機器をDTDL形式で定義することで、Azure IoTのデバイスとしての登録が簡単になるだろうという目論見です。

## 対応付け

各規格での定義を以下のように対応付けました。

|ECHONET Lite||IoT Plug and Play|
|-|-|-|
|ノード||CapabilityModel|
|機器オブジェクト||Interface|
|プロパティ|setのみ|Command|
||getのみ|Telemetry|
||set/get|Property|

このソフトでは、「ノード」の部分は変換していません。ECHONET Lite側は実態の定義ではなくガイドライン的な規格で、IoT Plug and Play側は実際の機器の定義なので、定義を保留してあります。実際に使うときには、出力されたJSONファイルの中のInterfaceを抜き出して、「CapabilityModel」に追加して使います。
[Microsoftの資料](https://docs.microsoft.com/ja-jp/azure/iot-pnp/overview-iot-plug-and-play)をもとした、作業が必要です。

## データの対応付け

|ECHONET Lite||IoT Plug and Play|
|-|-|-|
|state||Enum(integer)|
|object||Object|
|date_time||datetime|
|time||time|
|raw||string|
|array||Array|
|bitmap||Object|
|level||integer|
|number|int8, int16, int32, uint8, uint16|integer|
||uint32|long|
|numeric_value||integer|
|one_of||Object|

ECHONET Lite側はバイナリーフォーマットを定義していて、IoT Plug and Play側はクラウドや機器の開発言語（C, C#, JavaScript）で値を取り扱うための定義となっているので、定義の目的に差異があり単純には変換できないので、手作業が必要です。
このソフトは補助的なものと考えてください。
