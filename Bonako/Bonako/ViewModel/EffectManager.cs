using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Media.Media3D;

using Ragnarok;
using Ragnarok.Shogi;
using Ragnarok.ObjectModel;
using Ragnarok.Presentation;
using Ragnarok.Presentation.VisualObject.Control;
using Ragnarok.Presentation.Shogi;
using Ragnarok.Presentation.Shogi.Effects;
using Ragnarok.Presentation.Shogi.View;

namespace Bonako.ViewModel
{
    using View;

    /// <summary>
    /// エフェクトの使用フラグです。
    /// </summary>
    [Flags()]
    public enum EffectFlag
    {
        /// <summary>
        /// すべて無効。
        /// </summary>
        None = 0,

        /// <summary>
        /// 一手前に動かした駒を強調表示します。
        /// </summary>
        PrevCell = (1 << 0),
        /// <summary>
        /// 動かせるマスを強調表示します。
        /// </summary>
        MovableCell = (1 << 1),
        /// <summary>
        /// 手番側を強調表示します。
        /// </summary>
        Teban = (1 << 2),

        /// <summary>
        /// 背景エフェクトを使用します。
        /// </summary>
        Background = (1 << 8),
        /// <summary>
        /// 駒に関するエフェクトを使用します。
        /// </summary>
        Piece = (1 << 9),

        /// <summary>
        /// 全フラグ
        /// </summary>
        All = (PrevCell | MovableCell | Teban | Background | Piece),
    }

    /// <summary>
    /// エフェクトの管理を行います。
    /// </summary>
    public class EffectManager : NotifyObject, IEffectManager
    {
        private EffectObject prevMovedCell;
        private EffectObject movableCell;
        private EffectObject tebanCell;
        private int moveCount;
        private string prevBackgroundKey;

        /// <summary>
        /// コンストラクタ
        /// </summary>
        public EffectManager()
        {
            EffectEnabled = true;
            IsUseEffectSound = true;
            EffectVolume = 50;
            EffectFlag = EffectFlag.All;
        }

        /// <summary>
        /// エフェクトを表示するオブジェクトを取得または設定します。
        /// </summary>
        public ShogiControl Container
        {
            get;
            set;
        }

        /// <summary>
        /// 背景エフェクトを表示するコントロールを取得または設定します。
        /// </summary>
        public VisualBackground Background
        {
            get;
            set;
        }

        /// <summary>
        /// マスの横幅を取得します。
        /// </summary>
        private double CellWidth
        {
            get { return Container.CellSize.Width; }
        }

        /// <summary>
        /// エフェクトを有効にするかどうかを取得または設定します。
        /// </summary>
        public bool EffectEnabled
        {
            get { return GetValue<bool>("EffectEnabled"); }
            set { SetValue("EffectEnabled", value); }
        }

        /// <summary>
        /// エフェクトフラグを取得または設定します。
        /// </summary>
        public EffectFlag EffectFlag
        {
            get { return GetValue<EffectFlag>("EffectFlag"); }
            set { SetValue("EffectFlag", value); }
        }

        /// <summary>
        /// エフェクトＳＥを使用するかどうかを取得または設定します。
        /// </summary>
        public bool IsUseEffectSound
        {
            get { return GetValue<bool>("IsUseEffectSound"); }
            set { SetValue("IsUseEffectSound", value); }
        }

        /// <summary>
        /// エフェクト音量を取得または設定します。
        /// </summary>
        public int EffectVolume
        {
            get { return GetValue<int>("EffectVolume"); }
            set { SetValue("EffectVolume", value); }
        }

        /// <summary>
        /// エフェクト用の指し手カウントを取得または設定します。
        /// </summary>
        public int EffectMoveCount
        {
            get { return GetValue<int>("EffectMoveCount"); }
            set { SetValue("EffectMoveCount", value); }
        }

        /// <summary>
        /// エフェクトの個別の使用フラグを取得します。
        /// </summary>
        private bool HasEffectFlag(EffectFlag flag)
        {
            if (!EffectEnabled)
            {
                return false;
            }

            return ((EffectFlag & flag) != 0);
        }

