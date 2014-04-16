using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

using Ragnarok;
using Ragnarok.ObjectModel;

namespace Bonako
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
        /// DFPN用ボナンザのログかどうかを取得します。
        /// </summary>
        public bool IsDfpn
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
        public LogLine(string text, bool isDfpn, bool? isOutput)
        {
            if (string.IsNullOrEmpty(text))
            {
                throw new ArgumentNullException("text");
            }

            RawText = text.Trim();
            IsDfpn = isDfpn;
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
        private readonly NotifyCollection<LogLine> dfpnLogList =
            new NotifyCollection<LogLine>();
        private Bonanza bonanza;
        private Bonanza dfpnBonanza;

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
        /// 並列化サーバーのポートを取得または設定します。
        /// </summary>
        public string ServerPort
        {
            get { return GetValue<string>("ServerPort"); }
            set { SetValue("ServerPort", value); }
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
        /// ボナンザの初期化が終わったかどうかを取得します。
        /// </summary>
        [DependOnProperty(typeof(Bonanza), "IsMnjInited")]
        public bool? IsMnjInited
        {
            get
            {
                if (this.bonanza == null)
                {
                    return false;
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
        /// DFPNサーバーに接続しているかどうかを取得します。
        /// </summary>
        [DependOnProperty(typeof(Bonanza), "IsConnected")]
        public bool IsConnectedToDfpn
        {
            get
            {
                if (this.dfpnBonanza == null)
                {
                    return false;
                }

                return this.dfpnBonanza.IsConnected;
            }
        }

        /// <summary>
        /// 読み筋リストを取得または設定します。
        /// </summary>
        public NotifyCollection<VariationInfo> VariationList
        {
            get;
            private set;
        }

        /// <summary>
        /// 今までの指し手を取得します。
        /// </summary>
        public NotifyCollection<CsaMove> MoveList
        {
            get;
            private set;
        }

        /// <summary>
        /// 相手の予想手を取得します。
        /// </summary>
        public CsaMove PonderMove
        {
            get { return GetValue<CsaMove>("PonderMove"); }
            private set { SetValue("PonderMove", value); }
        }

        /// <summary>
        /// CPU使用率を取得します。
        /// </summary>
        public double CpuUsage
        {
            get { return GetValue<double>("CpuUsage"); }
            private set { SetValue("CpuUsage", value); }
        }

        /// <summary>
        /// NPS値[k]を取得します。
        /// </summary>
        public double Nps
        {
            get { return GetValue<double>("Nps"); }
            private set { SetValue("Nps", value); }
        }

        /// <summary>
        /// ボナンザのログ内容を取得または設定します。
        /// </summary>
        public NotifyCollection<LogLine> LogList
        {
            get { return this.logList; }
        }

        /// <summary>
        /// DFPNのログ内容を取得または設定します。
        /// </summary>
        public NotifyCollection<LogLine> DfpnLogList
        {
            get { return this.dfpnLogList; }
        }

        /// <summary>
        /// ボナンザログに出力します。
        /// </summary>
        public void AppendBonanzaLog(string log, bool isDfpn, bool? isOutput)
        {
            if (string.IsNullOrEmpty(log))
            {
                return;
            }

            using (LazyLock())
            {
                var logLine = new LogLine(log, isDfpn, isOutput);

                if (isDfpn)
                {
                    this.dfpnLogList.Add(logLine);
                }
                else
                {
                    this.logList.Add(logLine);
                }
            }
        }

        /// <summary>
        /// ボナンザの使用メモリが物理メモリを越えないような範囲で
        /// 使用量の増減をさせます。
        /// </summary>
        private IEnumerable<Tuple<int, int>> GetMemorySizeList()
        {
            var hashSize = 19;
            var mem = Bonanza.HashSizeMinimum;
            var memMax = (long)Global.GetAvailPhys() / 1024 / 1024 * 2 / 4;

            do
            {
                yield return Tuple.Create(hashSize, mem + Bonanza.MemUsedBase);

                mem *= 2;
                hashSize += 1;
            } while (mem + Bonanza.MemUsedBase <= memMax);
        }

        /// <summary>
        /// コンストラクタ
        /// </summary>
        public MainViewModel()
        {
            VariationList = new NotifyCollection<VariationInfo>();
            MoveList = new NotifyCollection<CsaMove>();

            if (string.IsNullOrEmpty(Name))
            {
                Name = "meijin_" + MathEx.RandInt(0, 1000);
            }
            ServerPort = "4084";

            ThreadNumMaximum = Global.GetCpuThreadNum();
            if (ThreadNum == 0)
            {
                ThreadNum = Math.Max(1, ThreadNumMaximum - 2);
            }

            var rawMemSizeList = GetMemorySizeList().ToList();
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
                AppendBonanzaLog(e.Command, false, true);
            bonanza.CommandReceived += (sender, e) =>
                AppendBonanzaLog(e.Command, false, false);
            bonanza.ErrorReceived += (sender, e) =>
                AppendBonanzaLog(e.Command, false, null);
            bonanza.CommandReceived += bonanza_ReceivedCommand;

            this.bonanza = bonanza;
            this.AddDependModel(bonanza);
        }

        /// <summary>
        /// DFPN用のボナンザを設定します。
        /// </summary>
        public void SetDfpnBonanza(Bonanza bonanza)
        {
            if (this.dfpnBonanza != null)
            {
                this.RemoveDependModel(this.dfpnBonanza);
                this.dfpnBonanza = null;
            }

            bonanza.CommandSent += (sender, e) =>
                AppendBonanzaLog(e.Command, true, true);
            bonanza.CommandReceived += (sender, e) =>
                AppendBonanzaLog(e.Command, true, false);
            bonanza.ErrorReceived += (sender, e) =>
                AppendBonanzaLog(e.Command, true, null);

            this.dfpnBonanza = bonanza;
            this.AddDependModel(bonanza);
        }

        #region ボナンザの変化などを解析します。
        private static readonly Regex MoveRegex = new Regex(
            @"^move\s*([\d\w]+)\s*(\d+)\s*(\d*)",
            RegexOptions.IgnoreCase);

        private static readonly Regex VariationRegex = new Regex(
            @"^info((\+|\-)([\d.]+))",
            RegexOptions.IgnoreCase);

        private static readonly Regex CpuRegex = new Regex(
            @"^statsatu=([\d.]+)\s+cpu=([\d.]+)\s+nps=([\d.]+)",
            RegexOptions.IgnoreCase);

        void bonanza_ReceivedCommand(object sender, BonanzaReceivedCommandEventArgs e)
        {
            var m = MoveRegex.Match(e.Command);
            if (m.Success)
            {
                var move = CsaMove.Parse(m.Groups[1].Value);
                if (move == null)
                {
                    return;
                }

                var nback = int.Parse(m.Groups[2].Value);
                while (nback > 0 && MoveList.Any())
                {
                    MoveList.RemoveAt(MoveList.Count - 1);
                }

                MoveList.Add(move);

                // 奇数手目は先手側の手です。
                PonderMove = ((MoveList.Count & 1) == 1 ? move : null);
            }

            m = VariationRegex.Match(e.Command);
            if (m.Success)
            {
                var variation = VariationInfo.Create(
                    double.Parse(m.Groups[1].Value),
                    e.Command.Substring(m.Length));
                if (variation == null)
                {
                    return;
                }

                if (VariationList.Count > 5)
                {
                    VariationList.RemoveAt(0);
                }
                VariationList.Add(variation);
            }

            m = CpuRegex.Match(e.Command);
            if (m.Success)
            {
                CpuUsage = double.Parse(m.Groups[2].Value);
                Nps = double.Parse(m.Groups[3].Value);

                VariationList.Clear();
            }
        }
        #endregion
    }
}
