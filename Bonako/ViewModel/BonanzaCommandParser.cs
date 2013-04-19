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
        public static BoardMove ConvertCsaMove(this Board board, CsaMove csaMove)
        {
            if (csaMove == null)
            {
                return null;
            }

            var newPiece = board[csaMove.NewPosition];
            if (csaMove.IsDrop)
            {
                if (newPiece != null)
                {
                    return null;
                }

                return new BoardMove
                {
                    BWType = board.MovePriority,
                    NewPosition = csaMove.NewPosition,
                    ActionType = ActionType.Drop,
                    DropPieceType = csaMove.Piece.PieceType,
                };
            }
            else
            {
                var oldPiece = board[csaMove.OldPosition];
                if (oldPiece == null)
                {
                    return null;
                }

                return new BoardMove
                {
                    BWType = board.MovePriority,
                    NewPosition = csaMove.NewPosition,
                    OldPosition = csaMove.OldPosition,
                    TookPiece = newPiece,
                    ActionType = (
                        !oldPiece.IsPromoted && csaMove.Piece.IsPromoted ?
                        ActionType.Promote :
                        ActionType.None),
                };
            }
        }

        public static CsaMove ConvertBoardMove(this Board board, BoardMove bmove)
        {
            if (bmove == null || !bmove.Validate())
            {
                return null;
            }

            var newPiece = board[bmove.NewPosition];
            if (bmove.ActionType == ActionType.Drop)
            {
                if (newPiece != null)
                {
                    return null;
                }

                return new CsaMove
                {
                    Side = board.MovePriority,
                    NewPosition = bmove.NewPosition,
                    Piece = new Piece(bmove.DropPieceType, false),
                };
            }
            else
            {
                var oldPiece = board[bmove.OldPosition];
                if (oldPiece == null)
                {
                    return null;
                }

                return new CsaMove
                {
                    Side = board.MovePriority,
                    NewPosition = bmove.NewPosition,
                    OldPosition = bmove.OldPosition,
                    Piece = new Piece(
                        oldPiece.PieceType,
                        oldPiece.IsPromoted ||
                        bmove.ActionType == ActionType.Promote),
                };
            }
        }

        #region new
        private static bool ParseNew(string command)
        {
            if (!command.StartsWith("new"))
            {
                return false;
            }

            Global.ShogiModel.InitBoard(new Board());
            Global.ShogiModel.ClearVariationList();
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

                var bmove = ConvertCsaMove(board, csaMove);
                if (bmove == null || !bmove.Validate())
                {
                    break;
                }

                board.DoMove(bmove);
            }

            Global.ShogiModel.InitBoard(board);
            Global.ShogiModel.ClearVariationList();
            return true;
        }
        #endregion

        #region move, alter, retract
        private static readonly Regex BonaMoveRegex = new Regex(
            @"^move([\d\w]+)\s*(\d+)",
            RegexOptions.IgnoreCase);

        private static readonly Regex MoveRegex = new Regex(
            @"^(p?move)\s+([\d\w]+)\s*(\d+)",
            RegexOptions.IgnoreCase);

        private static readonly Regex AlterRegex = new Regex(
            @"^alter\s+([\d\w]+)\s*(\d+)",
            RegexOptions.IgnoreCase);

        private static readonly Regex RetractRegex = new Regex(
            @"^retract\s+(\d+)\s*([\d\w]+)\s*(\d+)",
            RegexOptions.IgnoreCase);

        private static bool ParseMove(string command)
        {
            if (command.StartsWith("movehit") ||
                command.StartsWith("ponderhit"))
            {
                //Global.ShogiModel.PonderHit();
                return true;
            }

            /*var m = MoveRegex.Match(command);
            if (m.Success)
            {
                var ponder = (m.Groups[1].Value == "pmove");

                DoCsaMove(m.Groups[2].Value, 0, ponder);
                return true;
            }

            m = AlterRegex.Match(command);
            if (m.Success)
            {
                DoCsaMove(m.Groups[1].Value, 1, false);
                return true;
            }

            m = RetractRegex.Match(command);
            if (m.Success)
            {
                var nback = int.Parse(m.Groups[1].Value);

                DoCsaMove(m.Groups[2].Value, nback, false);
                return true;
            }*/

            return false;
        }

        /// <summary>
        /// 局面を<paramref name="nback"/>手元に戻し、さらに
        /// <paramref name="moveText"/>で示される手を指します。
        /// </summary>
        private static bool DoCsaMove(string moveText, int nback, bool ponder)
        {
            var board = Global.ShogiModel.CurrentBoard;

            var csaMove = CsaMove.Parse(moveText);
            if (csaMove == null)
            {
                return false;
            }

            while (nback-- > 0 && board.CanUndo)
            {
                board.Undo();
            }

            var bmove = ConvertCsaMove(board, csaMove);
            if (bmove == null || !bmove.Validate())
            {
                return false;
            }

            board.DoMove(bmove);
            Global.ShogiModel.InitBoard(board);
            return true;
        }
        #endregion

        #region variation
        private static readonly Regex VariationRegex = new Regex(
            @"^info((\+|\-)([\d.]+))",
            RegexOptions.IgnoreCase);

        private static bool ParseVariation(string command)
        {
            var m = VariationRegex.Match(command);
            if (!m.Success)
            {
                return false;
            }

            var variation = VariationInfo.Create(
                double.Parse(m.Groups[1].Value),
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
            //Global.ShogiModel.ClearVariationList();
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
            if (ParseStats(command)) return;
            if (ParseNew(command)) return;
            if (ParseInit(command)) return;
        }
    }
}
