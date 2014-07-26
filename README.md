# 大合神クジラちゃん #

これは大合神クジラちゃんという将棋ソフトのリポジトリです。
このソフトは以下の特徴を持っています。

* ボランティアクラスタ（＝各パソコンで指し手を計算）を採用しています。
* 局面を表示するGUIを付属しています。
* 読売新聞に記事が載りました。

### サーバーのビルドと実行方法 ###

1. cd src/server
2. make
3. ./godwhale < param.txt

対局時間などの必要な設定はparam.txtから行ってください。

### クライアントのビルドと実行方法 ###

1. cd src/client
2. make
3. ./one_godwhale < param.txt
 
スレッド数などの設定はparam.txtから行ってください。

### クライアントGUIのビルド方法 ###

1. Ragnarok(<https://github.com/ebifrier/Ragnarok>)をダウンロード
2. godwhaleとRangarokを同じディレクトリに配置
3. godwhale/utility/Bonako/Bonako.sln をビルド

### ライセンス ###

* ボナンザ部分(src/common/bonanza6以下)に関しては、商業利用不可のボナンザライセンスを適用します。
* utility/Bonako/Bonako/ShogiDataディレクトリのデータは、素材それぞれの著作権に従います。
* それ以外のソースについてはGPL v3.0を適用します。

### 謝辞 ###

* 内部で使っている将棋ソフトのBonanzaには大変お世話になっております。作者の保木氏に深く感謝致します。
