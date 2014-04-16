using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;

using Ragnarok;
using Ragnarok.Utility;

namespace ServerProxy
{
    /// <summary>
    /// 入力をコンソールからも受け付けるストリームクラスです。
    /// </summary>
    public sealed class WithConsoleStream : Stream
    {
        private readonly Stream consoleInput;
        private readonly List<ArraySegment<byte>> inputList =
            new List<ArraySegment<byte>>();

        /// <summary>
        /// コンストラクタ
        /// </summary>
        public WithConsoleStream(Stream stream)
        {
            Stream = stream;
            this.consoleInput = Console.OpenStandardInput(2048);

            InternalBeginRead(Stream);
            InternalBeginRead(this.consoleInput);
        }

        /// <summary>
        /// 内部ストリームを取得します。
        /// </summary>
        public Stream Stream
        {
            get;
            private set;
        }

        /// <summary>
        /// 現在のストリームが読み取りをサポートするかどうかを示す値を取得します。
        /// </summary>
        public override bool CanRead
        {
            get { return true; }
        }

        /// <summary>
        /// 現在のストリームが書き込みをサポートするかどうかを示す値を取得します。
        /// </summary>
        public override bool CanWrite
        {
            get { return Stream.CanWrite; }
        }

        /// <summary>
        /// 現在のストリームがシークをサポートするかどうかを示す値を取得します。
        /// </summary>
        public override bool CanSeek
        {
            get { return Stream.CanSeek; }
        }

        /// <summary>
        /// ストリームの長さをバイト単位で取得します。
        /// </summary>
        public override long Length
        {
            get { return Stream.Length; }
        }

        /// <summary>
        /// 現在のストリーム内の位置を取得または設定します。
        /// </summary>
        public override long Position
        {
            get { return Stream.Position; }
            set { Stream.Position = value; }
        }

        /// <summary>
        /// 現在のストリーム内の位置を設定します。
        /// </summary>
        public override long Seek(long offset, SeekOrigin origin)
        {
            return Stream.Seek(offset, origin);
        }

        /// <summary>
        /// 現在のストリームの長さを設定します。
        /// </summary>
        public override void SetLength(long value)
        {
            Stream.SetLength(value);
        }

        /// <summary>
        /// ストリームに対応するすべてのバッファーをクリアし、
        /// バッファー内のデータを基になるデバイスに書き込みます。
        /// </summary>
        public override void Flush()
        {
            Stream.Flush();
        }

        private void InternalBeginRead(Stream stream)
        {
            var buffer = new byte[256];

            try
            {
                stream.BeginRead(
                    buffer, 0, buffer.Length,
                    InternalReadDone,
                    Tuple.Create(stream, buffer));
            }
            catch (Exception ex)
            {
                Log.ErrorException(ex,
                    "受信開始処理に失敗しました。");
            }
        }

        private void InternalReadDone(IAsyncResult result)
        {
            try
            {
                var data = (Tuple<Stream, byte[]>)result.AsyncState;
                var size = data.Item1.EndRead(result);
                if (size == 0)
                {
                    return;
                }

                // 保存バッファに書き込みます。
                lock (this.inputList)
                {
                    this.inputList.Add(new ArraySegment<byte>(data.Item2, 0, size));

                    Monitor.PulseAll(this.inputList);
                }

                InternalBeginRead(data.Item1);
            }
            catch (Exception ex)
            {
                Log.ErrorException(ex,
                    "何かに失敗しました。");
            }
        }

        /// <summary>
        /// 現在のストリームからバイト シーケンスを読み取り、
        /// 読み取ったバイト数の分だけストリームの位置を進めます。
        /// </summary>
        public override int Read(byte[] buffer, int offset, int count)
        {
            lock (this.inputList)
            {
                while (!this.inputList.Any())
                {
                    Monitor.Wait(this.inputList);
                }

                var segment = this.inputList[0];
                var size = Math.Min(segment.Count, count);
                Array.Copy(
                    segment.Array, segment.Offset,
                    buffer, offset,
                    size);

                // 古いデータを削除するとともに、
                // 残ったデータを再度バッファに入れます。
                this.inputList.RemoveAt(0);
                if (segment.Count > size)
                {
                    this.inputList.Insert(0,
                        new ArraySegment<byte>(
                            segment.Array,
                            segment.Offset + size,
                            segment.Count - size));
                }

                return size;
            }
        }

        /// <summary>
        /// 現在のストリームにバイト シーケンスを書き込み、
        /// 書き込んだバイト数の分だけストリームの現在位置を進めます。
        /// </summary>
        public override void Write(byte[] buffer, int offset, int count)
        {
            Stream.Write(buffer, offset, count);
        }
    }
}