        #region データコンテキスト
        /// <summary>
        /// 通常のデータコンテキストを作成します。
        /// </summary>
        private EffectContext CreateContext(
            Position position,
            double z = ShogiControl.EffectZ)
        {
            var p = Container.GetPiecePos(position);
            var s = Container.CellSize;

            return new EffectContext()
            {
                Coord = new Vector3D(p.X, p.Y, z),
                BaseScale = new Vector3D(s.Width, s.Height, 1.0),
            };
        }

        /// <summary>
        /// セルエフェクトのコンテキストを作成します。
        /// </summary>
        private CellEffectContext CreateCellContext(
            Position position,
            Position cellPosition = null,
            double z = ShogiControl.BanEffectZ)
        {
            return CreateCellContext(new[] { position }, cellPosition, z);
        }

        /// <summary>
        /// セルエフェクトのコンテキストを作成します。
        /// </summary>
        private CellEffectContext CreateCellContext(
            IEnumerable<Position> positions,
            Position cellPosition = null,
            double z = ShogiControl.BanEffectZ)
        {
            var bp = Container.BanBounds.TopLeft;
            var bs = Container.BanBounds.Size;
            var s = Container.CellSize;

            var flipPosition =
                (cellPosition == null || Container.ViewSide == BWType.Black
                ? cellPosition
                : cellPosition.Flip());
            var flipPositions =
                (positions == null || Container.ViewSide == BWType.Black
                ? positions
                : positions.Where(_ => _ != null).Select(_ => _.Flip()));

            return new CellEffectContext()
            {
                CellPosition = flipPosition,
                CellPositions = flipPositions.ToArray(),
                BanCoord = new Vector3D(bp.X, bp.Y, z),
                BanScale = new Vector3D(bs.Width, bs.Height, 1.0),
                BaseScale = new Vector3D(s.Width, s.Height, 1.0),
            };
        }
        #endregion

        #region マスエフェクト関係
        /// <summary>
        /// 前回動かした駒の位置を設定します。
        /// </summary>
        private void UpdatePrevMovedCell()
        {
            if (this.prevMovedCell != null)
            {
                Container.RemoveBanEffect(this.prevMovedCell);
                this.prevMovedCell = null;
            }

            if (!HasEffectFlag(EffectFlag.PrevCell))
            {
                return;
            }

            var board = (Container != null ? Container.Board : null);
            if (board != null && board.LastMove != null)
            {
                var position = board.LastMove.NewPosition;

                var cell = EffectTable.PrevMovedCell.LoadEffect();
                if (cell != null)
                {
                    cell.DataContext = CreateCellContext(position);

                    Container.AddBanEffect(cell);
                    this.prevMovedCell = cell;
                }
            }
        }

        /// <summary>
        /// 駒を動かせる位置を光らせます。
        /// </summary>
        private void UpdateMovableCell(Position position, BoardPiece piece)
        {
            if (this.movableCell != null)
            {
                Container.RemoveBanEffect(this.movableCell);
                this.movableCell = null;
            }

            if (!HasEffectFlag(EffectFlag.MovableCell))
            {
                return;
            }

            var board = Container.Board;
            if (board == null || piece == null)
            {
                return;
            }

            // 移動可能もしくは駒打ち可能な全マスを取得します。
            var isMove = (position != null);
            var movePositions =
                from file in Enumerable.Range(1, Board.BoardSize)
                from rank in Enumerable.Range(1, Board.BoardSize)
                let move = new BoardMove()
                {
                    OldPosition = position,
                    NewPosition = new Position(file, rank),
                    BWType = piece.BWType,
                    ActionType = (isMove ? ActionType.None : ActionType.Drop),
                    DropPieceType = (isMove ? PieceType.None : piece.PieceType),
                }
                where board.CanMove(move)
                select move.NewPosition;

            // 移動可能なマスにエフェクトをかけます。
            var movableCell = EffectTable.MovableCell.LoadEffect();
            if (movableCell != null)
            {
                movableCell.DataContext = CreateCellContext(movePositions, position);

                Container.AddBanEffect(movableCell);
                this.movableCell = movableCell;
            }
        }

