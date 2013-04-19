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

namespace Bonako
{
    /// <summary>
    /// グローバルオブジェクトです。
    /// </summary>
    public static class Global
    {
        #region PInvoke
        [DllImport("kernel32.dll")]
        extern static void GlobalMemoryStatusEx(ref MEMORYSTATUSEX lpBuffer);

        [StructLayout(LayoutKind.Sequential)]
        private struct MEMORYSTATUSEX
        {
            public uint dwLength;
            public uint dwMemoryLoad;
            public ulong ullTotalPhys;
            public ulong ullAvailPhys;
            public ulong ullTotalPageFile;
            public ulong ullAvailPageFile;
            public ulong ullTotalVirtual;
            public ulong ullAvailVirtual;
            public ulong ullAvailExtendedVirtual;
        }
        #endregion

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
        /// DFPN用のボナンザの操作用オブジェクトを取得します。
        /// </summary>
        public static Bonanza DfpnBonanza
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
        internal static PresentationUpdater Updater
        {
            get;
            private set;
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
        /// CPUのスレッド数を取得します。
        /// </summary>
        public static int GetCpuThreadNum()
        {
            return Environment.ProcessorCount;
        }

        /// <summary>
        /// 使用可能物理メモリの総量を取得します。
        /// </summary>
        public static long GetAvailPhys()
        {
            var ms = new MEMORYSTATUSEX();

            ms.dwLength = (uint)Marshal.SizeOf(ms);
            GlobalMemoryStatusEx(ref ms);
            return (long)ms.ullAvailPhys;
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
            FlintSharp.Utils.ScreenSize = new Size(640, 480);

            Settings = Settings.CreateSettings<Settings>();
            MainViewModel = new ViewModel.MainViewModel();
            ShogiModel = new ViewModel.ShogiModel();
            Updater = new PresentationUpdater(
                "http://garnet-alice.net/programs/bonako/update/versioninfo.xml");

            // 通常のボナンザと詰将棋用のボナンザを起動します。
            ResetBonanza(null, false);
            ResetBonanza(null, true);

            Updater.Start();
            EffectInfo.InitializeEffect(typeof(ViewModel.EffectTable));
        }

        /// <summary>
        /// ボナンザの設定を行います。
        /// </summary>
        static void ResetBonanza(AbortReason? reason, bool isDfpn)
        {
            if (reason == AbortReason.Aborted)
            {
                if (isDfpn)
                {
                    DfpnBonanza = null;
                }
                else
                {
                    Bonanza = null;
                }

                return;
            }

            // 初回起動時とエラー時はボナンザを起動します。
            var bonanza = new Bonanza();
            bonanza.Aborted += (_, e) => ResetBonanza(e.Reason, isDfpn);

            // エラー時は自動的に再接続に行きます。
            if (reason == AbortReason.Error)
            {
                bonanza.MnjInited += (_, __) => Commands.ExecuteConnect();
            }

            if (isDfpn)
            {
                DfpnBonanza = bonanza;
                MainViewModel.SetDfpnBonanza(bonanza);
            }
            else
            {
                Bonanza = bonanza;
                MainViewModel.SetBonanza(bonanza);
                ShogiModel.SetBonanza(bonanza);
            }
            
            // オブジェクト設定後に初期化します。
            bonanza.Initialize();
            if (!isDfpn)
            {
                ShogiModel.Sync(bonanza);
                //bonanza.BeginPrepareMnj();
            }

            // UIをすべて更新します。
            WPFUtil.InvalidateCommand();
        }

        /// <summary>
        /// 終了処理を行います。
        /// </summary>
        public static void Quit()
        {
            if (DfpnBonanza != null)
            {
                DfpnBonanza.Abort(AbortReason.Aborted, 0);
                DfpnBonanza = null;
            }

            if (Bonanza != null)
            {
                Bonanza.Abort(AbortReason.Aborted, 0);
                Bonanza = null;
            }
        }
    }
}
