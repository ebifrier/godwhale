using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

using Ragnarok;
using Ragnarok.Shogi;
using Ragnarok.Shogi.Csa;
using Ragnarok.Presentation.Shogi;

namespace Bonako.ViewModel
{
    /// <summary>
    /// ボナンザから受信したコマンドを解析します。
    /// </summary>
    internal static class BonanzaCommandParser
    {
        #region new
        private static bool ParseNew(string command)
        {
            if (!command.StartsWith("new"))
            {
                return false;
            }

            Global.ShogiModel.InitBoard(new Board(), true, true);
            Global.ShogiModel.ClearParsedCommand();
            return true;
        }
        #endregion

        #region init
        private static readonly Regex InitRegex = new Regex(
            @"^init\s+(\d+)\s*(([\d\w]+\s*)+)",
            RegexOptions.IgnoreCase);

        private static bool ParseInit(string command)
        {
            var m = InitRegex.Match(command);
            if (!m.Success)
            {
                return false;
            }

            var moveTextList = m.Groups[2].Value.Split(
                new char[] { ' ' },
                StringSplitOptions.RemoveEmptyEntries);
            var board = new Board();

            // 初期局面から渡された手を進めます。
            foreach (var str in moveTextList)
            {
                var csaMove = CsaMove.Parse(str);
                if (csaMove == null)
                {
                    break;
                }

                var bmove = board.ConvertCsaMove(csaMove);
                if (bmove == null || !bmove.Validate())
                {
                    break;
                }

                board.DoMove(bmove);
            }

            Global.ShogiModel.InitBoard(board, true, true);
            Global.ShogiModel.ClearParsedCommand();
            return true;
        }
        #endregion

        #region game info
        private static readonly Regex GameInfoRegex = new Regex(
            @"^info gameinfo ([\+\-]) ([\w]+) ([\w]+) ([\d]+) ([\d]+)",
            RegexOptions.IgnoreCase);

        private static bool ParseGameInfo(string command)
        {
            var m = GameInfoRegex.Match(command);
            if (!m.Success)
            {
                return false;
            }

            var model = Global.ShogiModel;
            model.MyTurn = (m.Groups[1].Value == "+" ? BWType.Black : BWType.White);
            model.BlackPlayerName = m.Groups[2].Value;
            model.WhitePlayerName = m.Groups[3].Value;

            // 残り時間を設定します（切れ負けのみ対応）
            var seconds = int.Parse(m.Groups[4].Value);
            var time = TimeSpan.FromSeconds(seconds);
            model.BlackBaseLeaveTime = time;
            model.WhiteBaseLeaveTime = time;
            return true;
        }
        #endregion

        #region current info
        private static readonly Regex CurrentInfoRegex = new Regex(
            @"^info current (\d+) (\d+) ([\+\-]?[\d.]+)",
            RegexOptions.IgnoreCase);

        private static bool ParseCurrentInfo(string command)
        {
            var m = CurrentInfoRegex.Match(command);
            if (!m.Success)
            {
                return false;
            }

            var machineCount = int.Parse(m.Groups[1].Value);
            var nodes = int.Parse(m.Groups[2].Value);

            var model = Global.ShogiModel;
            model.CurrentEvaluationValue = double.Parse(m.Groups[3].Value);

            var window = Global.MainWindow;
            if (window != null)
            {
                Ragnarok.Presentation.WPFUtil.UIProcess(() =>
                {
                    var thinkTime =
                        model.CurrentTurn == BWType.Black ?
                        model.BlackBaseLeaveTime - model.BlackLeaveTime :
                        model.WhiteBaseLeaveTime - model.WhiteLeaveTime;
                    var thinkSeconds = Math.Max(1.0, thinkTime.TotalSeconds);

                    window.Title = string.Format(
                        "ボナ子ちゃん　ＰＣ台数:{0}　合計NPS:{1:0.00}[万]",
                        machineCount, nodes / thinkSeconds / 10000.0);
                });
            }

            return true;
        }
        #endregion

        #region move, alter, retract
        private static readonly Regex MovehitRegex = new Regex(
            @"^(ponderhit|movehit)\s+([\+\-\d\w]+)\s+(\d+)\s+(\d+)",
            RegexOptions.IgnoreCase);