        /// <summary>
        /// 手番の表示を行います。
        /// </summary>
        private void UpdateTeban(BWType teban)
        {
            if (this.tebanCell != null)
            {
                Container.RemoveBanEffect(this.tebanCell);
                this.tebanCell = null;
            }

            if (!HasEffectFlag(EffectFlag.Teban))
            {
                return;
            }

            // 手番なしの時はオブジェクトを消去して帰ります。
            if (teban == BWType.None)
            {
                return;
            }

            // 移動可能なマスにエフェクトをかけます。
            var tebanCell = EffectTable.Teban.LoadEffect();
            if (tebanCell != null)
            {
                if (Container.ViewSide != BWType.Black)
                {
                    teban = teban.Toggle();
                }

                tebanCell.DataContext = CreateCellContext((Position)null)
                    .Apply(_ => _.BWType = teban);

                Container.AddBanEffect(tebanCell);
                this.tebanCell = tebanCell;
            }
        }
        #endregion

        #region 背景エフェクト
        /// <summary>
        /// 背景の設定を行います。(キーが変わっている場合のみ設定します)
        /// </summary>
        private void TrySetBackgroundKey(string key)
        {
            if (Background == null)
            {
                return;
            }

            /*if (this.prevBackgroundKey == key)
            {
                return;
            }*/

            // 背景エフェクトの作成。
            var effectInfo = new EffectInfo(key, null);
            var effect = effectInfo.LoadBackground();

            Background.AddEntity(effect);
            this.prevBackgroundKey = key;
        }

        /// <summary>
        /// 背景エフェクトを更新します。
        /// </summary>
        public void UpdateBackground()
        {
            if (Container == null)
            {
                return;
            }

            WPFUtil.UIProcess(() =>
            {
                // 必要なら背景エフェクトを無効にします。
                if (!HasEffectFlag(EffectFlag.Background))
                {
                    TrySetBackgroundKey(null);
                    return;
                }

                var Unit = 30;
                if (this.moveCount >= Unit * 3)
                {
                    TrySetBackgroundKey("WinterEffect");
                }
                else if (this.moveCount >= Unit * 2)
                {
                    TrySetBackgroundKey("AutumnEffect");
                }
                else if (this.moveCount >= Unit)
                {
                    TrySetBackgroundKey("SummerEffect");
                }
                else
                {
                    TrySetBackgroundKey("SpringEffect");
                }
            });
        }

        /// <summary>
        /// 現局面の差し手が進んだときに呼ばれます。
        /// </summary>
        public void ChangeMoveCount(int moveCount)
        {
            this.moveCount = moveCount;

            UpdateBackground();
        }
        #endregion

        #region 駒のエフェクト関係
        /// <summary>
        /// エフェクトを追加します。
        /// </summary>
        private void AddEffect(EffectObject effect, Position position)
        {
            if (effect == null)
            {
                return;
            }

            // 効果音を調整します。
            if (!IsUseEffectSound)
            {
                effect.StartSoundPath = null;
                effect.StartSoundVolume = 0.0;
            }
            else
            {
                var percent = EffectVolume;

                effect.StartSoundVolume *= MathEx.Between(0, 100, percent) / 100.0;
            }

            WPFUtil.UIProcess(() =>
            {
                effect.DataContext = CreateContext(position);

                Container.AddEffect(effect);
            });
        }

        /// <summary>
        /// エフェクトを追加します。
        /// </summary>
        private void AddEffect(EffectInfo effectInfo, Position position)
        {
            var effect = effectInfo.LoadEffect();

            AddEffect(effect, position);
        }

        /// <summary>
        /// 手番によるパーティクルの色を取得します。
        /// </summary>
        private static Color TebanParticleColor(BWType bwType)
        {
            return (bwType == BWType.Black ? Colors.Red : Colors.Blue);
        }

        /// <summary>
        /// 駒を動かしたときのエフェクトです。
        /// </summary>
        private void AddMoveEffect(Position position, BoardMove move)
        {
            var table = new Dictionary<string, object>
            {
                { "Color",  TebanParticleColor(move.BWType) },
            };

            var effect = EffectTable.PieceMove.LoadEffect(table);
            AddEffect(effect, position);
        }

