using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Runtime.InteropServices;

using Ragnarok;
using Ragnarok.Shogi.Bonanza;
using Ragnarok.Presentation;
using Ragnarok.Presentation.Update;

namespace Bonako.DFPN
{
    /// <summary>
    /// グローバルオブジェクトです。
    /// </summary>
    public static class Global
    {
        /// <summary>
        /// メインビューモデルを取得します。
        /// </summary>
        public static MainViewModel MainViewModel
        {
            get;
            private set;
        }

        /// <summary>
        /// DFPN用のボナンザの操作用オブジェクトを取得します。
        /// </summary>
        public static Bonanza Bonanza
        {
            get;
            private set;
        }

        /// <summary>
        /// 設定オブジェクトを取得します。
        /// </summary>
        internal static Settings Settings
        {
            get;
            private set;
        }

        /// <summary>
        /// アプリ更新用オブジェクトを取得します。
        /// </summary>
        public static PresentationUpdater Updater
        {
            get;
            private set;
        }

        /// <summary>
        /// 静的コンストラクタ。
        /// </summary>
        static Global()
        {
            if (WPFUtil.IsInDesignMode)
            {
                return;
            }

            WPFUtil.Init();

            Settings = Settings.CreateSettings<Settings>();
            MainViewModel = new MainViewModel();
            Updater = new PresentationUpdater(
                "http://garnet-alice.net/programs/bonako-dfpn/update/versioninfo.xml");

            // 詰将棋用のボナンザを起動します。
            ResetBonanza(null);

            Updater.Start();
        }

        /// <summary>
        /// ボナンザの設定を行います。
        /// </summary>
        static void ResetBonanza(AbortReason? reason)
        {
            if (reason == AbortReason.Aborted)
            {
                Bonanza = null;
                return;
            }

            // 初回起動時とエラー時はボナンザを起動します。
            var bonanza = new Bonanza();
            bonanza.PropertyChanged += (_, __) => WPFUtil.InvalidateCommand();
            bonanza.Aborted += (_, e) => ResetBonanza(e.Reason);

            Bonanza = bonanza;
            MainViewModel.SetBonanza(bonanza);
            
            // オブジェクト設定後に初期化します。
            bonanza.Initialize("bonaster.exe");

            // UIをすべて更新します。
            WPFUtil.InvalidateCommand();
        }

        /// <summary>
        /// 終了処理を行います。
        /// </summary>
        public static void Quit()
        {
            if (Bonanza != null)
            {
                Bonanza.Abort(AbortReason.Aborted, 0);
                Bonanza = null;
            }
        }
    }
}
