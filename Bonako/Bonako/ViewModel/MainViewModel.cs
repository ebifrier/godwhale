using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;

using Ragnarok;
using Ragnarok.ObjectModel;
using Ragnarok.Utility;
using Ragnarok.Shogi.Bonanza;

namespace Bonako.ViewModel
{
    /// <summary>
    /// ログの各行の情報を保持します。
    /// </summary>
    public sealed class LogLine
    {
        /// <summary>
        /// ログ用のテキストを取得します。
        /// </summary>
        public string RawText
        {
            get;
            private set;
        }

        /// <summary>
        /// ログがボナンザからの出力かどうかを取得します。
        /// </summary>
        public bool? IsOutput
        {
            get;
            private set;
        }

        /// <summary>
        /// 加工済みのログ用テキストを取得します。
        /// </summary>
        public string Text
        {
            get
            {
                var prefix = string.Empty;
                if (IsOutput == true)
                {
                    prefix = "< ";
                }
                else if (IsOutput == false)
                {
                    prefix = "> ";
                }

                return (prefix + RawText);
            }
        }

        /// <summary>
        /// コンストラクタ
        /// </summary>
        public LogLine(string text, bool? isOutput)
        {
            if (string.IsNullOrEmpty(text))
            {
                throw new ArgumentNullException("text");
            }

            RawText = text.Trim();
            IsOutput = isOutput;
        }
    }

    /// <summary>
    /// 
    /// </summary>
    public sealed class MainViewModel : NotifyObject
    {
        private readonly NotifyCollection<LogLine> logList =
            new NotifyCollection<LogLine>();
        private Bonanza bonanza;

        /// <summary>
        /// クライアント名を取得または設定します。
        /// </summary>
        [DependOnProperty(typeof(Settings), "AS_Name")]
        public string Name
        {
            get { return Global.Settings.AS_Name; }
            set { Global.Settings.AS_Name = value; }
        }

        /// <summary>
        /// スレッド数を取得または設定します。
        /// </summary>
        [DependOnProperty(typeof(Settings), "AS_ThreadNum")]
        public int ThreadNum
        {
            get { return Global.Settings.AS_ThreadNum; }
            set { Global.Settings.AS_ThreadNum = value; }
        }

        /// <summary>
        /// スレッド数の最大値を取得または設定します。
        /// </summary>
        public int ThreadNumMaximum
        {
            get { return GetValue<int>("ThreadNumMaximum"); }
            private set { SetValue("ThreadNumMaximum", value); }
        }

        /// <summary>
        /// メモリの使用量をリストにします。
        /// </summary>
        public List<Tuple<int, int>> MemSizeList
        {
            get;
            private set;
        }

        /// <summary>
        /// 使用ハッシュサイズを取得または設定します。
        /// </summary>
        [DependOnProperty(typeof(Settings), "AS_HashMemSize")]
        public int HashMemSize
        {
            get { return Global.Settings.AS_HashMemSize; }
            set { Global.Settings.AS_HashMemSize = value; }
        }

        /// <summary>
        /// CPU使用率を取得します。
        /// </summary>
        public double CpuUsage
        {
            get { return GetValue<double>("CpuUsage"); }
            set { SetValue("CpuUsage", value); }
        }

        /// <summary>
        /// NPS値[k]を取得します。
        /// </summary>
        public double Nps
        {
            get { return GetValue<double>("Nps"); }
            set { SetValue("Nps", value); }
        }

        /// <summary>
        /// ボナンザのログ内容を取得または設定します。
        /// </summary>
        public NotifyCollection<LogLine> LogList
        {
            get { return this.logList; }
        }

        /// <summary>
        /// 変化リストを取得または設定します。
        /// </summary>
        public NotifyCollection<VariationInfo> VariationList
        {
            get { return Global.ShogiModel.VariationList; }
        }

        /// <summary>
        /// ボナンザの初期化が終わったかどうかを取得します。
        /// </summary>
        [DependOnProperty(typeof(Bonanza), "IsMnjInited")]
        public bool? IsMnjInited
        {
            get
            {
                if (this.bonanza == null)
                {
                    return null;
                }

                return this.bonanza.IsMnjInited;
            }
        }

        /// <summary>
        /// 並列化サーバーに接続しているかどうかを取得します。
        /// </summary>
        [DependOnProperty(typeof(Bonanza), "IsConnected")]
        public bool IsConnected
        {
            get
            {
                if (this.bonanza == null)
                {
                    return false;
                }

                return this.bonanza.IsConnected;
            }
        }

        /// <summary>
        /// ボナンザログに出力します。
        /// </summary>
        public void AppendBonanzaLog(string log, bool? isOutput)
        {
            if (string.IsNullOrEmpty(log))
            {
                return;
            }

            Ragnarok.Presentation.WPFUtil.UIProcess(() =>
            {
                using (LazyLock())
                {
                    var logLine = new LogLine(log, isOutput);

                    this.logList.Add(logLine);
                }
            });
        }

        /// <summary>
        /// コンストラクタ
        /// </summary>
        public MainViewModel()
        {
            if (string.IsNullOrEmpty(Name))
            {
                Name = "meijin_" + MathEx.RandInt(0, 1000);
            }

            ThreadNumMaximum = DeviceInventory.CPUCount;
            if (ThreadNum == 0)
            {
                ThreadNum = Math.Max(1, ThreadNumMaximum - 2);
            }

            var rawMemSizeList = Bonanza.MemorySizeList(0.5).ToList();
            MemSizeList = rawMemSizeList.Take(7).ToList();
            if (HashMemSize == 0)
            {
                var index = MathEx.Between(0, 6, rawMemSizeList.Count - 2);
                HashMemSize = rawMemSizeList[index].Item1;
            }

            this.AddDependModel(Global.Settings);
        }

        /// <summary>
        /// ボナンザを設定します。
        /// </summary>
        public void SetBonanza(Bonanza bonanza)
        {
            if (this.bonanza != null)
            {
                this.RemoveDependModel(this.bonanza);
                this.bonanza = null;
            }

            bonanza.CommandSent += (sender, e) =>
                AppendBonanzaLog(e.Command, true);
            bonanza.CommandReceived += (sender, e) =>
                AppendBonanzaLog(e.Command, false);
            bonanza.ErrorReceived += (sender, e) =>
                AppendBonanzaLog(e.Command, null);

            this.bonanza = bonanza;
            this.AddDependModel(bonanza);
        }
    }
}