        /// <summary>
        /// 駒取りエフェクトです。
        /// </summary>
        private void AddTookEffect(Position position, BoardPiece tookPiece)
        {
            var bwType = tookPiece.BWType.Toggle();
            var bp = Container.GetPiecePos(position);
            var ep = Container.GetCapturedPiecePos(bwType, tookPiece.PieceType);
            var d = Vector3D.Subtract(ep, bp);
            var rad = Math.Atan2(d.Y, d.X) + Math.PI;

            var table = new Dictionary<string, object>
            {
                { "TargetXY",  new Vector(d.X, d.Y) },
                { "StartAngle", MathEx.ToDeg(rad) },
                { "Color",  TebanParticleColor(bwType) },
            };

            var effect = EffectTable.PieceTook.LoadEffect(table);
            AddEffect(effect, position);
        }
        #endregion

        #region オーバーライド
        /// <summary>
        /// 初期化時・オブジェクトの破棄時などに呼ばれます。
        /// </summary>
        public void Clear()
        {
            if (Container == null)
            {
                return;
            }

            UpdateTeban(BWType.None);
            UpdateMovableCell(null, null);
            UpdatePrevMovedCell();
            UpdateBackground();
        }

        /// <summary>
        /// 局面更新時に呼ばれます。
        /// </summary>
        public void InitEffect(BWType bwType)
        {
            if (Container == null)
            {
                return;
            }

            UpdateTeban(bwType);
            UpdatePrevMovedCell();
            ((IEffectManager)this).EndMove();
        }

        /// <summary>
        /// 駒の移動を開始したときに呼ばれます。
        /// </summary>
        void IEffectManager.BeginMove(Position position, BoardPiece piece)
        {
            if (Container == null)
            {
                return;
            }

            UpdateMovableCell(position, piece);
        }

        /// <summary>
        /// 駒の移動が終わったときに呼ばれます。
        /// </summary>
        void IEffectManager.EndMove()
        {
            if (Container == null)
            {
                return;
            }

            UpdateMovableCell(null, null);
        }

        /// <summary>
        /// 玉の位置を取得します。
        /// </summary>
        private Position FindGyoku(Board board, BWType bwType)
        {
            var positions =
                from file in Enumerable.Range(1, Board.BoardSize)
                from rank in Enumerable.Range(1, Board.BoardSize)
                let piece = board[file, rank]
                where piece != null &&
                      piece.PieceType == PieceType.Gyoku &&
                      piece.BWType == bwType
                select new Position(file, rank);
            if (positions.Count() != 1)
            {
                return null;
            }

            return positions.FirstOrDefault();
        }

        /// <summary>
        /// 投了します。
        /// </summary>
        public void Resign()
        {
            if (Container == null)
            {
                return;
            }

            var board = Container.Board;
            if (board == null)
            {
                return;
            }

            // 投了時は玉の位置にエフェクトをかけます。
            var position = FindGyoku(board, board.MovePriority);
            if (position == null)
            {
                return;
            }

            AddEffect(EffectTable.Win, position);
        }

        /// <summary>
        /// エフェクトを追加します。
        /// </summary>
        void IEffectManager.Moved(BoardMove move, bool isUndo)
        {
            if (Container == null)
            {
                return;
            }

            // 前回動かした駒を強調表示します。
            // これはエフェクト機能がオフの時でも表示します。
            UpdatePrevMovedCell();

            if (!EffectEnabled)
            {
                return;
            }

            // アンドゥ時
            if (isUndo)
            {
                if (move.OldPosition != null &&
                    HasEffectFlag(EffectFlag.Piece))
                {
                    AddMoveEffect(move.OldPosition, move);
                }

                UpdateTeban(move.BWType);
                return;
            }

            UpdateTeban(move.BWType.Toggle());

            if (HasEffectFlag(EffectFlag.Piece))
            {
                if (move.TookPiece != null)
                {
                    AddTookEffect(move.NewPosition, move.TookPiece);
                }

                if (move.ActionType == ActionType.Drop)
                {
                    AddEffect(EffectTable.PieceDrop, move.NewPosition);
                }
                else if (move.ActionType == ActionType.Promote)
                {
                    AddEffect(EffectTable.Promote, move.NewPosition);
                }

                AddMoveEffect(move.NewPosition, move);
            }
        }
        #endregion
    }
}
