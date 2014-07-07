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
using Ragnarok.Presentation.Extra.Effect;
using Ragnarok.Presentation.Extra.Element;
using Ragnarok.Presentation.Shogi;
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
        public ShogiUIElement3D Container
        {
            get;
            set;
        }

        /// <summary>
        /// 背景エフェクトを表示するコントロールを取得または設定します。
        /// </summary>
        public BackgroundUIElement3D Background
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
            Square square,
            double z = ShogiUIElement3D.EffectZ)
        {
            var p = Container.SquareToPoint(square);
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
            Square square,
            Square cellSquare = null,
            double z = ShogiUIElement3D.BanEffectZ)
        {
            return CreateCellContext(new[] { square }, cellSquare, z);
        }

        /// <summary>
        /// セルエフェクトのコンテキストを作成します。
        /// </summary>
        private CellEffectContext CreateCellContext(
            IEnumerable<Square> squares,
            Square cellSquare = null,
            double z = ShogiUIElement3D.BanEffectZ)
        {
            var bp = Container.BanBounds.TopLeft;
            var bs = Container.BanBounds.Size;
            var s = Container.CellSize;

            var flipSquare =
                (cellSquare == null || Container.ViewSide == BWType.Black
                ? cellSquare
                : cellSquare.Flip());
            var flipSquares =
                (squares == null || Container.ViewSide == BWType.Black
                ? squares
                : squares.Where(_ => _ != null).Select(_ => _.Flip()));

            return new CellEffectContext()
            {
                CellSquare = flipSquare,
                CellSquares = flipSquares.ToArray(),
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
                var position = board.LastMove.DstSquare;

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
        private void UpdateMovableCell(Square square, BoardPiece piece)
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
            var isMove = (square != null);
            var moveSquares =
                from file in Enumerable.Range(1, Board.BoardSize)
                from rank in Enumerable.Range(1, Board.BoardSize)
                let move = new BoardMove()
                {
                    DstSquare = square,
                    SrcSquare = new Square(file, rank),
                    BWType = piece.BWType,
                    DropPieceType = (isMove ? PieceType.None : piece.PieceType),
                }
                let move2 = (Board.IsPromoteForce(move) ?
                    move.Apply(_ => _.IsPromote = true) : move)
                where board.CanMove(move2)
                select move2.DstSquare;

            // 移動可能なマスにエフェクトをかけます。
            var movableCell = EffectTable.MovableCell.LoadEffect();
            if (movableCell != null)
            {
                movableCell.DataContext = CreateCellContext(moveSquares, square);

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
                    teban = teban.Flip();
                }

                tebanCell.DataContext = CreateCellContext((Square)null)
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
        private void AddEffect(EffectObject effect, Square square)
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
                effect.DataContext = CreateContext(square);

                Container.AddEffect(effect);
            });
        }

        /// <summary>
        /// エフェクトを追加します。
        /// </summary>
        private void AddEffect(EffectInfo effectInfo, Square square)
        {
            var effect = effectInfo.LoadEffect();

            AddEffect(effect, square);
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
        private void AddMoveEffect(Square square, BoardMove move)
        {
            var table = new Dictionary<string, object>
            {
                { "Color",  TebanParticleColor(move.BWType) },
            };

            var effect = EffectTable.PieceMove.LoadEffect(table);
            AddEffect(effect, square);
        }

        /// <summary>
        /// 駒取りエフェクトです。
        /// </summary>
        private void AddTookEffect(Square square, Piece tookPiece, BWType bwType)
        {
            var bp = Container.SquareToPoint(square);
            var ep = Container.CapturedPieceToPoint(tookPiece.PieceType, bwType);
            var d = Vector3D.Subtract(ep, bp);
            var rad = Math.Atan2(d.Y, d.X) + Math.PI;

            var table = new Dictionary<string, object>
            {
                { "TargetXY",  new Vector(d.X, d.Y) },
                { "StartAngle", MathEx.ToDeg(rad) },
                { "Color",  TebanParticleColor(bwType) },
            };

            var effect = EffectTable.PieceTook.LoadEffect(table);
            AddEffect(effect, square);
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
        void IEffectManager.BeginMove(Square square, BoardPiece piece)
        {
            if (Container == null)
            {
                return;
            }

            UpdateMovableCell(square, piece);
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
        private Square FindGyoku(Board board, BWType bwType)
        {
            var squares =
                from file in Enumerable.Range(1, Board.BoardSize)
                from rank in Enumerable.Range(1, Board.BoardSize)
                let piece = board[file, rank]
                where piece != null &&
                      piece.PieceType == PieceType.Gyoku &&
                      piece.BWType == bwType
                select new Square(file, rank);
            if (squares.Count() != 1)
            {
                return null;
            }

            return squares.FirstOrDefault();
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
            var square = FindGyoku(board, board.Turn);
            if (square == null)
            {
                return;
            }

            AddEffect(EffectTable.Win, square);
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
                if (move.SrcSquare != null &&
                    HasEffectFlag(EffectFlag.Piece))
                {
                    AddMoveEffect(move.SrcSquare, move);
                }

                UpdateTeban(move.BWType);
                return;
            }

            UpdateTeban(move.BWType.Flip());

            if (HasEffectFlag(EffectFlag.Piece))
            {
                if (move.TookPiece != null)
                {
                    AddTookEffect(move.DstSquare, move.TookPiece, move.BWType);
                }

                if (move.ActionType == ActionType.Drop)
                {
                    AddEffect(EffectTable.PieceDrop, move.DstSquare);
                }
                else if (move.ActionType == ActionType.Promote)
                {
                    AddEffect(EffectTable.Promote, move.DstSquare);
                }

                AddMoveEffect(move.DstSquare, move);
            }
        }
        #endregion
    }
}
