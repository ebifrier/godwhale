using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

using Ragnarok;
using Ragnarok.Presentation;
using Ragnarok.Presentation.Control;
using Ragnarok.Presentation.Command;

namespace Bonako.DFPN
{
    public static class Commands
    {
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
        /// 詰将棋サーバーへ接続します。
        /// </summary>
        public static readonly RelayCommand Connect =
            new RelayCommand(ExecuteConnect, CanExecuteConnect);

        /// <summary>
        /// 名前には英数字とアンダーバーしか使えません。
        /// </summary>
        private static readonly Regex NameRegex = new Regex(
            @"^([a-zA-Z0-9_])+$");

        /// <summary>
        /// 詰将棋サーバーへ接続します。
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
                bonanza.ConnectToDfpn(
                    "garnet-alice.net",
                    4085,
                    model.Name,
                    model.ThreadNum,
                    model.HashMemSize);
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
        private static bool CanExecuteConnect()
        {
            var bonanza = Global.Bonanza;
            if (bonanza == null)
            {
                return false;
            }

            return !bonanza.IsConnected;
        }
        #endregion
    }
}
