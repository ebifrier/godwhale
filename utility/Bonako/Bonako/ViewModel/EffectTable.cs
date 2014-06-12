using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Windows;
using System.Windows.Markup;
using System.Windows.Media;
using System.Windows.Media.Media3D;
using System.Xml;

using Ragnarok.Shogi;
using Ragnarok.Presentation.Extra.Effect;

namespace Bonako.ViewModel
{
    /// <summary>
    /// 読み込み可能なエフェクトのリストです。
    /// </summary>
    public static class EffectTable
    {
        #region Cell
        /// <summary>
        /// 駒の動かせるマスを表示します。
        /// </summary>
        public readonly static EffectInfo MovableCell = new EffectInfo(
            "MovableCellEffect", "Cell");

        /// <summary>
        /// 以前に移動させた駒を表示します。
        /// </summary>
        public readonly static EffectInfo PrevMovedCell = new EffectInfo(
            "PrevMovedCellEffect", "Cell");

        /// <summary>
        /// 手番のある側を表示します。
        /// </summary>
        public readonly static EffectInfo Teban = new EffectInfo(
            "TebanEffect", "Cell");
        #endregion

        #region Piece
        /// <summary>
        /// 駒が動いたときのエフェクトです。
        /// </summary>
        public readonly static EffectInfo PieceMove = new EffectInfo(
            "PieceMoveEffect", "Piece",
            new List<EffectArgument>
            {
                new EffectArgument("Color", typeof(Color), "#ffffffff"),
            });

        /// <summary>
        /// 駒を打ったときのエフェクトです。
        /// </summary>
        public readonly static EffectInfo PieceDrop = new EffectInfo(
            "PieceDropEffect", "Piece");

        /// <summary>
        /// 駒が成ったときのエフェクトです。
        /// </summary>
        public readonly static EffectInfo Promote = new EffectInfo(
            "PromoteEffect", "Piece");

        /// <summary>
        /// 駒を取ったときのエフェクトです。
        /// </summary>
        public readonly static EffectInfo PieceTook = new EffectInfo(
            "PieceTookEffect", "Piece",
            new List<EffectArgument>
            {
                new EffectArgument("StartAngle", typeof(double), "0"),
                new EffectArgument("TargetXY", typeof(Vector), "0,0"),
                new EffectArgument("Color", typeof(Color), "#ffffffff"),
            });
        #endregion

        #region その他
        /// <summary>
        /// 勝利時のエフェクトです。
        /// </summary>
        public readonly static EffectInfo Win = new EffectInfo(
            "WinEffect", "Other");
        #endregion
    }
}
