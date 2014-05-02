using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

using Ragnarok;
using Ragnarok.Shogi;
using Ragnarok.Shogi.Csa;
using Ragnarok.Presentation;
using Ragnarok.Presentation.Shogi;

namespace Bonako.ViewModel
{
    /// <summary>
    /// ボナンザから受信したコマンドを解析します。
    /// </summary>
    internal static class BonanzaCommandParser
    {
        #region init
        private static readonly Regex InitRegex = new Regex(
            @"^init\s+(\w+)\s+(\w+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(([\d\w]+\s*)+)?",
            RegexOptions.IgnoreCase);

        private static bool ParseInit(string command)
        {
            var m = InitRegex.Match(command);
            if (!m.Success)
            {
                return false;
            }

            var board = new Board();
            if (m.Groups[7].Success)
            {
                var moveTextList = m.Groups[7].Value.Split(
                    new char[] { ' ' },
                    StringSplitOptions.RemoveEmptyEntries);

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
            }

            var model = Global.ShogiModel;
            model.BlackPlayerName = m.Groups[1].Value;
            model.WhitePlayerName = m.Groups[2].Value;
            model.MyTurn = (m.Groups[3].Value == "0" ? BWType.Black : BWType.White);

            // 残り時間を設定します（切れ負けのみ対応）
            var seconds = int.Parse(m.Groups[4].Value);
            var byoyomi = int.Parse(m.Groups[5].Value);
            model.TotalTime = TimeSpan.FromSeconds(seconds);
            model.ByoyomiTime = TimeSpan.FromSeconds(byoyomi);

            model.BlackBaseLeaveTime = model.TotalTime;
            model.WhiteBaseLeaveTime = model.TotalTime;

            model.InitBoard(board, true, true);
            model.ClearParsedCommand();
            return true;
        }
        #endregion

        #region time info
        private static readonly Regex TimeInfoRegex = new Regex(
            @"^info (bt|wt) ([\d]+)",
            RegexOptions.IgnoreCase);

        private static bool ParseTimeInfo(string command)
        {
            var m = TimeInfoRegex.Match(command);
            if (!m.Success)
            {
                return false;
            }

            var isBlack = (m.Groups[1].Value == "bt");
            var seconds = int.Parse(m.Groups[2].Value);
            var time = TimeSpan.FromSeconds(seconds);

            // 残り時間を設定します（切れ負けのみ対応）
            var model = Global.ShogiModel;
            var remainTime = MathEx.Max(TimeSpan.Zero, model.TotalTime - time);

            if (isBlack)
            {
                model.BlackBaseLeaveTime = remainTime;
            }
            else
            {
                model.WhiteBaseLeaveTime = remainTime;
            }
            
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

            // ＰＣ台数
            var machineCount = int.Parse(m.Groups[1].Value);

            // NPS
            var nps = int.Parse(m.Groups[2].Value);

            // 評価値(先手有利で＋となる値)
            var value = double.Parse(m.Groups[3].Value);

            var model = Global.ShogiModel;
            model.CurrentEvaluationValue =
                value * (model.MyTurn == BWType.Black ? +1 : -1);

            var window = Global.MainWindow;
            if (window != null)
            {
                WPFUtil.UIProcess(() =>
                {
                    window.Title = string.Format(
                        "ボナ子ちゃん　ＰＣ台数:{0}　合計NPS:{1:0.00}[万]",
                        machineCount, nps / 10000.0);
                });
            }

            return true;
        }
        #endregion

        #region move
        private static readonly Regex MovehitRegex = new Regex(
            @"^movehit\s+([\d\w]+)",
            RegexOptions.IgnoreCase);

        private static readonly Regex MoveRegex = new Regex(
            @"^move\s+(\d+)\s+([\d\w]+)\s+(\d+)",
            RegexOptions.IgnoreCase);

        private static readonly Regex MyMoveSecondsRegex = new Regex(
            @"^info mymove (\d+)",
            RegexOptions.IgnoreCase);

        /// <summary>
        /// 先読みに関する指し手は無視するため、局面を戻すことはありません。
        /// </summary>
        private static bool ParseMove(string command)
        {
            if (command.StartsWith("pmove"))
            {
                return true;
            }

            var m = MovehitRegex.Match(command);
            if (m.Success)
            {
                DoCsaMove(m.Groups[1].Value, "0");
                return true;
            }

            m = MoveRegex.Match(command);
            if (m.Success)
            {
                DoCsaMove(m.Groups[2].Value, "0");
                return true;
            }

            m = MyMoveSecondsRegex.Match(command);
            if (m.Success)
            {
                // 自分残り時間を減らします。
                //var seconds = int.Parse(m.Groups[1].Value);
                //Global.ShogiModel.DecMyLeaveTime(seconds);
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
            @"^info\s*((\+|\-)?([\d.]+))([\+\-\d\w* ]+)(\s+n=(\d+))?$",
            RegexOptions.IgnoreCase);

        private static bool ParseVariation(string command)
        {
            var m = VariationRegex.Match(command);
            if (!m.Success)
            {
                return false;
            }

            var nodeCount = m.Groups[6].Success ?
                long.Parse(m.Groups[6].Value) : 0;

            // 後手番の時は評価値を反転します。
            var sign = (Global.ShogiModel.MyTurn == BWType.Black ? +1 : -1);
            var variation = VariationInfo.Create(
                double.Parse(m.Groups[1].Value) * sign * 100,
                m.Groups[4].Value,
                nodeCount);
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
            Global.MainViewModel.Nps = double.Parse(m.Groups[3].Value) * 1000;
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
            if (ParseTimeInfo(command)) return;
            if (ParseInit(command)) return;

            //Log.Error("不明なコマンド: {0}", command);
        }
    }
}
