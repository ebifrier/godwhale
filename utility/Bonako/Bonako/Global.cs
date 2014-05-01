using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Runtime.InteropServices;

using Ragnarok;
using Ragnarok.Presentation;
using Ragnarok.Presentation.Update;
using Ragnarok.Presentation.Shogi.Effects;
using Ragnarok.Presentation.Shogi.View;
using Ragnarok.Shogi.Bonanza;

namespace Bonako
{
    /// <summary>
    /// グローバルオブジェクトです。
    /// </summary>
    public static class Global
    {
        /// <summary>
        /// ボナンザの起動ファイル名を取得します。
        /// </summary>
        public static readonly string ClientFileName = "client.exe";

        /// <summary>
        /// 並列化サーバーのアドレスを取得します。
        /// </summary>
        public static readonly string ServerAddress = "localhost"; //"133.242.205.114";

        /// <summary>
        /// 並列化サーバーのポート番号を取得します。
        /// </summary>
        public static readonly int ServerPort = 4082;

        /// <summary>
        /// 配布用ファイルかどうかを取得します。
        /// </summary>
        public static bool IsPublished
        {
            get
            {
#if PUBLISHED
                return true;
#else
                return false;
#endif
            }
        }

        /// <summary>
        /// 配布用ファイルであれば表示しません。
        /// </summary>
        public static Visibility VisibleIfNotPublished
        {
            get
            {
                return (IsPublished ? Visibility.Collapsed : Visibility.Visible);
            }
        }

        /// <summary>
        /// メインビューモデルを取得します。
        /// </summary>
        public static ViewModel.MainViewModel MainViewModel
        {
            get;
            private set;
        }

        /// <summary>
        /// 将棋局面用のモデルオブジェクトを取得します。
        /// </summary>
        public static ViewModel.ShogiModel ShogiModel
        {
            get;
            private set;
        }

        /// <summary>
        /// ボナンザの操作用オブジェクトを取得します。
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
        /// メインウィンドウを取得します。
        /// </summary>
        internal static MainWindow MainWindow
        {
            get;
            set;
        }

        /// <summary>
        /// 将棋曲面用のウィンドウを取得します。
        /// </summary>
        internal static View.ShogiWindow ShogiWindow
        {
            get;
            set;
        }

        /// <summary>
        /// エフェクト管理用オブジェクトを取得します。
        /// </summary>
        internal static ViewModel.EffectManager EffectManager
        {
            get
            {
                if (ShogiWindow == null)
                {
                    return null;
                }

                return ShogiWindow.EffectManager;
            }
        }

        /// <summary>
        /// 将棋用のコントロールを取得します。
        /// </summary>
        internal static ShogiControl ShogiControl
        {
            get
            {
                if (ShogiWindow == null)
                {
                    return null;
                }

                return ShogiWindow.ShogiControl;
            }
        }

        /// <summary>
        /// 初期化メソッド
        /// </summary>
        public static void Init()
        {
            if (WPFUtil.IsInDesignMode)
            {
                return;
            }

            WPFUtil.Init();
            FlintSharp.Utils.ScreenSize = new Size(640, 480);

            Settings = Settings.CreateSettings<Settings>();
            MainViewModel = new ViewModel.MainViewModel();
            ShogiModel = new ViewModel.ShogiModel();
            Updater = new PresentationUpdater(
                "http://garnet-alice.net/programs/bonako/update/versioninfo.xml");

            // ボナンザを起動します。
            ResetBonanza(null);

            Updater.Start();
            EffectInfo.InitializeEffect(typeof(ViewModel.EffectTable));
        }

        /// <summary>
        /// ボナンザの設定を行います。
        /// </summary>
        static void ResetBonanza(AbortReason? reason)
        {
            if (reason == AbortReason.Aborted ||
                reason == AbortReason.FatalError)
            {
                Bonanza = null;
                return;
            }

            // 初回起動時とエラー時はボナンザを起動します。
            var bonanza = new Bonanza();
            bonanza.PropertyChanged += (_, __) => WPFUtil.InvalidateCommand();
            bonanza.Aborted += (_, e) => ResetBonanza(e.Reason);

            // エラー時は自動的に再接続に行きます。
            if (reason == AbortReason.Error)
            {
                //bonanza.MnjInited += (_, __) => Commands.ExecuteConnect();
            }
            
            Bonanza = bonanza;
            MainViewModel.SetBonanza(bonanza);
            ShogiModel.SetBonanza(bonanza);
            
            // オブジェクト設定後に初期化します。
            bonanza.Initialize(ClientFileName);

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