        private static readonly Regex MoveRegex = new Regex(
            @"^move\s+([\d\w]+)\s+(\d+)\s+(\d+)",
            RegexOptions.IgnoreCase);

        private static readonly Regex AlterRegex = new Regex(
            @"^alter\s+([\d\w]+)\s+(\d+)\s+(\d+)",
            RegexOptions.IgnoreCase);

        private static readonly Regex RetractRegex = new Regex(
            @"^retract\s+(\d+)\s+([\d\w]+)\s+(\d+)\s+(\d+)",
            RegexOptions.IgnoreCase);

        private static readonly Regex MyMoveSecondsRegex = new Regex(
            @"^info mymove (\d+)",
            RegexOptions.IgnoreCase);

        /// <summary>
        /// 先読みに関する指し手は無視するため、局面を戻すことはありません。
        /// </summary>
        private static bool ParseMove(string command)
        {
            var m = MovehitRegex.Match(command);
            if (m.Success)
            {
                DoCsaMove(m.Groups[2].Value, m.Groups[4].Value);
                return true;
            }

            m = MoveRegex.Match(command);
            if (m.Success)
            {
                DoCsaMove(m.Groups[1].Value, m.Groups[3].Value);
                return true;
            }

            m = AlterRegex.Match(command);
            if (m.Success)
            {
                DoCsaMove(m.Groups[1].Value, m.Groups[3].Value);
                return true;
            }

            m = RetractRegex.Match(command);
            if (m.Success)
            {
                DoCsaMove(m.Groups[2].Value, m.Groups[4].Value);
                return true;
            }

            m = MyMoveSecondsRegex.Match(command);
            if (m.Success)
            {
                // 自分残り時間を減らします。
                var seconds = int.Parse(m.Groups[1].Value);
                Global.ShogiModel.DecMyLeaveTime(seconds);
                return true;
            }

            return false;
        }

        /// <summary>
        /// <paramref name="moveText"/>で示される手を指します。
        /// </summary>
        private static void DoCsaMove(string moveText, string secondsText)
        {
            var csaMove = CsaMove.Parse(moveText);
            if (csaMove == null)
            {
                Log.Error("{0}: 指し手ではありません。", moveText);
                return;
            }

            var seconds = int.Parse(secondsText);
            Global.ShogiModel.DoMove(csaMove, seconds);
        }
        #endregion

        #region variation
        private static readonly Regex VariationRegex = new Regex(
#if PUBLISHED
            @"^info\s*((\+|\-)?([\d.]+))",
#else
            @"^info\s+((\+|\-)?([\d.]+))",
#endif
            RegexOptions.IgnoreCase);

        private static bool ParseVariation(string command)
        {
            var m = VariationRegex.Match(command);
            if (!m.Success)
            {
                return false;
            }

            var variation = VariationInfo.Create(
                double.Parse(m.Groups[1].Value) * 100,
                command.Substring(m.Length));
            if (variation == null)
            {
                return false;
            }

            Global.ShogiModel.AddVariation(variation);
            return true;
        }
        #endregion

        #region stats
        private static readonly Regex StatsRegex = new Regex(
            @"^statsatu=([\d.]+)\s+cpu=([\d.]+)\s+nps=([\d.]+)",
            RegexOptions.IgnoreCase);

        private static bool ParseStats(string command)
        {
            var m = StatsRegex.Match(command);
            if (!m.Success)
            {
                return false;
            }

            Global.MainViewModel.CpuUsage = double.Parse(m.Groups[2].Value);
            Global.MainViewModel.Nps = double.Parse(m.Groups[3].Value);
            return true;
        }
        #endregion

        /// <summary>
        /// ボナンザから送られてきたコマンドを解析します。
        /// </summary>
        public static void Parse(string command)
        {
            if (string.IsNullOrEmpty(command))
            {
                return;
            }

            if (Global.MainViewModel == null ||
                Global.ShogiModel == null)
            {
                return;
            }

            if (ParseMove(command)) return;
            if (ParseVariation(command)) return;
            if (ParseCurrentInfo(command)) return;
            if (ParseStats(command)) return;
            if (ParseNew(command)) return;
            if (ParseInit(command)) return;
            if (ParseGameInfo(command)) return;

            //Log.Error("不明なコマンド: {0}", command);
        }
    }
}
