using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Threading;

using Ragnarok;

namespace ServerProxy
{
    class Program
    {
        /// <summary>
        /// ソケットストリームを作成します。
        /// </summary>
        private static Stream Connect(string address, int port)
        {
            try
            {
                var socket = new Socket(
                    AddressFamily.InterNetwork,
                    SocketType.Stream,
                    ProtocolType.Tcp);

                socket.Connect(address, port);
                return new NetworkStream(socket);
            }
            catch (Exception ex)
            {
                Util.ThrowIfFatal(ex);

                Log.ErrorException(ex,
                    "'{0}:{1}'への接続に失敗しました。",
                    address, port);
                Thread.Sleep(10 * 1000);
            }

            return null;
        }

        static void Main(string[] args)
        {
            var proxy = new ServerProxy();

            proxy.Start(
                "CSA", () => Connect("wdoor.c.u-tokyo.ac.jp", 4081),
                "god", () => Connect("garnet-alice.net", 4090));
            /*proxy.Start(
                () => Connect("localhost", 10000),
                () => new ConsoleStream());*/

            foreach (var th in proxy.Threads)
            {
                th.Join();
            }
        }
    }
}
