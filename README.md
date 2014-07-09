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

1. Ragnarokライブラリ(https://github.com/ebifrier/Ragnarok)をダウンロード
2. godwhaleとRangarokを同じディレクトリに配置
3. godwhale/utility/Bonako/Bonako.sln をビルド

### ライセンス ###

* ボナンザ部分(src/common/bonanza6以下)に関しては、商業利用不可のボナンザライセンスを適用します。
* それ以外のソース、素材についてはGPL v3.0を適用します。

### 謝辞 ###

* ボナンザには非常にお世話になっております。作者の保木氏には感謝致します。
