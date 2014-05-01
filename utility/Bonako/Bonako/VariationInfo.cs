using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

using Ragnarok.Shogi;
using Ragnarok.Shogi.Csa;
using Ragnarok.Utility;

namespace Bonako
{
    /// <summary>
    /// ボナンザから送られてきた変化を保持します。
    /// </summary>
    public class VariationInfo
    {
        /// <summary>
        /// 評価値を取得または設定します。
        /// </summary>
        public double Value
        {
            get;
            set;
        }

        /// <summary>
        /// 指し手のリストを取得または設定します。
        /// </summary>
        public List<CsaMove> MoveList
        {
            get;
            set;
        }

        /// <summary>
        /// PVノード数を取得または設定します。
        /// </summary>
        public long NodeCount
        {
            get;
            set;
        }

        /// <summary>
        /// PVノード数[単位=万]を取得または設定します。
        /// </summary>
        public double ViewNodeCount
        {
            get { return ((double)NodeCount / 10000); }
        }

        /// <summary>
        /// 変化を文字列にして取得します。
        /// </summary>
        public string MoveText
        {
            get { return string.Join(" ", MoveList); }
        }

        /// <summary>
        /// 変化を表示したかどうかを取得または設定します。
        /// </summary>
        public bool IsShowed
        {
            get;
            set;
        }

        /// <summary>
        /// info-6.01 -5142OU +5968OU -7162GI +8822UM -3122GI +7988GI
        /// </summary>
        public static VariationInfo Create(double value, string moveStr, long nodeCount)
        {
            if (string.IsNullOrEmpty(moveStr))
            {
                return null;
            }

            var moveList = moveStr
                .Split(new[] { ' ' }, StringSplitOptions.RemoveEmptyEntries)
                .Where(_ => !string.IsNullOrEmpty(_))
                .Select(_ => CsaMove.Parse(_))
                .Where(_ => _ != null)
                .ToList();
            if (!moveList.Any())
            {
                return null;
            }

            return new VariationInfo()
            {
                Value = value,
                MoveList = moveList,
                NodeCount = nodeCount,
            };
        }
    }
}
