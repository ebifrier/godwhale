using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

using Ragnarok;
using Ragnarok.Presentation;
using Ragnarok.Presentation.Control;
using Ragnarok.Presentation.Command;

namespace Bonako
{
    public static class Commands
    {
        #region Check to update
        /// <summary>
        /// アプリの新バージョンをチェックします。
        /// </summary>
        public static readonly RelayCommand CheckToUpdate =
            new RelayCommand(ExecuteCheckToUpdate);

        /// <summary>
        /// アプリの新バージョンをチェックします。
        /// </summary>
        private static void ExecuteCheckToUpdate()
        {
            try
            {
                var updater = Global.Updater;

                updater.CheckToUpdate(TimeSpan.FromSeconds(20));
            }
            catch (Exception ex)
            {
                Util.ThrowIfFatal(ex);

                DialogUtil.ShowError(
                    "新バージョンの確認に失敗しました。");
            }
        }
        #endregion

        #region Version Info
        /// <summary>
        /// バージョン情報を表示します。
        /// </summary>
        public static readonly RelayCommand ShowVersionInfo =
            new RelayCommand(ExecuteShowVersionInfo);

        /// <summary>
        /// バージョン情報を表示します。
        /// </summary>
        private static void ExecuteShowVersionInfo()
        {
            try
            {
                var window = new VersionWindow(null);

                window.ShowDialog();
            }
            catch (Exception ex)
            {
                Util.ThrowIfFatal(ex);

                DialogUtil.ShowError(
                    "バージョン情報の表示に失敗しました。");
            }
        }
        #endregion

        #region WriteBonanza
        /// <summary>
        /// ボナンザにコマンドを出力します。
        /// </summary>
        public static readonly RelayCommand<string> WriteBonanza =
            new RelayCommand<string>(ExecuteWriteBonanza);

        /// <summary>
        /// ボナンザにコマンドを出力します。
        /// </summary>
        public static void ExecuteWriteBonanza(string command)
        {
            var bonaObj = Global.Bonanza;
            if (bonaObj == null)
            {
                return;
            }

            bonaObj.WriteCommand(command);
        }
        #endregion

        #region Connect
        /// <summary>
        /// 並列化サーバーへ接続します。
        /// </summary>
        public static readonly RelayCommand Connect =
            new RelayCommand(ExecuteConnect/*, CanExecuteConnect*/);

        /// <summary>
        /// 名前には英数字とアンダーバーしか使えません。
        /// </summary>
        private static readonly Regex NameRegex = new Regex(
            @"^([a-zA-Z0-9_])+$");

        /// <summary>
        /// 並列化サーバーへ接続します。
        /// </summary>
        public static void ExecuteConnect()
        {
            var bonanza = Global.Bonanza;
            if (bonanza == null)
            {
                return;
            }

            var model = Global.MainViewModel;
            if (model == null)
            {
                return;
            }

            if (!NameRegex.IsMatch(model.Name))
            {
                DialogUtil.ShowError(
                    "名前には英数字とアンダーバーしか使えません (-o-;)");
                return;
            }

            try
            {
                // 並列化サーバーへの接続コマンドを発行します。
                bonanza.Connect(
                    "153.127.241.151", //"garnet-alice.net",
                    4084, 4085,
                    model.Name,
                    model.ThreadNum,
                    model.HashMemSize);
            }
            catch (Exception ex)
            {
                DialogUtil.ShowError(ex,
                    "並列化サーバーへの接続に失敗しました。");
            }
        }

        /// <summary>
        /// 並列化サーバーへの接続コマンドが実行可能か調べます。
        /// </summary>
        private static bool CanExecuteConnect()
        {
            var bonanza = Global.Bonanza;
            if (bonanza == null)
            {
                return false;
            }

            return (bonanza.IsMnjInited == true && !bonanza.IsConnected);
        }
        #endregion

        #region ConnectToDfpn
        /// <summary>
        /// 詰将棋サーバーへ接続します。
        /// </summary>
        public static readonly RelayCommand ConnectToDfpn =
            new RelayCommand(ExecuteConnectToDfpn, CanExecuteConnectToDfpn);

        /// <summary>
        /// 詰将棋サーバーへ接続します。
        /// </summary>
        public static void ExecuteConnectToDfpn()
        {
            var bonanza = Global.DfpnBonanza;
            if (bonanza == null)
            {
                return;
            }

            var model = Global.MainViewModel;
            if (model == null)
            {
                return;
            }

            if (!NameRegex.IsMatch(model.Name))
            {
                DialogUtil.ShowError(
                    "名前には英数字とアンダーバーしか使えません (-o-;)");
                return;
            }

            try
            {
                // 並列化サーバーへの接続コマンドを発行します。
                bonanza.ConnectToDfpn(
                    "153.127.241.151", //"garnet-alice.net",
                    4085,
                    model.Name,
                    model.ThreadNum,
                    model.HashMemSize);

                /*var moves = Ragnarok.Shogi.BoardExtension.MakeMoveList(
                    Ragnarok.Shogi.SampleMove.BonaTest);
                var moveList = Ragnarok.Shogi.BoardExtension.ConvertMove(
                    new Ragnarok.Shogi.Board(), moves);
                var autoPlay = new Ragnarok.Presentation.Shogi.AutoPlay(
                    new Ragnarok.Shogi.Board(), moveList)
                    {
                        IsChangeBackground = true,
                    };

                Global.ShogiWindow.ShogiControl.StartAutoPlay(autoPlay);*/
            }
            catch (Exception ex)
            {
                DialogUtil.ShowError(ex,
                    "詰将棋サーバーへの接続に失敗しました。");
            }
        }

        /// <summary>
        /// 詰将棋サーバーへの接続コマンドが実行可能か調べます。
        /// </summary>
        private static bool CanExecuteConnectToDfpn()
        {
            var bonanza = Global.DfpnBonanza;
            if (bonanza == null)
            {
                return false;
            }

            return !bonanza.IsConnected;
        }
        #endregion
    }
}
