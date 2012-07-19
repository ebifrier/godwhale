----------------------------------------------------------------------
         Bonanza 6.0 - Scripts for Cluster Parallelizatoin
                                              Kunihito Hoki, May 2011
----------------------------------------------------------------------


1. Legal Notices
-----------------

This program is protected by copyrights. Without a specific 
permission, any means of commercial applications are prohibited.
Furthermore, this program is distributed without any warranty.
Within these limits, you can use, redistribute, and/or modify it.


2. 使い方
------------------

このディレクトリには2011年5月に開催された第21回世界コンピュータ将棋選手権で Bonanza が使用したサーバスクリプト群が展開されます。TCP/IP ソケット通信が可能なクラスター環境で動作します。

- majority_server.pl

多数決合議を行う Perl スクリプト。CSA 対局サーバのクライアントとして動作します。bonanza や parallel_server.pl がこのサーバスクリプトのクライアントです。実行例は run-server.sh に示されています。

--client_port        クライアントとの通信接続のために listen するポート番号
--client_num         対局開始に必要なクライアント数
--csa_host           CSA 対局サーバのホスト名
--csa_port           CSA 対局サーバのポート番号
--csa_id             CSA 対局サーバに報告する名前
--csa_pw             CSA 対局サーバに報告するパスワード
--sec_limit          持ち時間（秒）
--sec_limit_up       持ち時間が切れてからの秒読み
--time_response      予想される通信遅延 (秒)
--time_stable_min    序盤における思考切り上げを行う最低時間 (秒)
--final_as_confident final 信号を confident 扱いするフラグ。

これはあから２０１０の合議サーバを基に作られたものです。電気通信大学伊藤研究室修士（元）の小幡拓弥様、東京大学大学院総合文化研究科の金子知適助教から多数のバグレポートをいただきました。ありがとうございます。また、合議法の研究を推進してくださった電気通信大学伊藤毅志助教と社団法人情報処理学会に感謝します。



- parallel_server.pl

ルート並列化探索を行う Perl スクリプト。多数決合議サーバ (majority_server.pl) のクライアントとして動作します。bonanza がこのサーバスクリプトのクライアントです。実行例は run-parallel.sh に示されています。多数決合議サーバとは異なり、クライアントとの通信が切断された場合には動作を終了します。

--client_port        クライアントとの通信接続のために listen するポート番号
--client_num         対局開始に必要なクライアント数
--server_host        合議サーバのホスト名
--server_port        合議サーバのポート番号
--server_id          合議サーバに報告するクライアント名
--split_nodes        並列探索前に行う事前探索のノード数



- dfpn_server.pl

DFPN 詰将棋探索のサーバプログラム。DFPN 詰将棋探索を行うワーカは bonanza から dfpn connect コマンドで、詰将棋探索の結果を利用するクライアントは bonanza から dfpn_client コマンドで接続します。クライアントからルート局面を受け取り、この局面とその全子局面をワーカに投げ、結果をクライアントに報告します。仕事の優先順位は１. ルート、2. 最善手により移行する子局面、3. その他全子局面です。ワーカーとクライアントの数は複数あることを想定しています。クライアントが異なる局面をルートとして思考していても正しく動作します。

--client_port        クライアントやワーカとの通信接続のために listen する
                     ポート番号
