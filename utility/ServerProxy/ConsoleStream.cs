using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

using Ragnarok;
using Ragnarok.Utility;

namespace ServerProxy
{
    public sealed class ConsoleStream : Stream
    {
        private readonly Stream inputStream;
        private readonly BinarySplitReader reader =
            new BinarySplitReader(2048);

        /// <summary>
        /// コンストラクタ
        /// </summary>
        public ConsoleStream()
        {
            //this.outputStream = Console.OpenStandardOutput();
            this.inputStream = Console.OpenStandardInput();
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
            get { return true; }
        }

        /// <summary>
        /// 現在のストリームがシークをサポートするかどうかを示す値を取得します。
        /// </summary>
        public override bool CanSeek
        {
            get { return false; }
        }

        /// <summary>
        /// ストリームの長さをバイト単位で取得します。
        /// </summary>
        public override long Length
        {
            get { throw new NotImplementedException(); }
        }

        /// <summary>
        /// 現在のストリーム内の位置を取得または設定します。
        /// </summary>
        public override long Position
        {
            get { throw new NotImplementedException(); }
            set { throw new NotImplementedException(); }
        }

        /// <summary>
        /// 現在のストリーム内の位置を設定します。
        /// </summary>
        public override long Seek(long offset, SeekOrigin origin)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// 現在のストリームの長さを設定します。
        /// </summary>
        public override void SetLength(long value)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// ストリームに対応するすべてのバッファーをクリアし、
        /// バッファー内のデータを基になるデバイスに書き込みます。
        /// </summary>
        public override void Flush()
        {
            Console.Out.Flush();
        }

        /// <summary>
        /// 現在のストリームからバイト シーケンスを読み取り、
        /// 読み取ったバイト数の分だけストリームの位置を進めます。
        /// </summary>
        public override int Read(byte[] buffer, int offset, int count)
        {
            return this.inputStream.Read(buffer, offset, count);
        }

        /// <summary>
        /// 現在のストリームにバイト シーケンスを書き込み、
        /// 書き込んだバイト数の分だけストリームの現在位置を進めます。
        /// </summary>
        public override void Write(byte[] buffer, int offset, int count)
        {
            if (buffer != null)
            {
                this.reader.Write(buffer);
            }

            // 行ごとにテキストに直しながら出力します。
            while (true)
            {
                var bytes = this.reader.ReadUntil((byte)'\n');
                if (bytes == null)
                {
                    break;
                }

                var line = Encoding.UTF8.GetString(bytes);
                line = line.TrimEnd('\n');
                Console.WriteLine(line);
            }
        }
    }
}
